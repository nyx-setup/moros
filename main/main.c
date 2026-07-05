#include "moros_hal.h"
#include "moros_display.h"
#include "moros_font.h"
#include "moros_input.h"
#include "menu.h"
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

    moros_draw_text(VERSION_X, VERSION_Y, "v0.2.0", COLOR_PURPLE, 2);
}

static const char *const menu_items[] = {
    "Sub-GHz", "RFID", "NFC", "Infrared", "Settings",
};

static moros_menu_t main_menu = {
    .items = menu_items,
    .count = sizeof(menu_items) / sizeof(menu_items[0]),
    .selected = 0,
};

static void navigation_task(void *arg)
{
    moros_display_fill(MOROS_COLOR_BLACK);
    moros_menu_draw(&main_menu);

    moros_input_event_t ev;
    for (;;) {
        if (!moros_input_get(&ev, portMAX_DELAY)) continue;

        switch (ev) {
        case MOROS_INPUT_CW:
            moros_menu_move(&main_menu, 1);
            break;
        case MOROS_INPUT_CCW:
            moros_menu_move(&main_menu, -1);
            break;
        case MOROS_INPUT_CLICK:
            ESP_LOGI(TAG, "enter: %s", menu_items[main_menu.selected]);
            break;
        case MOROS_INPUT_LONG_PRESS:
            moros_display_fill(MOROS_COLOR_BLACK);
            vTaskDelay(pdMS_TO_TICKS(200));
            moros_power_off();
            break;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "MOROS starting...");

    ESP_ERROR_CHECK(moros_gpio_init());
    ESP_ERROR_CHECK(moros_i2c_init());
    ESP_ERROR_CHECK(moros_spi_init());
    ESP_ERROR_CHECK(moros_display_init());
    ESP_ERROR_CHECK(moros_input_init());

    moros_display_fill(MOROS_COLOR_BLACK);
    moros_splash();

    xTaskCreate(navigation_task, "nav", 3072, NULL, 5, NULL);

    ESP_LOGI(TAG, "Ready");
}
