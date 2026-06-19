#include "board_init.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "project_config.h"
#include "lsm9ds0_accel.h"
#include "pcd8544.h"
#include "tasks.h"

/* audio task handle declared in audio_task.c */
extern TaskHandle_t audio_task_handle;

/* DEBUG: count every IRQ hit on GP15 (visible from other tasks) */
volatile uint32_t debug_audio_irq_count = 0;
volatile uint32_t debug_isr_entry_count = 0;  // Count every ISR entry

/* Raw GPIO IRQ handler for IO_IRQ_BANK0 */
static void __isr gpio_bank0_irq_handler(void) {
    debug_isr_entry_count++;  // Increment immediately on ISR entry
    
    uint32_t events = gpio_get_irq_event_mask(MIC_IRQ_PIN);
    
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Clear the interrupt immediately
        gpio_acknowledge_irq(MIC_IRQ_PIN, GPIO_IRQ_EDGE_FALL);
        
        debug_audio_irq_count++;
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

    gpio_init(MIC_IRQ_PIN);
    gpio_set_dir(MIC_IRQ_PIN, GPIO_IN);
    gpio_pull_up(MIC_IRQ_PIN);

    pcd8544_init(&lcd, SPI_PORT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_BL_PIN, 0x3B);
}

void board_enable_audio_irq(void) {
    // Register raw IRQ handler directly (bypass SDK callback wrapper)
    irq_set_exclusive_handler(IO_IRQ_BANK0, gpio_bank0_irq_handler);
    
    // Set IRQ priority to be within FreeRTOS's safe range
    irq_set_priority(IO_IRQ_BANK0, 192);
    
    // Enable the IRQ in NVIC
    irq_set_enabled(IO_IRQ_BANK0, true);
    
    // Enable GPIO interrupt for falling edge
    gpio_set_irq_enabled(MIC_IRQ_PIN, GPIO_IRQ_EDGE_FALL, true);
}