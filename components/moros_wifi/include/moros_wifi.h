#ifndef MOROS_WIFI_H
#define MOROS_WIFI_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MOROS_WIFI_MAX_APS 32

/* Security grade the UI switches on for color-coding. Semantic to our domain,
 * not a mirror of IDF's wifi_auth_mode_t (which splits PSK/SAE variants we
 * don't care to distinguish on screen). */
typedef enum {
    MOROS_AUTH_OPEN,
    MOROS_AUTH_WEP,
    MOROS_AUTH_WPA,
    MOROS_AUTH_WPA2,
    MOROS_AUTH_WPA3,
    MOROS_AUTH_ENTERPRISE,
    MOROS_AUTH_UNKNOWN,
} moros_auth_t;

/* One discovered access point. Decoupled from IDF's wifi_ap_record_t so the
 * radio layer never leaks into the UI. */
typedef struct {
    uint8_t bssid[6];
    char ssid[33];      /* 32 chars + NUL */
    uint8_t channel;
    int8_t rssi;
    moros_auth_t auth;
} moros_ap_t;

/* Bring up the radio: NVS, netif, event loop, STA mode, start. OFF -> IDLE. */
esp_err_t moros_wifi_init(void);

/* Kick a non-blocking scan. Legal only from IDLE. IDLE -> SCANNING. */
esp_err_t moros_wifi_scan_start(void);

/* Predicate polled by the UI loop each iteration. Returns false while the scan
 * is in flight. On the first observation of completion it pulls the results
 * into the internal array (in the caller's thread, keeping a single reader of
 * that array) and moves back to IDLE; stays true until the next scan_start. */
bool moros_wifi_scan_done(void);

/* Borrow the internal results. Valid until the next moros_wifi_scan_start. */
const moros_ap_t *moros_wifi_get_aps(size_t *count);

/* Promiscuous capture — the planned second radio mode. Foundation only:
 * declared so the UI can be wired against it, not implemented yet. */
esp_err_t moros_wifi_sniffer_start(void);
esp_err_t moros_wifi_sniffer_stop(void);

#endif
