#include "moros_input.h"
#include "moros_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* Quadrature transitions indexed by (prev << 2 | curr), where each 2-bit state
 * is (A << 1 | B). A valid step is +1 or -1; bounces and no-ops are 0. This
 * encoder yields two steps per detent. */
static const int8_t QUAD_LUT[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};

#define STEPS_PER_DETENT 2

static QueueHandle_t s_queue;
static volatile uint8_t s_prev;
static volatile int8_t s_accum;

static inline uint8_t enc_read(void)
{
    return (gpio_get_level(MOROS_PIN_ENC_A) << 1) | gpio_get_level(MOROS_PIN_ENC_B);
}

static void enc_isr(void *arg)
{
    uint8_t curr = enc_read();
    int8_t step = QUAD_LUT[(s_prev << 2) | curr];
    s_prev = curr;
    if (step == 0) return;

    s_accum += step;
    if (s_accum >= STEPS_PER_DETENT || s_accum <= -STEPS_PER_DETENT) {
        moros_input_event_t ev = (s_accum > 0) ? MOROS_INPUT_CW : MOROS_INPUT_CCW;
        s_accum = 0;
        BaseType_t woken = pdFALSE;
        xQueueSendFromISR(s_queue, &ev, &woken);
        if (woken) portYIELD_FROM_ISR();
    }
}

#define KEY_POLL_MS     20
#define KEY_LONG_MS     2000
#define KEY_DEBOUNCE_MS 40

/* The button is polled (not interrupt-driven) so we can time short vs long
 * press. Released before KEY_LONG_MS is a CLICK; held past it is a LONG_PRESS. */
static void key_task(void *arg)
{
    int held = 0;
    bool down_prev = false;
    bool long_sent = false;

    for (;;) {
        bool down = (gpio_get_level(MOROS_PIN_KEY) == 0);

        if (down) {
            held += KEY_POLL_MS;
            if (!long_sent && held >= KEY_LONG_MS) {
                moros_input_event_t ev = MOROS_INPUT_LONG_PRESS;
                xQueueSend(s_queue, &ev, 0);
                long_sent = true;
            }
        } else {
            if (down_prev && !long_sent && held >= KEY_DEBOUNCE_MS) {
                moros_input_event_t ev = MOROS_INPUT_CLICK;
                xQueueSend(s_queue, &ev, 0);
            }
            held = 0;
            long_sent = false;
        }

        down_prev = down;
        vTaskDelay(pdMS_TO_TICKS(KEY_POLL_MS));
    }
}

esp_err_t moros_input_init(void)
{
    s_queue = xQueueCreate(8, sizeof(moros_input_event_t));
    if (!s_queue) return ESP_ERR_NO_MEM;

    gpio_config_t enc_cfg = {
        .pin_bit_mask = (1ULL << MOROS_PIN_ENC_A) | (1ULL << MOROS_PIN_ENC_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    esp_err_t ret = gpio_config(&enc_cfg);
    if (ret != ESP_OK) return ret;

    /* GPIO 0 is a strapping pin (see moros-docs/hardware/power.md). */
    gpio_config_t key_cfg = {
        .pin_bit_mask = (1ULL << MOROS_PIN_KEY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&key_cfg);
    if (ret != ESP_OK) return ret;

    s_prev = enc_read();

    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret;

    gpio_isr_handler_add(MOROS_PIN_ENC_A, enc_isr, NULL);
    gpio_isr_handler_add(MOROS_PIN_ENC_B, enc_isr, NULL);

    xTaskCreate(key_task, "key", 2048, NULL, 5, NULL);
    return ESP_OK;
}

bool moros_input_get(moros_input_event_t *event, uint32_t timeout_ms)
{
    return xQueueReceive(s_queue, event, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
