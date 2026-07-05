#include "moros_hal.h"

esp_err_t moros_gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << MOROS_PIN_LCD_DC)  |
                        (1ULL << MOROS_PIN_LCD_RST) |
                        (1ULL << MOROS_PIN_LCD_BL)  |
                        (1ULL << MOROS_PIN_PWR_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) return ret;

    /* Power on and backlight on */
    gpio_set_level(MOROS_PIN_PWR_EN, 1);
    gpio_set_level(MOROS_PIN_LCD_BL, 1);

    /* KEY button as input with pull-up */
    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << MOROS_PIN_KEY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&btn);
    if (ret != ESP_OK) return ret;

    return ESP_OK;
}

void moros_power_off(void)
{
    gpio_set_level(MOROS_PIN_LCD_BL, 0);
    moros_pmic_shutdown();
    gpio_set_level(MOROS_PIN_PWR_EN, 0);
}
