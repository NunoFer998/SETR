#include "drivers/lsm9ds0_accel.h"

#include <stdio.h>

#include "FreeRTOS.h"

#include "hardware/i2c.h"

#include "config/project_config.h"

static inline int16_t read_signed_16(const uint8_t *data, int index) {
    uint16_t raw = (uint16_t)data[index] | ((uint16_t)data[index + 1] << 8);
    return (int16_t)raw;
}

void accel_init(void) {
    uint8_t buf[2] = {0x20, 0x67};
    i2c_write_blocking(I2C_PORT, ACCEL_ADDR, buf, 2, false);
    printf("[HW] LSM9DS0 accelerometer initialized\n");
}

int16_t accel_read_x(void) {
    uint8_t reg = 0x28 | 0x80;
    uint8_t data[6];

    i2c_write_blocking(I2C_PORT, ACCEL_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, ACCEL_ADDR, data, 6, false);

    return read_signed_16(data, 0);
}