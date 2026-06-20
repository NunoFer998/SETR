#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "pcd8544.h"
#include <stdint.h>

void hw_init(void);
void board_enable_audio_irq(void);
pcd8544_t *board_get_lcd(void);

extern volatile uint32_t debug_audio_irq_count;
extern volatile uint32_t debug_isr_entry_count;

#endif /* BOARD_INIT_H */