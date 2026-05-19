import time

import pcd8544 as pcd8544
from machine import I2C, Pin, SPI


def read_signed_16(data, index):
	value = data[index] | (data[index + 1] << 8)
	if value > 32767:
		value -= 65536
	return value


sck_pin = Pin(6)
mosi_pin = Pin(7)

# Initialize SPI0 with the specific clock and data pins
spi = SPI(0, baudrate=4000000, polarity=0, phase=0, sck=sck_pin, mosi=mosi_pin)

cs = Pin(5)
dc = Pin(4)
rst = Pin(8)

bl = Pin(9, Pin.OUT, value=1)

lcd = pcd8544.PCD8544_FB(spi, cs, dc, rst)
lcd.init(horizontal=True, contrast=0x3b)

i2c = I2C(0, sda=Pin(0), scl=Pin(1), freq=400000)
ACCEL_ADDR = 0x1d

# Power up the accelerometer like the main application does.
i2c.writeto_mem(ACCEL_ADDR, 0x20, bytes([0x67]))

SQUARE_SIZE = 10
SQUARE_Y = (48 - SQUARE_SIZE) // 2
DEAD_ZONE = 6000
MAX_RAW = 16000


while True:
	data = i2c.readfrom_mem(ACCEL_ADDR, 0x28 | 0x80, 6)
	x = read_signed_16(data, 0)

	if x > DEAD_ZONE:
		x = min(x - DEAD_ZONE, MAX_RAW)
	elif x < -DEAD_ZONE:
		x = max(x + DEAD_ZONE, -MAX_RAW)
	else:
		x = 0

	max_x = 84 - SQUARE_SIZE
	square_x = int((x + MAX_RAW) * max_x / (2 * MAX_RAW))
	square_x = max(0, min(max_x, square_x))

	lcd.fill(0)
	lcd.fill_rect(square_x, SQUARE_Y, SQUARE_SIZE, SQUARE_SIZE, 1)
	lcd.show()

	time.sleep_ms(30)