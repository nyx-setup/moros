#include "moros_hal.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "moros_i2c";

/* BQ25896 REG09 bit5 disables the BATFET, disconnecting the battery from the
 * system rail. Releasing the GPIO 15 latch alone won't power the board down —
 * the PMIC keeps the rail alive until this bit is set. */
#define BQ25896_REG09        0x09
#define BQ25896_BATFET_DIS   (1 << 5)

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_pmic;

esp_err_t moros_i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = MOROS_PIN_I2C_SDA,
        .scl_io_num = MOROS_PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MOROS_PMIC_ADDR,
        .scl_speed_hz = 100000,
    };
    ret = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_pmic);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMIC add failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t moros_pmic_shutdown(void)
{
    if (!s_pmic) return ESP_ERR_INVALID_STATE;

    uint8_t reg = BQ25896_REG09;
    uint8_t val = 0;
    esp_err_t ret = i2c_master_transmit_receive(s_pmic, &reg, 1, &val, 1, 100);
    if (ret != ESP_OK) return ret;

    uint8_t out[2] = { BQ25896_REG09, (uint8_t)(val | BQ25896_BATFET_DIS) };
    return i2c_master_transmit(s_pmic, out, 2, 100);
}
