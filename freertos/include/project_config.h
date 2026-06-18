#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include "pcd8544.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

/* I2C — LSM9DS0 accelerometer */
#define I2C_PORT        i2c0
#define I2C_SDA_PIN     0
#define I2C_SCL_PIN     1
#define I2C_FREQ_HZ     400000
#define ACCEL_ADDR      0x1D

/* SPI — PCD8544 LCD */
#define SPI_PORT        spi0
#define SPI_BAUD        4000000
#define SPI_SCK_PIN     6
#define SPI_MOSI_PIN    7
#define LCD_CS_PIN      5
#define LCD_DC_PIN      4
#define LCD_RST_PIN     8
#define LCD_BL_PIN      9
#define LEFT_BUTTON     10
#define RIGHT_BUTTON    11

/* Microphone IRQ pin (used to trigger audio-based drops) */
#define MIC_IRQ_PIN     15

/* Game constants */
#define SQUARE_SIZE     10
#define INIT_SQUARE_Y   ((PCD8544_HEIGHT - SQUARE_SIZE) / 2)
#define DEAD_ZONE       2000   /* Reduced from 6000 for more sensitive tilt */
#define MAX_RAW         16000
#define BUTTON_STEP     2

/* Task priorities */
#define PRIORITY_INPUT    1
#define PRIORITY_UPDATE   3
#define PRIORITY_DISPLAY  2
#define PRIORITY_AUDIO    4

/* Task stack sizes (words) */
#define STACK_INPUT       512
#define STACK_UPDATE      512
#define STACK_DISPLAY     512
/* Audio task stack size (words) */
#define STACK_AUDIO       256

/* Audio sporadic server budget and replenishment */
#define AUDIO_SERVER_BUDGET_US        5000   /* 5ms of CPU time per period */
#define AUDIO_SERVER_PERIOD_MS        1000   /* 1 second replenishment period */
#define AUDIO_SERVER_MAX_REPLENISHMENTS  4

/* Task periods in milliseconds */
#define PERIOD_INPUT_MS   20
#define PERIOD_UPDATE_MS  33
#define PERIOD_DISPLAY_MS 33

#endif /* PROJECT_CONFIG_H */