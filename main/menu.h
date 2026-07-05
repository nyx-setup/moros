#ifndef MENU_H
#define MENU_H

typedef struct {
    const char *const *items;
    int count;
    int selected;
} moros_menu_t;

void moros_menu_draw(const moros_menu_t *menu);

/* Move the selection by delta (wraps around) and redraw only the affected rows. */
void moros_menu_move(moros_menu_t *menu, int delta);

#endif
