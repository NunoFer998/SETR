#include "pcd8544.h"
#include "pico/stdlib.h"
#include <string.h>

/* ─── PCD8544 command constants ──────────────────────────────────────────── */
#define FUNCTION_SET     0x20
#define POWER_DOWN       0x04
#define ADDRESSING_VERT  0x02
#define EXTENDED_INSTR   0x01

#define DISPLAY_NORMAL   0x0C
#define DISPLAY_BLANK    0x08

#define TEMP_COEFF_2     0x06
#define BIAS_1_40        0x14
#define SET_VOP          0x80

#define COL_ADDR         0x80
#define BANK_ADDR        0x40

/* ─── Low-level helpers ──────────────────────────────────────────────────── */

static void pcd8544_cmd(pcd8544_t *lcd, uint8_t cmd) {
    gpio_put(lcd->pin_dc, 0);  /* command mode */
    gpio_put(lcd->pin_cs, 0);
    spi_write_blocking(lcd->spi, &cmd, 1);
    gpio_put(lcd->pin_cs, 1);
}

static void pcd8544_data(pcd8544_t *lcd, const uint8_t *data, size_t len) {
    gpio_put(lcd->pin_dc, 1);  /* data mode */
    gpio_put(lcd->pin_cs, 0);
    spi_write_blocking(lcd->spi, data, len);
    gpio_put(lcd->pin_cs, 1);
}

/* ─── Public implementation ──────────────────────────────────────────────── */

void pcd8544_reset(pcd8544_t *lcd) {
    gpio_put(lcd->pin_rst, 1);
    sleep_us(100);
    gpio_put(lcd->pin_rst, 0);
    sleep_us(100);
    gpio_put(lcd->pin_rst, 1);
    sleep_us(100);
}

void pcd8544_init(pcd8544_t *lcd, spi_inst_t *spi,
                  uint cs, uint dc, uint rst, uint bl,
                  uint8_t contrast) {
    lcd->spi     = spi;
    lcd->pin_cs  = cs;
    lcd->pin_dc  = dc;
    lcd->pin_rst = rst;
    lcd->pin_bl  = bl;

    /* Configure GPIO pins */
    gpio_init(cs);   gpio_set_dir(cs,  GPIO_OUT); gpio_put(cs, 1);
    gpio_init(dc);   gpio_set_dir(dc,  GPIO_OUT); gpio_put(dc, 0);
    gpio_init(rst);  gpio_set_dir(rst, GPIO_OUT); gpio_put(rst, 1);
    gpio_init(bl);   gpio_set_dir(bl,  GPIO_OUT); gpio_put(bl, 1);

    /* Clear framebuffer */
    memset(lcd->buf, 0, PCD8544_BUF_SIZE);

    /* Hardware reset */
    pcd8544_reset(lcd);

    /* Initialization sequence (horizontal addressing, set contrast) */
    uint8_t fn = FUNCTION_SET;  /* horizontal addressing, basic instruction set */

    /* Enter extended instruction set to configure contrast */
    pcd8544_cmd(lcd, fn | EXTENDED_INSTR);
    pcd8544_cmd(lcd, TEMP_COEFF_2);
    pcd8544_cmd(lcd, BIAS_1_40);
    pcd8544_cmd(lcd, SET_VOP | (contrast & 0x7F));
    /* Return to basic instruction set */
    pcd8544_cmd(lcd, fn & ~EXTENDED_INSTR);

    pcd8544_cmd(lcd, DISPLAY_NORMAL);

    /* Clear display RAM */
    pcd8544_cmd(lcd, COL_ADDR | 0);
    pcd8544_cmd(lcd, BANK_ADDR | 0);
    uint8_t zeros[PCD8544_BUF_SIZE];
    memset(zeros, 0, PCD8544_BUF_SIZE);
    pcd8544_data(lcd, zeros, PCD8544_BUF_SIZE);
}

void pcd8544_show(pcd8544_t *lcd) {
    /* Reset cursor to top-left */
    pcd8544_cmd(lcd, COL_ADDR | 0);
    pcd8544_cmd(lcd, BANK_ADDR | 0);
    /* Write entire framebuffer */
    pcd8544_data(lcd, lcd->buf, PCD8544_BUF_SIZE);
}

void pcd8544_clear(pcd8544_t *lcd) {
    memset(lcd->buf, 0, PCD8544_BUF_SIZE);
}

void pcd8544_fill(pcd8544_t *lcd, uint8_t value) {
    memset(lcd->buf, value, PCD8544_BUF_SIZE);
}

void pcd8544_pixel(pcd8544_t *lcd, int x, int y, uint8_t color) {
    if (x < 0 || x >= PCD8544_WIDTH || y < 0 || y >= PCD8544_HEIGHT)
        return;
    /*
     * MONO_VLSB layout: each byte is a vertical column of 8 pixels.
     * Byte index = x + (y / 8) * WIDTH
     * Bit position = y % 8  (LSB = top pixel of the 8-row bank)
     */
    uint16_t idx = x + (y / 8) * PCD8544_WIDTH;
    if (color)
        lcd->buf[idx] |= (1 << (y & 7));
    else
        lcd->buf[idx] &= ~(1 << (y & 7));
}

void pcd8544_fill_rect(pcd8544_t *lcd, int x, int y, int w, int h, uint8_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            pcd8544_pixel(lcd, i, j, color);
        }
    }
}
