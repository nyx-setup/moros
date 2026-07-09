#include "moros_list.h"
#include "moros_hal.h"
#include "moros_display.h"
#include "moros_font.h"

#define ACCENT      0xA81F  /* MOROS purple */
#define LIST_TOP    10
#define ROW_HEIGHT  30
#define VISIBLE_ROWS 5      /* (170 - LIST_TOP) / ROW_HEIGHT, rounded down */
#define TEXT_SCALE  2
#define TEXT_X      20
#define TEXT_PAD    ((ROW_HEIGHT - 8 * TEXT_SCALE) / 2)

static void draw_row(const moros_list_t *list, int screen_row)
{
    uint16_t y = LIST_TOP + screen_row * ROW_HEIGHT;
    int idx = list->top + screen_row;

    /* Beyond the end of a short list: clear the leftover row. */
    if (idx >= list->count) {
        moros_display_fill_rect(0, y, MOROS_LCD_WIDTH, ROW_HEIGHT, MOROS_COLOR_BLACK);
        return;
    }

    int selected = (idx == list->selected);
    uint16_t bg = selected ? ACCENT : MOROS_COLOR_BLACK;
    uint16_t fg = selected ? MOROS_COLOR_BLACK : ACCENT;

    moros_display_fill_rect(0, y, MOROS_LCD_WIDTH, ROW_HEIGHT, bg);
    moros_draw_text(TEXT_X, y + TEXT_PAD, list->items[idx], fg, TEXT_SCALE);
}

void moros_list_draw(const moros_list_t *list)
{
    for (int r = 0; r < VISIBLE_ROWS; r++)
        draw_row(list, r);
}

/* Keep the selected item inside the visible window, then clamp so the window
 * never runs past either end of the list. */
static void scroll_to_selected(moros_list_t *list)
{
    if (list->selected < list->top)
        list->top = list->selected;
    else if (list->selected >= list->top + VISIBLE_ROWS)
        list->top = list->selected - VISIBLE_ROWS + 1;

    if (list->top > list->count - VISIBLE_ROWS)
        list->top = list->count - VISIBLE_ROWS;
    if (list->top < 0)
        list->top = 0;
}

void moros_list_move(moros_list_t *list, int delta)
{
    if (list->count <= 0) return;

    int prev = list->selected;
    list->selected = (list->selected + delta + list->count) % list->count;
    if (list->selected == prev) return;

    int prev_top = list->top;
    scroll_to_selected(list);

    if (list->top != prev_top) {
        moros_list_draw(list);
    } else {
        draw_row(list, prev - list->top);
        draw_row(list, list->selected - list->top);
    }
}
