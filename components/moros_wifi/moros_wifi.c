#include "moros_wifi.h"
#include "moros_wifi_priv.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

static moros_wifi_state_t s_state = MOROS_WIFI_OFF;

moros_wifi_state_t moros_wifi_get_state(void)
{
    return s_state;
}

void moros_wifi_set_state(moros_wifi_state_t state)
{
    s_state = state;
}

esp_err_t moros_wifi_init(void)
{
    if (s_state != MOROS_WIFI_OFF)
        return ESP_ERR_INVALID_STATE;

    /* NVS first: the radio reads its RF calibration data from there. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) return err;
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    err = esp_netif_init();
    if (err != ESP_OK) return err;

    err = esp_event_loop_create_default();
    if (err != ESP_OK) return err;

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;

    err = esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                              moros_wifi_on_scan_done, NULL, NULL);
    if (err != ESP_OK) return err;

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return err;

    err = esp_wifi_start();
    if (err != ESP_OK) return err;

    s_state = MOROS_WIFI_IDLE;
    return ESP_OK;
}
