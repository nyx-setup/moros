#ifndef MOROS_HAL_H
#define MOROS_HAL_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

/* Pin mapping: T-Embed CC1101 Plus (K268-01)
 * Reference: moros-docs/hardware/pinout.md */

/* SPI bus (shared: LCD + CC1101, active CS selects device) */
#define MOROS_SPI_HOST      SPI2_HOST
#define MOROS_PIN_SPI_CLK   GPIO_NUM_11
#define MOROS_PIN_SPI_MOSI  GPIO_NUM_9
#define MOROS_PIN_SPI_MISO  GPIO_NUM_10

/* LCD */
#define MOROS_PIN_LCD_CS    GPIO_NUM_41
#define MOROS_PIN_LCD_DC    GPIO_NUM_16
#define MOROS_PIN_LCD_RST   GPIO_NUM_40  /* shared with I2S LRCLK */
#define MOROS_PIN_LCD_BL    GPIO_NUM_21

#define MOROS_LCD_WIDTH     320
#define MOROS_LCD_HEIGHT    170

/* Power */
#define MOROS_PIN_PWR_EN    GPIO_NUM_15  /* latch only; real cut is via the PMIC */

/* I2C: BQ25896 PMIC (0x6B) + BQ27220 fuel gauge (0x55) */
#define MOROS_PIN_I2C_SDA   GPIO_NUM_8
#define MOROS_PIN_I2C_SCL   GPIO_NUM_18
#define MOROS_PMIC_ADDR     0x6B

/* Input */
#define MOROS_PIN_KEY       GPIO_NUM_0  /* encoder push, strapping pin */

void moros_power_off(void);

esp_err_t moros_spi_init(void);
esp_err_t moros_gpio_init(void);
esp_err_t moros_i2c_init(void);
esp_err_t moros_pmic_shutdown(void);

#endif
