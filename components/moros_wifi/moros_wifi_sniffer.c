#include "moros_wifi.h"
#include "moros_wifi_priv.h"

/* Promiscuous capture, deliberately unimplemented. The discovery state machine
 * already reserves MOROS_WIFI_SNIFFING, so the eventual wiring is local: start
 * moves IDLE -> SNIFFING and installs a promiscuous RX callback; stop tears it
 * down and returns to IDLE. Until then these refuse rather than corrupt the
 * radio state. */
esp_err_t moros_wifi_sniffer_start(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t moros_wifi_sniffer_stop(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
