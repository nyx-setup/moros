#ifndef MOROS_LIST_H
#define MOROS_LIST_H

/* Scrollable, selectable list of text rows. Shared by the main menu and the AP
 * list: same navigation concept, one of the items may be a "< Back" entry the
 * caller recognizes by index. The widget only renders and tracks selection; it
 * knows nothing about what the rows mean. */
typedef struct {
    const char *const *items;
    int count;
    int selected;   /* absolute index of the highlighted item */
    int top;        /* absolute index of the first visible row (scroll offset) */
} moros_list_t;

void moros_list_draw(const moros_list_t *list);

/* Move the selection by delta (wraps around), scroll to keep it visible, and
 * redraw the minimum needed: two rows when the window is unchanged, the whole
 * window when it scrolled. */
void moros_list_move(moros_list_t *list, int delta);

#endif
