#ifndef MOROS_DISPLAY_H
#define MOROS_DISPLAY_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t moros_display_init(void);
void moros_display_fill(uint16_t color);
void moros_display_set_pixel(uint16_t x, uint16_t y, uint16_t color);

/* RGB565 color helpers */
#define MOROS_COLOR_BLACK   0x0000
#define MOROS_COLOR_WHITE   0xFFFF
#define MOROS_COLOR_RED     0xF800
#define MOROS_COLOR_GREEN   0x07E0
#define MOROS_COLOR_BLUE    0x001F

#endif
