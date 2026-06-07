#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "pcd8544.h"

void hw_init(void);
pcd8544_t *board_get_lcd(void);

#endif /* BOARD_INIT_H */