#ifndef LSM9DS0_ACCEL_H
#define LSM9DS0_ACCEL_H

#include <stdint.h>

void accel_init(void);
int16_t accel_read_x(void);

#endif /* LSM9DS0_ACCEL_H */