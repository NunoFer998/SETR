#include "board_init.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "project_config.h"
#include "lsm9ds0_accel.h"
#include "pcd8544.h"
#include "tasks.h"

/* audio task handle declared in audio_task.c */
extern TaskHandle_t audio_task_handle;

static void gpio_irq_callback(uint gpio, uint32_t events) {
    if (gpio == LSM_OUT_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
        audio_task_request_drop_from_isr();
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (audio_task_handle != NULL) {
            vTaskNotifyGiveFromISR(audio_task_handle, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static pcd8544_t lcd;

pcd8544_t *board_get_lcd(void) {
    return &lcd;
}

void hw_init(void) {
    stdio_init_all();

    i2c_init(I2C_PORT, I2C_FREQ_HZ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    spi_init(SPI_PORT, SPI_BAUD);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);

    accel_init();

    gpio_init(LEFT_BUTTON);
    gpio_set_dir(LEFT_BUTTON, GPIO_IN);
    gpio_pull_up(LEFT_BUTTON);

    gpio_init(RIGHT_BUTTON);
    gpio_set_dir(RIGHT_BUTTON, GPIO_IN);
    gpio_pull_up(RIGHT_BUTTON);

    /* Oscilloscope probe outputs (GP2, GP3) */
    gpio_init(SCOPE_PIN_INPUT);
    gpio_set_dir(SCOPE_PIN_INPUT, GPIO_OUT);
    gpio_put(SCOPE_PIN_INPUT, 0);

    gpio_init(SCOPE_PIN_DISPLAY);
    gpio_set_dir(SCOPE_PIN_DISPLAY, GPIO_OUT);
    gpio_put(SCOPE_PIN_DISPLAY, 0);

    gpio_init(LSM_OUT_PIN);
    gpio_set_dir(LSM_OUT_PIN, GPIO_IN);
    gpio_pull_down(LSM_OUT_PIN);
    /* Register the callback now, but leave the IRQ disabled until the audio task starts. */
    gpio_set_irq_enabled_with_callback(LSM_OUT_PIN, GPIO_IRQ_EDGE_RISE, false, &gpio_irq_callback);
    printf("[HW] Button GPIOs initialized (GP10, GP11, GP15)\n");

    pcd8544_init(&lcd, SPI_PORT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_BL_PIN, 0x3B);
    printf("[HW] PCD8544 LCD initialized\n");
}

void board_enable_audio_irq(void) {
    gpio_set_irq_enabled(LSM_OUT_PIN, GPIO_IRQ_EDGE_RISE, true);
}