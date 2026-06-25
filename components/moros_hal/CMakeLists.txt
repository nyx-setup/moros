idf_component_register(
    SRCS "moros_spi.c" "moros_gpio.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_spi esp_driver_gpio
)
