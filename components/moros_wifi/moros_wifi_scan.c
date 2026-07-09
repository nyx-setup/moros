#include "moros_wifi.h"
#include "moros_wifi_priv.h"
#include "esp_wifi.h"
#include <string.h>

static moros_ap_t s_aps[MOROS_WIFI_MAX_APS];
static size_t s_ap_count;

/* Staging buffer for the IDF records before translation. Static rather than on
 * the caller's stack: wifi_ap_record_t is large and the UI task's stack is small. */
static wifi_ap_record_t s_records[MOROS_WIFI_MAX_APS];

/* Written by the event loop task, read by the UI thread. A lone aligned bool,
 * so a plain volatile is enough — no results are touched here. */
static volatile bool s_scan_complete;
static bool s_results_ready;

void moros_wifi_on_scan_done(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE)
        s_scan_complete = true;
}

static moros_auth_t map_auth(wifi_auth_mode_t mode)
{
    switch (mode) {
    case WIFI_AUTH_OPEN:            return MOROS_AUTH_OPEN;
    case WIFI_AUTH_WEP:             return MOROS_AUTH_WEP;
    case WIFI_AUTH_WPA_PSK:         return MOROS_AUTH_WPA;
    case WIFI_AUTH_WPA2_PSK:
    case WIFI_AUTH_WPA_WPA2_PSK:    return MOROS_AUTH_WPA2;
    case WIFI_AUTH_WPA3_PSK:
    case WIFI_AUTH_WPA2_WPA3_PSK:   return MOROS_AUTH_WPA3;
    case WIFI_AUTH_WPA2_ENTERPRISE: return MOROS_AUTH_ENTERPRISE;
    default:                        return MOROS_AUTH_UNKNOWN;
    }
}

static void translate(const wifi_ap_record_t *src, moros_ap_t *dst)
{
    memcpy(dst->bssid, src->bssid, sizeof(dst->bssid));
    memcpy(dst->ssid, src->ssid, sizeof(dst->ssid));
    dst->ssid[sizeof(dst->ssid) - 1] = '\0';
    dst->channel = src->primary;
    dst->rssi = src->rssi;
    dst->auth = map_auth(src->authmode);
}

/* Runs in the UI thread on the scan_done edge, once the radio has finished. */
static void collect_results(void)
{
    uint16_t num = MOROS_WIFI_MAX_APS;
    if (esp_wifi_scan_get_ap_records(&num, s_records) != ESP_OK) {
        s_ap_count = 0;
        return;
    }

    s_ap_count = num;
    for (uint16_t i = 0; i < num; i++)
        translate(&s_records[i], &s_aps[i]);
}

esp_err_t moros_wifi_scan_start(void)
{
    if (moros_wifi_get_state() != MOROS_WIFI_IDLE)
        return ESP_ERR_INVALID_STATE;

    s_scan_complete = false;
    s_results_ready = false;
    s_ap_count = 0;

    /* Passive scan for stealth: listen for beacons instead of sending probe
     * requests that would announce us. show_hidden surfaces cloaked SSIDs. */
    wifi_scan_config_t cfg = {
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
    };
    esp_err_t err = esp_wifi_scan_start(&cfg, false);
    if (err != ESP_OK) return err;

    moros_wifi_set_state(MOROS_WIFI_SCANNING);
    return ESP_OK;
}

bool moros_wifi_scan_done(void)
{
    moros_wifi_state_t state = moros_wifi_get_state();

    if (state == MOROS_WIFI_SCANNING && s_scan_complete) {
        collect_results();
        s_scan_complete = false;
        s_results_ready = true;
        moros_wifi_set_state(MOROS_WIFI_IDLE);
        state = MOROS_WIFI_IDLE;
    }

    return state == MOROS_WIFI_IDLE && s_results_ready;
}

const moros_ap_t *moros_wifi_get_aps(size_t *count)
{
    if (count) *count = s_ap_count;
    return s_aps;
}
