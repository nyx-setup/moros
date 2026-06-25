#ifndef MOROS_FONT_H
#define MOROS_FONT_H

#include <stdint.h>

extern const uint8_t moros_font8x8[96][8];

void moros_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint8_t scale);
void moros_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale);

#endif
