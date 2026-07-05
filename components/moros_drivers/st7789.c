#include "moros_display.h"
#include "moros_hal.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "st7789";
static spi_device_handle_t lcd_spi;

static void lcd_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(MOROS_PIN_LCD_DC, 0);
    spi_device_transmit(lcd_spi, &t);
}

static void lcd_data(const uint8_t *data, int len)
{
    if (len == 0) return;
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(MOROS_PIN_LCD_DC, 1);
    spi_device_transmit(lcd_spi, &t);
}

static void lcd_data_byte(uint8_t val)
{
    lcd_data(&val, 1);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t col[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    uint8_t row[4] = {(y0+35) >> 8, (y0+35) & 0xFF, (y1+35) >> 8, (y1+35) & 0xFF};

    lcd_cmd(0x2A);
    lcd_data(col, 4);
    lcd_cmd(0x2B);
    lcd_data(row, 4);
    lcd_cmd(0x2C);
}

esp_err_t moros_display_init(void)
{
    spi_device_interface_config_t dev = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = MOROS_PIN_LCD_CS,
        .queue_size = 7,
    };

    esp_err_t ret = spi_bus_add_device(MOROS_SPI_HOST, &dev, &lcd_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add LCD to SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Hardware reset */
    gpio_set_level(MOROS_PIN_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(MOROS_PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_cmd(0x01);                      /* SWRESET */
    vTaskDelay(pdMS_TO_TICKS(150));
    lcd_cmd(0x11);                      /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_cmd(0x3A); lcd_data_byte(0x55); /* COLMOD: RGB565 */
    lcd_cmd(0x36); lcd_data_byte(0xA0); /* MADCTL: orientation */
    lcd_cmd(0x21);                      /* INVON (IPS panel) */
    lcd_cmd(0x29);                      /* DISPON */

    ESP_LOGI(TAG, "Display initialized (%dx%d)", MOROS_LCD_WIDTH, MOROS_LCD_HEIGHT);
    return ESP_OK;
}

void moros_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= MOROS_LCD_WIDTH || y >= MOROS_LCD_HEIGHT) return;
    if (x + w > MOROS_LCD_WIDTH)  w = MOROS_LCD_WIDTH - x;
    if (y + h > MOROS_LCD_HEIGHT) h = MOROS_LCD_HEIGHT - y;

    lcd_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    size_t row_size = w * 2;
    uint8_t *row = heap_caps_malloc(row_size, MALLOC_CAP_DMA);
    if (!row) return;

    for (int i = 0; i < w; i++) {
        row[i * 2]     = hi;
        row[i * 2 + 1] = lo;
    }

    gpio_set_level(MOROS_PIN_LCD_DC, 1);
    for (int j = 0; j < h; j++) {
        spi_transaction_t t = {
            .length = row_size * 8,
            .tx_buffer = row,
        };
        spi_device_transmit(lcd_spi, &t);
    }

    free(row);
}

void moros_display_fill(uint16_t color)
{
    moros_display_fill_rect(0, 0, MOROS_LCD_WIDTH, MOROS_LCD_HEIGHT, color);
}

void moros_display_set_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= MOROS_LCD_WIDTH || y >= MOROS_LCD_HEIGHT) return;

    lcd_set_window(x, y, x, y);
    uint8_t data[2] = {color >> 8, color & 0xFF};
    lcd_data(data, 2);
}
