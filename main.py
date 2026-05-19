from machine import I2C, Pin, Timer, SPI
import pcd8544 as pcd8544

# ─── Scheduler ───────────────────────────────────────────────
MAX_TASKS = 10
tasks = []
timer = Timer()

def sched_init(tick_ms=10):
    """Initialize scheduler with tick period in ms"""
    global tasks
    tasks = []
    timer.init(period=tick_ms, mode=Timer.PERIODIC, callback=_sched_schedule)

def sched_add_task(func, delay_ticks, period_ticks):
    """Add a task: func=function, delay=start offset, period=repeat (0=one-shot)"""
    if len(tasks) >= MAX_TASKS:
        print("Max tasks reached!")
        return -1
    task = {
        'func': func,
        'delay': delay_ticks,
        'period': period_ticks,
        'exec': 0,
        'enabled': True
    }
    tasks.append(task)
    return len(tasks) - 1

def _sched_schedule(t):
    """Called every tick by hardware timer - marks tasks as ready"""
    for task in tasks:
        if not task['enabled']:
            continue
        if task['delay'] > 0:
            task['delay'] -= 1
        else:
            task['exec'] += 1  # accumulate activations
            if task['period'] > 0:
                task['delay'] = task['period'] - 1
            else:
                task['enabled'] = False  # one-shot: disable after scheduling

def sched_dispatch():
    """Execute tasks that are ready - call this in main loop"""
    for task in tasks:
        if task['exec'] > 0:
            task['exec'] = 0  # reset (discard late activations)
            task['func']()

# ─── Hardware Setup ──────────────────────────────────────────
i2c = I2C(0, sda=Pin(0), scl=Pin(1), freq=400000)
ACCEL_ADDR = 0x1d

sck_pin = Pin(6)
mosi_pin = Pin(7)
spi = SPI(0, baudrate=4000000, polarity=0, phase=0, sck=sck_pin, mosi=mosi_pin)
cs = Pin(5)
dc = Pin(4)
rst = Pin(8)
bl = Pin(9, Pin.OUT, value=1)

lcd = pcd8544.PCD8544_FB(spi, cs, dc, rst)
lcd.init(horizontal=True, contrast=0x3b)

# ─── Game State ──────────────────────────────────────────────
accel_x = 0
square_x = 42  # center of 84px display

SQUARE_SIZE = 10
SQUARE_Y = (48 - SQUARE_SIZE) // 2
DEAD_ZONE = 6000
MAX_RAW = 16000

def read_signed_16(data, index):
    value = data[index] | (data[index + 1] << 8)
    if value > 32767:
        value -= 65536
    return value

# ─── Tasks ───────────────────────────────────────────────────

def task_read_input():
    """Task 1: Read accelerometer - runs every 30ms"""
    global accel_x
    data = i2c.readfrom_mem(ACCEL_ADDR, 0x28 | 0x80, 6)
    accel_x = read_signed_16(data, 0)
    print(f"Accel X: {accel_x}")

def task_update_square():
    """Task 2: Update square position based on tilt - runs every 30ms"""
    global square_x, accel_x
    x = accel_x
    
    # Apply dead zone
    if x > DEAD_ZONE:
        x = min(x - DEAD_ZONE, MAX_RAW)
    elif x < -DEAD_ZONE:
        x = max(x + DEAD_ZONE, -MAX_RAW)
    else:
        x = 0
    
    # Map to screen position (0 to 84-SQUARE_SIZE)
    max_x = 84 - SQUARE_SIZE
    square_x = int((x + MAX_RAW) * max_x / (2 * MAX_RAW))
    square_x = max(0, min(max_x, square_x))

def task_display():
    """Task 3: Render square to LCD - runs every 30ms"""
    global square_x
    lcd.fill(0)
    lcd.fill_rect(square_x, SQUARE_Y, SQUARE_SIZE, SQUARE_SIZE, 1)
    lcd.show()

# ─── Initialization ──────────────────────────────────────────

def init_hardware():
    i2c.writeto_mem(ACCEL_ADDR, 0x20, bytes([0x67]))
    print("LSM9DS0 initialized")
    print("LCD initialized")

# ─── Main ─────────────────────────────────────────────────────
init_hardware()

sched_init(tick_ms=10)  # 10ms tick resolution

sched_add_task(task_read_input, 0, 3)    # every 30ms  (3 ticks × 10ms)
sched_add_task(task_update_square, 0, 3) # every 30ms
sched_add_task(task_display, 0, 3)       # every 30ms

print("Tetris scheduler running!")

while True:
    sched_dispatch()