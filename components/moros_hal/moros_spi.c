#include "moros_hal.h"
#include "esp_log.h"

static const char *TAG = "moros_spi";

esp_err_t moros_spi_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = MOROS_PIN_SPI_MOSI,
        .miso_io_num = MOROS_PIN_SPI_MISO,
        .sclk_io_num = MOROS_PIN_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MOROS_LCD_WIDTH * MOROS_LCD_HEIGHT * 2,
    };

    esp_err_t ret = spi_bus_initialize(MOROS_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
    }
    return ret;
}
