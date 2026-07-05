idf_component_register(
    SRCS "moros_spi.c" "moros_gpio.c" "moros_i2c.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_spi esp_driver_gpio esp_driver_i2c
)
