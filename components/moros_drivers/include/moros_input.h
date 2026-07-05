#ifndef MOROS_INPUT_H
#define MOROS_INPUT_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MOROS_INPUT_CW,          /* rotated one detent forward */
    MOROS_INPUT_CCW,         /* rotated one detent backward */
    MOROS_INPUT_CLICK,       /* short press of the encoder button */
    MOROS_INPUT_LONG_PRESS,  /* button held past the long-press threshold */
} moros_input_event_t;

esp_err_t moros_input_init(void);

/* Block up to timeout_ms for the next event. Returns false on timeout. */
bool moros_input_get(moros_input_event_t *event, uint32_t timeout_ms);

#endif
