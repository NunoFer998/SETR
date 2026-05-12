from machine import I2C, Pin, Timer
import time

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

# ─── Tasks ───────────────────────────────────────────────────
led = Pin(25, Pin.OUT)
i2c = I2C(0, sda=Pin(0), scl=Pin(1), freq=400000)
ACCEL_ADDR = 0x1d

def init_lsm():
    i2c.writeto_mem(ACCEL_ADDR, 0x20, bytes([0x67]))
    print("LSM9DS0 initialized")

def read_tilt():
    """Read accelerometer and control LED - runs every 100ms"""
    data = i2c.readfrom_mem(ACCEL_ADDR, 0x28 | 0x80, 6)
    x = (data[1] << 8 | data[0])
    if x > 32767:
        x -= 65536
    if x > 8000:
        led.value(1)
        print("RIGHT")
    elif x < -8000:
        led.value(1)
        print("LEFT")
    else:
        led.value(0)
        print("CENTER")

def heartbeat():
    """Blink LED quickly to show scheduler is alive - runs every 500ms"""
    print("-- tick --")

# ─── Main ─────────────────────────────────────────────────────
init_lsm()

sched_init(tick_ms=10)              # 10ms tick resolution

sched_add_task(read_tilt, 0, 10)    # every 100ms  (10 ticks × 10ms)
sched_add_task(heartbeat, 50, 50)   # every 500ms  (50 ticks × 10ms)

print("Scheduler running!")

while True:
    sched_dispatch()