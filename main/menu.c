#include "menu.h"
#include "moros_hal.h"
#include "moros_display.h"
#include "moros_font.h"

#define ACCENT      0xA81F  /* MOROS purple */
#define MENU_TOP    10
#define ROW_HEIGHT  30
#define TEXT_SCALE  2
#define TEXT_X      20
#define TEXT_PAD    ((ROW_HEIGHT - 8 * TEXT_SCALE) / 2)

static void draw_row(const moros_menu_t *menu, int i)
{
    uint16_t y = MENU_TOP + i * ROW_HEIGHT;
    int selected = (i == menu->selected);
    uint16_t bg = selected ? ACCENT : MOROS_COLOR_BLACK;
    uint16_t fg = selected ? MOROS_COLOR_BLACK : ACCENT;

    moros_display_fill_rect(0, y, MOROS_LCD_WIDTH, ROW_HEIGHT, bg);
    moros_draw_text(TEXT_X, y + TEXT_PAD, menu->items[i], fg, TEXT_SCALE);
}

void moros_menu_draw(const moros_menu_t *menu)
{
    for (int i = 0; i < menu->count; i++)
        draw_row(menu, i);
}

void moros_menu_move(moros_menu_t *menu, int delta)
{
    int prev = menu->selected;
    menu->selected = (menu->selected + delta + menu->count) % menu->count;
    if (menu->selected == prev) return;

    draw_row(menu, prev);
    draw_row(menu, menu->selected);
}
