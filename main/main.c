#include "moros_hal.h"
#include "moros_display.h"
#include "moros_font.h"
#include "moros_input.h"
#include "moros_list.h"
#include "moros_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "moros";

#define COLOR_PURPLE  0xA81F
#define COLOR_AMBER   0xFD20
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
    "WiFi", "Sub-GHz", "RFID", "NFC", "Infrared", "Settings",
};

static moros_list_t main_menu = {
    .items = menu_items,
    .count = sizeof(menu_items) / sizeof(menu_items[0]),
    .selected = 0,
    .top = 0,
};

/* AP labels are owned here: the "< Back" guard plus one SSID per AP. The widget
 * borrows these pointers, so they must outlive the screen. Index 0 is "< Back";
 * index i+1 maps to aps[i]. */
static char s_ap_labels[MOROS_WIFI_MAX_APS + 1][34];
static const char *s_ap_items[MOROS_WIFI_MAX_APS + 1];
static moros_list_t ap_list = { .items = s_ap_items };

typedef enum { SCREEN_MENU, SCREEN_AP_LIST, SCREEN_DETAIL } screen_t;

/* Borrowed from the wifi component's internal array; valid until the next scan.
 * We never rescan while the detail screen is open, so it stays live. */
static const moros_ap_t *s_detail_ap;

static const char *auth_name(moros_auth_t auth)
{
    switch (auth) {
    case MOROS_AUTH_OPEN:       return "OPEN";
    case MOROS_AUTH_WEP:        return "WEP";
    case MOROS_AUTH_WPA:        return "WPA";
    case MOROS_AUTH_WPA2:       return "WPA2";
    case MOROS_AUTH_WPA3:       return "WPA3";
    case MOROS_AUTH_ENTERPRISE: return "ENTERPRISE";
    default:                    return "UNKNOWN";
    }
}

/* Security traffic light: red = no/broken crypto, amber = crackable-but-real,
 * green = current. */
static uint16_t auth_color(moros_auth_t auth)
{
    switch (auth) {
    case MOROS_AUTH_OPEN:
    case MOROS_AUTH_WEP:        return MOROS_COLOR_RED;
    case MOROS_AUTH_WPA:
    case MOROS_AUTH_WPA2:       return COLOR_AMBER;
    case MOROS_AUTH_WPA3:
    case MOROS_AUTH_ENTERPRISE: return MOROS_COLOR_GREEN;
    default:                    return MOROS_COLOR_WHITE;
    }
}

static void draw_ap_detail(const moros_ap_t *ap)
{
    char line[32];

    moros_display_fill(MOROS_COLOR_BLACK);

    const char *ssid = ap->ssid[0] ? ap->ssid : "<hidden>";
    moros_draw_text(10, 8, ssid, COLOR_PURPLE, 2);

    snprintf(line, sizeof(line), "%02X:%02X:%02X:%02X:%02X:%02X",
             ap->bssid[0], ap->bssid[1], ap->bssid[2],
             ap->bssid[3], ap->bssid[4], ap->bssid[5]);
    moros_draw_text(10, 45, line, MOROS_COLOR_WHITE, 2);

    snprintf(line, sizeof(line), "Ch %-3d %d dBm", ap->channel, ap->rssi);
    moros_draw_text(10, 78, line, MOROS_COLOR_WHITE, 2);

    moros_draw_text(10, 112, auth_name(ap->auth), auth_color(ap->auth), 3);

    moros_draw_text(10, 152, "click: back", 0x8410, 1);
}

static void status_message(const char *msg)
{
    moros_display_fill(MOROS_COLOR_BLACK);
    moros_draw_text(72, 77, msg, COLOR_PURPLE, 2);
}

static void ap_list_build(void)
{
    size_t n;
    const moros_ap_t *aps = moros_wifi_get_aps(&n);

    strcpy(s_ap_labels[0], "< Back");
    s_ap_items[0] = s_ap_labels[0];

    for (size_t i = 0; i < n; i++) {
        const char *ssid = aps[i].ssid[0] ? aps[i].ssid : "<hidden>";
        snprintf(s_ap_labels[i + 1], sizeof(s_ap_labels[0]), "%s", ssid);
        s_ap_items[i + 1] = s_ap_labels[i + 1];
    }

    ap_list.count = n + 1;
    ap_list.selected = 0;
    ap_list.top = 0;
}

/* Radio stays off until the user opens the WiFi screen (stealth + faster boot);
 * bring it up on first entry. Returns false if the radio failed to start. */
static bool wifi_scan_begin(void)
{
    static bool ready;

    if (!ready) {
        esp_err_t err = moros_wifi_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "wifi init failed: %s", esp_err_to_name(err));
            return false;
        }
        ready = true;
    }

    esp_err_t err = moros_wifi_scan_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan start failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

static void navigation_task(void *arg)
{
    screen_t screen = SCREEN_MENU;
    bool scanning = false;

    moros_display_fill(MOROS_COLOR_BLACK);
    moros_list_draw(&main_menu);

    for (;;) {
        if (scanning && moros_wifi_scan_done()) {
            scanning = false;
            ap_list_build();
            moros_display_fill(MOROS_COLOR_BLACK);
            moros_list_draw(&ap_list);
        }

        moros_input_event_t ev;
        /* Poll fast while a scan is in flight so scan_done runs each iteration;
         * otherwise idle-block waiting on the encoder. */
        if (!moros_input_get(&ev, scanning ? 50 : 1000))
            continue;

        if (ev == MOROS_INPUT_LONG_PRESS) {
            moros_display_fill(MOROS_COLOR_BLACK);
            vTaskDelay(pdMS_TO_TICKS(200));
            moros_power_off();
            continue;
        }

        if (screen == SCREEN_MENU) {
            switch (ev) {
            case MOROS_INPUT_CW:
                moros_list_move(&main_menu, 1);
                break;
            case MOROS_INPUT_CCW:
                moros_list_move(&main_menu, -1);
                break;
            case MOROS_INPUT_CLICK:
                if (strcmp(menu_items[main_menu.selected], "WiFi") == 0) {
                    status_message("Scanning...");
                    if (wifi_scan_begin()) {
                        screen = SCREEN_AP_LIST;
                        scanning = true;
                    } else {
                        status_message("WiFi error");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        moros_display_fill(MOROS_COLOR_BLACK);
                        moros_list_draw(&main_menu);
                    }
                } else {
                    ESP_LOGI(TAG, "enter: %s", menu_items[main_menu.selected]);
                }
                break;
            default:
                break;
            }
            continue;
        }

        if (screen == SCREEN_DETAIL) {
            if (ev == MOROS_INPUT_CLICK) {
                screen = SCREEN_AP_LIST;
                moros_display_fill(MOROS_COLOR_BLACK);
                moros_list_draw(&ap_list);
            }
            continue;
        }

        /* SCREEN_AP_LIST — ignore navigation until the scan has painted. */
        if (scanning)
            continue;

        switch (ev) {
        case MOROS_INPUT_CW:
            moros_list_move(&ap_list, 1);
            break;
        case MOROS_INPUT_CCW:
            moros_list_move(&ap_list, -1);
            break;
        case MOROS_INPUT_CLICK:
            if (ap_list.selected == 0) {
                screen = SCREEN_MENU;
                moros_display_fill(MOROS_COLOR_BLACK);
                moros_list_draw(&main_menu);
            } else {
                size_t n;
                const moros_ap_t *aps = moros_wifi_get_aps(&n);
                s_detail_ap = &aps[ap_list.selected - 1];
                screen = SCREEN_DETAIL;
                draw_ap_detail(s_detail_ap);
            }
            break;
        default:
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

    xTaskCreate(navigation_task, "nav", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Ready");
}
