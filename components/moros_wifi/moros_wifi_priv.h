#ifndef MOROS_WIFI_PRIV_H
#define MOROS_WIFI_PRIV_H

#include "moros_wifi.h"
#include "esp_event.h"

/* SNIFFING is drawn here but only exercised by the sniffer stub for now, so the
 * promiscuous branch slots into the same state machine later. */
typedef enum {
    MOROS_WIFI_OFF,
    MOROS_WIFI_IDLE,
    MOROS_WIFI_SCANNING,
    MOROS_WIFI_SNIFFING,
} moros_wifi_state_t;

/* Shared across the component's translation units without exposing radio state
 * on the public boundary. */
moros_wifi_state_t moros_wifi_get_state(void);
void moros_wifi_set_state(moros_wifi_state_t state);

/* Sets a flag only; the results are pulled by the UI thread in scan_done so the
 * AP array keeps a single reader/writer. Lives in the discovery unit. */
void moros_wifi_on_scan_done(void *arg, esp_event_base_t base, int32_t id, void *data);

#endif
