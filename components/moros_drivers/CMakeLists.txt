idf_component_register(
    SRCS "st7789.c" "moros_font.c"
    INCLUDE_DIRS "include"
    REQUIRES moros_hal esp_driver_spi esp_driver_gpio freertos heap
)
