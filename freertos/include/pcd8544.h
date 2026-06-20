#ifndef PCD8544_H
#define PCD8544_H

#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

/* ─── Display dimensions ─────────────────────────────────────────────────── */
#define PCD8544_WIDTH   84
#define PCD8544_HEIGHT  48
#define PCD8544_BUF_SIZE (PCD8544_WIDTH * PCD8544_HEIGHT / 8)

/* ─── PCD8544 handle ─────────────────────────────────────────────────────── */
typedef struct {
    spi_inst_t *spi;
    uint pin_cs;
    uint pin_dc;
    uint pin_rst;
    uint pin_bl;
    uint8_t buf[PCD8544_BUF_SIZE];
} pcd8544_t;

/* ─── Public API ─────────────────────────────────────────────────────────── */

void pcd8544_init(pcd8544_t *lcd, spi_inst_t *spi,
                  uint cs, uint dc, uint rst, uint bl,
                  uint8_t contrast);
void pcd8544_reset(pcd8544_t *lcd);
void pcd8544_show(pcd8544_t *lcd);
void pcd8544_clear(pcd8544_t *lcd);
void pcd8544_fill(pcd8544_t *lcd, uint8_t value);
void pcd8544_pixel(pcd8544_t *lcd, int x, int y, uint8_t color);
void pcd8544_fill_rect(pcd8544_t *lcd, int x, int y, int w, int h, uint8_t color);

#endif /* PCD8544_H */
