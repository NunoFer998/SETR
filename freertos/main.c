/*
 * main.c — FreeRTOS Tetris Scheduler for Raspberry Pi Pico
 *
 * Features:
 *   • Preemptive, priority-based scheduling (FreeRTOS kernel)
 *   • Mutex-protected shared state (accel_x, square_x)
 *   • Three periodic tasks at different priorities:
 *       1. Input   (HIGH)   — reads accelerometer via I2C
 *       2. Update  (MEDIUM) — computes new square position
 *       3. Display (LOW)    — renders to PCD8544 LCD
 *
 * Pin mapping (matches your MicroPython wiring):
 *   I2C0: SDA=GP0, SCL=GP1   (LSM9DS0 accelerometer @ 0x1D)
 *   SPI0: SCK=GP6, MOSI=GP7  (PCD8544 LCD)
 *   GPIO: CS=GP5, DC=GP4, RST=GP8, BL=GP9
 *   Buttons: UP=GP10, DOWN=GP11, DIAG(comparator)=GP15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "pcd8544.h"

/* ═══════════════════════════════════════════════════════════════════════════
 *  Hardware Configuration
 * ═══════════════════════════════════════════════════════════════════════════ */

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
#define LSM_OUT_PIN     15

/* ═══════════════════════════════════════════════════════════════════════════
 *  Game Constants
 * ═══════════════════════════════════════════════════════════════════════════ */
#define SQUARE_SIZE     10
#define INIT_SQUARE_Y   ((PCD8544_HEIGHT - SQUARE_SIZE) / 2)
#define DEAD_ZONE       6000
#define MAX_RAW         16000
#define BUTTON_STEP     2   /* pixels per frame when a button is held */

/* ═══════════════════════════════════════════════════════════════════════════
 *  Task Priorities (higher number = higher priority)
 *
 *  Input has the highest priority because reading the accelerometer is
 *  time-sensitive — we don't want display rendering to delay sensor reads.
 *  Update is medium: it processes already-read data.
 *  Display is lowest: it can tolerate jitter; humans won't notice a few ms.
 * ═══════════════════════════════════════════════════════════════════════════ */
#define PRIORITY_INPUT    (tskIDLE_PRIORITY + 3)  /* HIGH   */
#define PRIORITY_UPDATE   (tskIDLE_PRIORITY + 2)  /* MEDIUM */
#define PRIORITY_DISPLAY  (tskIDLE_PRIORITY + 1)  /* LOW    */

/* Task stack sizes (in words, 1 word = 4 bytes on Cortex-M0+) */
#define STACK_INPUT       512
#define STACK_UPDATE      512
#define STACK_DISPLAY     512

/* Task periods in milliseconds */
#define PERIOD_INPUT_MS   30
#define PERIOD_UPDATE_MS  30
#define PERIOD_DISPLAY_MS 30

/* ═══════════════════════════════════════════════════════════════════════════
 *  Shared State — Protected by Mutexes
 *
 *  accel_mutex guards accel_x: written by Input, read by Update.
 *  display_mutex guards square_x, square_y AND the LCD hardware: written by
 *  Update, read/used by Display.
 *
 *  This prevents data races and, because FreeRTOS mutexes implement
 *  priority inheritance, avoids unbounded priority inversion.
 * ═══════════════════════════════════════════════════════════════════════════ */
static SemaphoreHandle_t accel_mutex   = NULL;
static SemaphoreHandle_t display_mutex = NULL;

static volatile int16_t accel_x  = 0;            /* raw accelerometer X reading */
static volatile int     square_x = 42;           /* LCD column position of square */
static volatile int     square_y = INIT_SQUARE_Y; /* LCD row position of square */

/* LCD handle (global, but display_mutex protects concurrent access) */
static pcd8544_t lcd;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Accelerometer Helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Read a signed 16-bit value from a 2-byte little-endian buffer.
 */
static inline int16_t read_signed_16(const uint8_t *data, int index) {
    uint16_t raw = (uint16_t)data[index] | ((uint16_t)data[index + 1] << 8);
    return (int16_t)raw;
}

/**
 * Initialize the LSM9DS0 accelerometer.
 * Register 0x20 (CTRL_REG1_XM): 0x67 = 100 Hz ODR, all axes enabled.
 */
static void accel_init(void) {
    uint8_t buf[2] = {0x20, 0x67};
    i2c_write_blocking(I2C_PORT, ACCEL_ADDR, buf, 2, false);
    printf("[HW] LSM9DS0 accelerometer initialized\n");
}

/**
 * Read the 6 accelerometer data bytes (X, Y, Z — 16 bits each).
 * Uses auto-increment (register address | 0x80) to read all 6 in one burst.
 * Returns X-axis as signed 16-bit value.
 */
static int16_t accel_read_x(void) {
    uint8_t reg = 0x28 | 0x80;  /* auto-increment starting at OUT_X_L_A */
    uint8_t data[6];

    i2c_write_blocking(I2C_PORT, ACCEL_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, ACCEL_ADDR, data, 6, false);

    return read_signed_16(data, 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Task 1: Input — Read Accelerometer (HIGHEST PRIORITY)
 *
 *  This task preempts Update and Display whenever it becomes ready.
 *  It reads the I2C accelerometer and writes the result into accel_x
 *  under mutex protection.
 * ═══════════════════════════════════════════════════════════════════════════ */
static void task_read_input(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        /* Block until the next period — yields CPU to lower-priority tasks */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_INPUT_MS));

        int16_t raw_x = accel_read_x();

        /* --- Critical section: write shared accel_x --- */
        if (xSemaphoreTake(accel_mutex, portMAX_DELAY) == pdTRUE) {
            accel_x = raw_x;
            xSemaphoreGive(accel_mutex);
        }

        printf("[INPUT] Accel X: %d\n", raw_x);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Task 2: Update — Compute Square Position (MEDIUM PRIORITY)
 *
 *  Reads accel_x (under accel_mutex), reads button GPIOs, computes the new
 *  square position, then writes square_x and square_y (under display_mutex).
 * ═══════════════════════════════════════════════════════════════════════════ */
static void task_update_square(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_UPDATE_MS));

        /* --- Read shared accel_x --- */
        int16_t local_x = 0;
        if (xSemaphoreTake(accel_mutex, portMAX_DELAY) == pdTRUE) {
            local_x = accel_x;
            xSemaphoreGive(accel_mutex);
        }

        /* --- Read button / comparator GPIOs (instant, no mutex needed) --- */
        bool btn_up   = gpio_get(LEFT_BUTTON);   /* GP10 — move up   */
        bool btn_down = gpio_get(RIGHT_BUTTON);   /* GP11 — move down */
        bool btn_diag = gpio_get(LSM_OUT_PIN);   /* GP15 — diagonal  */

        /* Apply dead zone */
        int x = (int)local_x;
        if (x > DEAD_ZONE) {
            x = x - DEAD_ZONE;
            if (x > MAX_RAW) x = MAX_RAW;
        } else if (x < -DEAD_ZONE) {
            x = x + DEAD_ZONE;
            if (x < -MAX_RAW) x = -MAX_RAW;
        } else {
            x = 0;
        }

        /* Map accelerometer to horizontal screen position */
        int max_x = PCD8544_WIDTH - SQUARE_SIZE;
        int new_pos_x = (x + MAX_RAW) * max_x / (2 * MAX_RAW);
        if (new_pos_x < 0)     new_pos_x = 0;
        if (new_pos_x > max_x) new_pos_x = max_x;

        /* --- Compute vertical movement from buttons --- */
        int max_y = PCD8544_HEIGHT - SQUARE_SIZE;
        int new_pos_y;

        /* Read current square_y under mutex */
        if (xSemaphoreTake(display_mutex, portMAX_DELAY) == pdTRUE) {
            new_pos_y = square_y;
            xSemaphoreGive(display_mutex);
        } else {
            new_pos_y = INIT_SQUARE_Y;
        }

        if (btn_up) {
            new_pos_y -= BUTTON_STEP;        /* move up */
        }
        if (btn_down) {
            new_pos_y += BUTTON_STEP;        /* move down */
        }
        if (btn_diag) {
            new_pos_y -= BUTTON_STEP;        /* diagonal: up … */
            new_pos_x += BUTTON_STEP;        /* … and to the right */
        }

        /* Clamp to screen bounds */
        if (new_pos_y < 0)     new_pos_y = 0;
        if (new_pos_y > max_y) new_pos_y = max_y;
        if (new_pos_x < 0)     new_pos_x = 0;
        if (new_pos_x > max_x) new_pos_x = max_x;

        /* --- Write shared square_x and square_y --- */
        if (xSemaphoreTake(display_mutex, portMAX_DELAY) == pdTRUE) {
            square_x = new_pos_x;
            square_y = new_pos_y;
            xSemaphoreGive(display_mutex);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Task 3: Display — Render to LCD (LOWEST PRIORITY)
 *
 *  Reads square_x/square_y and exclusively accesses the SPI LCD, all under
 *  display_mutex.  Because this has the lowest priority, the Input and
 *  Update tasks can preempt it at any time (except inside the critical
 *  section where it holds the mutex — and thanks to priority inheritance,
 *  if Input or Update need display_mutex they temporarily boost Display's
 *  priority to avoid inversion).
 * ═══════════════════════════════════════════════════════════════════════════ */
static void task_display(void *params) {
    (void)params;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_DISPLAY_MS));

        /* --- Critical section: read square position + write to LCD --- */
        if (xSemaphoreTake(display_mutex, portMAX_DELAY) == pdTRUE) {
            int pos_x = square_x;
            int pos_y = square_y;

            pcd8544_clear(&lcd);
            pcd8544_fill_rect(&lcd, pos_x, pos_y, SQUARE_SIZE, SQUARE_SIZE, 1);
            pcd8544_show(&lcd);

            xSemaphoreGive(display_mutex);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Hardware Initialization
 * ═══════════════════════════════════════════════════════════════════════════ */

static void hw_init(void) {
    stdio_init_all();

    /* ── I2C setup ────────────────────────────────────────────────────────── */
    i2c_init(I2C_PORT, I2C_FREQ_HZ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    /* ── SPI setup ────────────────────────────────────────────────────────── */
    spi_init(SPI_PORT, SPI_BAUD);
    gpio_set_function(SPI_SCK_PIN,  GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);

    /* ── Accelerometer ────────────────────────────────────────────────────── */
    accel_init();

    /* ── Button / Comparator GPIOs ────────────────────────────────────────── */
    gpio_init(LEFT_BUTTON);
    gpio_set_dir(LEFT_BUTTON, GPIO_IN);
    gpio_pull_down(LEFT_BUTTON);

    gpio_init(RIGHT_BUTTON);
    gpio_set_dir(RIGHT_BUTTON, GPIO_IN);
    gpio_pull_down(RIGHT_BUTTON);

    gpio_init(LSM_OUT_PIN);
    gpio_set_dir(LSM_OUT_PIN, GPIO_IN);
    gpio_pull_down(LSM_OUT_PIN);
    printf("[HW] Button GPIOs initialized (GP10, GP11, GP15)\n");

    /* ── LCD ──────────────────────────────────────────────────────────────── */
    pcd8544_init(&lcd, SPI_PORT, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, LCD_BL_PIN, 0x3B);
    printf("[HW] PCD8544 LCD initialized\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  FreeRTOS Hooks
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Called when a stack overflow is detected.
 * Required because configCHECK_FOR_STACK_OVERFLOW is set to 2.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
    for (;;) {
        tight_loop_contents();
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Main — Create mutexes, tasks, and start the scheduler
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    hw_init();

    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   FreeRTOS Tetris — Preemptive Scheduler     ║\n");
    printf("║   Priorities: Input=3  Update=2  Display=1   ║\n");
    printf("║   Mutexes: accel_mutex, display_mutex        ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    printf("\n");

    /* ── Create mutexes ──────────────────────────────────────────────────── */
    accel_mutex   = xSemaphoreCreateMutex();
    display_mutex = xSemaphoreCreateMutex();

    if (accel_mutex == NULL || display_mutex == NULL) {
        printf("[FATAL] Failed to create mutexes!\n");
        return 1;
    }
    printf("[RTOS] Mutexes created (priority inheritance enabled)\n");

    /* ── Create tasks ────────────────────────────────────────────────────── */
    BaseType_t ret;

    ret = xTaskCreate(task_read_input,   "Input",   STACK_INPUT,   NULL, PRIORITY_INPUT,   NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Input'   created — priority %d, period %d ms\n", PRIORITY_INPUT, PERIOD_INPUT_MS);

    ret = xTaskCreate(task_update_square, "Update",  STACK_UPDATE,  NULL, PRIORITY_UPDATE,  NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Update'  created — priority %d, period %d ms\n", PRIORITY_UPDATE, PERIOD_UPDATE_MS);

    ret = xTaskCreate(task_display,       "Display", STACK_DISPLAY, NULL, PRIORITY_DISPLAY, NULL);
    configASSERT(ret == pdPASS);
    printf("[RTOS] Task 'Display' created — priority %d, period %d ms\n", PRIORITY_DISPLAY, PERIOD_DISPLAY_MS);

    /* ── Start the FreeRTOS scheduler ────────────────────────────────────── */
    printf("[RTOS] Starting preemptive scheduler...\n\n");
    vTaskStartScheduler();

    /* Should never reach here — the scheduler runs forever */
    printf("[FATAL] Scheduler exited!\n");
    for (;;) {
        tight_loop_contents();
    }

    return 0;
}
