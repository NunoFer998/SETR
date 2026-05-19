#ifndef PCD8544_H
#define PCD8544_H

#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

/* ─── Display dimensions ─────────────────────────────────────────────────── */
#define PCD8544_WIDTH   84
#define PCD8544_HEIGHT  48
#define PCD8544_BUF_SIZE (PCD8544_WIDTH * PCD8544_HEIGHT / 8)  /* 504 bytes */

/* ─── PCD8544 handle ─────────────────────────────────────────────────────── */
typedef struct {
    spi_inst_t *spi;
    uint pin_cs;
    uint pin_dc;
    uint pin_rst;
    uint pin_bl;
    uint8_t buf[PCD8544_BUF_SIZE];  /* MONO_VLSB framebuffer */
} pcd8544_t;

/* ─── Public API ─────────────────────────────────────────────────────────── */

/**
 * Initialize the display hardware and clear the screen.
 * Call once at startup after configuring SPI and GPIO.
 */
void pcd8544_init(pcd8544_t *lcd, spi_inst_t *spi,
                  uint cs, uint dc, uint rst, uint bl,
                  uint8_t contrast);

/** Reset the display via the RST pin. */
void pcd8544_reset(pcd8544_t *lcd);

/** Send the entire framebuffer to the display (504 bytes). */
void pcd8544_show(pcd8544_t *lcd);

/** Clear the framebuffer (all pixels off). */
void pcd8544_clear(pcd8544_t *lcd);

/** Fill the entire framebuffer with a value (0x00 = off, 0xFF = on). */
void pcd8544_fill(pcd8544_t *lcd, uint8_t value);

/**
 * Set a single pixel in the framebuffer.
 * @param color  1 = pixel on (dark), 0 = pixel off
 */
void pcd8544_pixel(pcd8544_t *lcd, int x, int y, uint8_t color);

/**
 * Draw a filled rectangle in the framebuffer.
 */
void pcd8544_fill_rect(pcd8544_t *lcd, int x, int y, int w, int h, uint8_t color);

#endif /* PCD8544_H */
