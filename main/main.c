#include "moros_hal.h"
#include "moros_display.h"
#include "moros_font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "moros";

#define COLOR_PURPLE  0xA81F
#define MOROS_X       80
#define MOROS_Y       55
#define VERSION_X     112
#define VERSION_Y     100

static void draw_cursor(uint16_t x, uint16_t y, uint16_t color)
{
    moros_draw_char(x, y, '_', color, 4);
}

static void moros_splash(void)
{
    const char *title = "MOROS";

    for (int i = 0; i < 3; i++) {
        draw_cursor(MOROS_X, MOROS_Y, COLOR_PURPLE);
        vTaskDelay(pdMS_TO_TICKS(400));
        draw_cursor(MOROS_X, MOROS_Y, MOROS_COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(400));
    }

    for (int i = 0; title[i] != '\0'; i++) {
        moros_draw_char(MOROS_X + i * 8 * 4, MOROS_Y, title[i], COLOR_PURPLE, 4);
        draw_cursor(MOROS_X + (i + 1) * 8 * 4, MOROS_Y, COLOR_PURPLE);
        vTaskDelay(pdMS_TO_TICKS(150));
        draw_cursor(MOROS_X + (i + 1) * 8 * 4, MOROS_Y, MOROS_COLOR_BLACK);
    }

    for (int i = 0; i < 4; i++) {
        draw_cursor(MOROS_X + 5 * 8 * 4, MOROS_Y, COLOR_PURPLE);
        vTaskDelay(pdMS_TO_TICKS(400));
        draw_cursor(MOROS_X + 5 * 8 * 4, MOROS_Y, MOROS_COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(400));
    }

    moros_draw_text(VERSION_X, VERSION_Y, "v0.1.0", COLOR_PURPLE, 2);
}

/* KEY is the encoder push on GPIO 0, an ESP32-S3 strapping pin. Holding it LOW
 * at power-on/reset makes the boot ROM enter serial download mode instead of
 * running the firmware, so the board appears "dead". This is decided in ROM
 * before app_main runs and cannot be worked around in software. This task is
 * also created only after the splash, so the button isn't watched during boot. */
static void power_watch_task(void *arg)
{
    int held_ms = 0;

    for (;;) {
        if (gpio_get_level(MOROS_PIN_KEY) == 0) {
            held_ms += 50;
            if (held_ms >= 2000) {
                ESP_LOGI(TAG, "Power off");
                moros_display_fill(MOROS_COLOR_BLACK);
                vTaskDelay(pdMS_TO_TICKS(200));
                moros_power_off();
            }
        } else {
            held_ms = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "MOROS starting...");

    ESP_ERROR_CHECK(moros_gpio_init());
    ESP_ERROR_CHECK(moros_i2c_init());
    ESP_ERROR_CHECK(moros_spi_init());
    ESP_ERROR_CHECK(moros_display_init());

    moros_display_fill(MOROS_COLOR_BLACK);
    moros_splash();

    xTaskCreate(power_watch_task, "pwr", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Ready");
}
