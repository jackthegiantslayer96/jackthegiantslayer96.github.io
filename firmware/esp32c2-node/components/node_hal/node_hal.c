#include "node_hal.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "node_config.h"

static const char *TAG = "node_hal";

#define NODE_STATE_MAGIC 0x4E4F4445u /* "NODE" */
#define NODE_STATE_VERSION 1u

/* Retained across deep sleep; RAM is discarded, RTC slow memory is not. */
static RTC_DATA_ATTR node_hal_state_t s_state;
static bool s_state_valid;
static bool s_sensor_wakeup_armed;
static bool s_sensor_wakeup_active_low;

/* ------------------------------------------------------------------ */
/* Checksum                                                            */
/* ------------------------------------------------------------------ */

static uint32_t fletcher32(const uint8_t *data, size_t len)
{
    uint32_t sum1 = 0xFFFF;
    uint32_t sum2 = 0xFFFF;

    while (len > 0) {
        size_t block = len < 360 ? len : 360;
        len -= block;

        for (size_t i = 0; i < block; i++) {
            sum1 += data[i];
            sum2 += sum1;
        }

        sum1 = (sum1 & 0xFFFF) + (sum1 >> 16);
        sum2 = (sum2 & 0xFFFF) + (sum2 >> 16);
    }

    sum1 = (sum1 & 0xFFFF) + (sum1 >> 16);
    sum2 = (sum2 & 0xFFFF) + (sum2 >> 16);

    return (sum2 << 16) | sum1;
}

/*
 * The checksum covers everything except itself, so a partially written or
 * corrupted block is rejected instead of silently driving the backoff logic
 * with garbage failure counts.
 */
static uint32_t state_checksum(const node_hal_state_t *s)
{
    node_hal_state_t copy = *s;

    /* The checksum field must read as zero while the other fields are summed. */
    copy.checksum = 0;
    return fletcher32((const uint8_t *)&copy, sizeof(copy));
}

/* ------------------------------------------------------------------ */
/* Retained state                                                      */
/* ------------------------------------------------------------------ */

node_hal_state_t *node_hal_state(void)
{
    if (!s_state_valid) {
        bool reset_needed = (s_state.magic != NODE_STATE_MAGIC) ||
                            (s_state.version != NODE_STATE_VERSION) ||
                            (s_state.checksum != state_checksum(&s_state));

        if (reset_needed) {
            /* First ever boot, or the block did not survive power-on. */
            memset(&s_state, 0, sizeof(s_state));
            s_state.magic = NODE_STATE_MAGIC;
            s_state.version = NODE_STATE_VERSION;
            s_state.checksum = state_checksum(&s_state);
        }

        s_state_valid = true;
    }

    return &s_state;
}

void node_hal_state_commit(void)
{
    s_state.magic = NODE_STATE_MAGIC;
    s_state.version = NODE_STATE_VERSION;
    s_state.checksum = state_checksum(&s_state);
}

/* ------------------------------------------------------------------ */
/* LED                                                                 */
/* ------------------------------------------------------------------ */

void node_hal_led_init(void)
{
#if NODE_LED_ENABLED
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << NODE_PIN_LED,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    node_hal_led_set(false);
#endif
}

void node_hal_led_set(bool on)
{
#if NODE_LED_ENABLED
    gpio_set_level(NODE_PIN_LED, on ? 1 : 0);
#else
    (void)on;
#endif
}

/* ------------------------------------------------------------------ */
/* Deep sleep                                                          */
/* ------------------------------------------------------------------ */

void node_hal_enable_sensor_wakeup(bool active_low)
{
    s_sensor_wakeup_armed = true;
    s_sensor_wakeup_active_low = active_low;
}

void node_hal_deep_sleep(uint64_t sleep_us)
{
    node_hal_state_t *st = node_hal_state();
    /*
     * esp_sleep_get_wakeup_cause() is deprecated in favour of the bitmap form,
     * which reports every source that fired rather than only one - relevant
     * here because a timer wake and a GPIO wake can land on the same instant.
     */
    st->last_wakeup_cause = (uint32_t)esp_sleep_get_wakeup_causes();
    node_hal_state_commit();

    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_us));

    if (s_sensor_wakeup_armed) {
        /*
         * On the C2, deep-sleep GPIO wakeup only accepts RTC IOs. The pin is
         * guaranteed <= 5 by the _Static_assert block in node_config.h, so
         * this is the C2-correct API rather than the legacy esp_sleep_ext1
         * variant, which does not exist on this chip.
         */
        esp_sleep_gpio_wake_up_mode_t mode = s_sensor_wakeup_active_low ? ESP_GPIO_WAKEUP_GPIO_LOW
                                                                        : ESP_GPIO_WAKEUP_GPIO_HIGH;
        uint64_t mask = 1ULL << NODE_PIN_SENSOR_INT;

        esp_err_t err = esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown(mask, mode);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "sensor GPIO wakeup on pin %d failed (%s); timer wake only",
                     NODE_PIN_SENSOR_INT, esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "sensor GPIO wakeup armed on pin %d (%s)", NODE_PIN_SENSOR_INT,
                     s_sensor_wakeup_active_low ? "active low" : "active high");
        }
    }

    ESP_LOGI(TAG, "entering deep sleep for %llu us", (unsigned long long)sleep_us);

    /* Flushes the UART FIFO and never returns. */
    esp_deep_sleep_start();
}