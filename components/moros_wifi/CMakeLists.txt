idf_component_register(
    SRCS "moros_wifi.c" "moros_wifi_scan.c" "moros_wifi_sniffer.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp_event nvs_flash
)
