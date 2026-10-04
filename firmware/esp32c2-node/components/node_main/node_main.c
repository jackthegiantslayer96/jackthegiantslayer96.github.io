/*
 * The wake cycle: sample -> connect -> publish -> sleep.
 *
 * Deep sleep discards RAM and stops the radio, so this is a one-shot cycle
 * rather than a daemon. app_main() runs once per wake and never returns.
 *
 * Failure policy, decided here rather than discovered in the field:
 *   - sensor read failure  -> log and sleep. Publishing nothing beats spamming a
 *                             broken endpoint with bad data.
 *   - Wi-Fi failure       -> sleep, and let the retained failure counter push
 *                             the interval out. Long outages are normal, not
 *                             exceptional.
 *   - publish failure     -> retry a few times, then sleep long. Without this
 *                             a dead server turns the node into a hot loop
 *                             that drains the battery.
 */
#include <inttypes.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "node_config.h"
#include "node_hal.h"
#include "node_sensor.h"
#include "node_transport.h"

static const char *TAG = "node_main";

static void log_boot_context(void)
{
    node_hal_state_t *st = node_hal_state();
    /* Bitmap form: a timer wake and a GPIO wake can both fire on the same wake. */
    uint32_t causes = esp_sleep_get_wakeup_causes();

    st->boot_count++;
    node_hal_state_commit();

    ESP_LOGI(TAG, "wake #%" PRIu32 " causes: 0x%08" PRIx32, st->boot_count, causes);
    ESP_LOGI(TAG, "sensor='%s' transport='%s'", node_sensor_active_name(),
             node_transport_active_name());

    if (st->publish_fail_streak > 0) {
        ESP_LOGW(TAG, "carrying over a fail streak of %" PRIu32, st->publish_fail_streak);
    }
}

/*
 * Decides how long this cycle should sleep. The failure streak, which lives in
 * RTC memory, is what prevents a long outage from turning into a battery drain.
 */
static uint64_t choose_sleep_interval(const node_hal_state_t *st)
{
    if (st->publish_fail_streak >= NODE_MAX_PUBLISH_ATTEMPTS) {
        ESP_LOGW(TAG, "in backoff: sleeping %llu us until the endpoint recovers",
                 (unsigned long long)NODE_BACKOFF_INTERVAL_US);
        return NODE_BACKOFF_INTERVAL_US;
    }

    return NODE_SLEEP_INTERVAL_US;
}

static void sleep_and_never_return(uint64_t interval_us)
{
    node_hal_led_set(false);

#if NODE_BRINGUP_HOLD_MS > 0
    /*
     * Stay awake without deep-sleeping so the USB console (and any in-flight
     * esptool transaction) survives. Deep sleep powers the native USB
     * peripheral down, so on the C3 it takes the serial connection with it.
     */
    ESP_LOGW(TAG, "bring-up hold: staying awake for %d ms (deep sleep disabled)",
             NODE_BRINGUP_HOLD_MS);
    vTaskDelay(pdMS_TO_TICKS(NODE_BRINGUP_HOLD_MS));
    ESP_LOGI(TAG, "bring-up hold over, resuming the normal sleep cycle");
#endif

    node_hal_deep_sleep(interval_us);
}

void app_main(void)
{
    const uint64_t awake_start_us = esp_timer_get_time();

    node_hal_led_init();
    node_hal_led_set(true);

    log_boot_context();

    /* ---- sample ------------------------------------------------- */

    node_readings_t readings = {0};

    esp_err_t err = node_sensor_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensor init failed, skipping this cycle's publish");
        node_hal_state_commit();
        sleep_and_never_return(choose_sleep_interval(node_hal_state()));
        return; /* unreachable */
    }

    err = node_sensor_read(&readings);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensor read failed: %s", esp_err_to_name(err));
        node_sensor_deinit();
        node_hal_state_commit();
        sleep_and_never_return(choose_sleep_interval(node_hal_state()));
        return;
    }

    ESP_LOGI(TAG, "sample taken: %s sensor=%s", node_sensor_active_name(),
             (readings.valid_mask & NODE_FIELD_RAW_MV) ? "adc mV" : "i2c");

    node_sensor_deinit();

    /* ---- connect + publish -------------------------------------- */

    err = node_transport_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "transport init failed: %s", esp_err_to_name(err));

        node_hal_state_t *st = node_hal_state();
        st->publish_fail_streak++;
        node_hal_state_commit();

        node_transport_deinit();
        sleep_and_never_return(choose_sleep_interval(node_hal_state()));
        return;
    }

    bool published = false;

    for (int attempt = 1; attempt <= NODE_MAX_PUBLISH_ATTEMPTS; attempt++) {
        err = node_transport_publish(&readings);
        if (err == ESP_OK) {
            published = true;
            break;
        }

        ESP_LOGW(TAG, "publish attempt %d/%d failed: %s", attempt, NODE_MAX_PUBLISH_ATTEMPTS,
                 esp_err_to_name(err));

        if (attempt < NODE_MAX_PUBLISH_ATTEMPTS) {
            vTaskDelay(pdMS_TO_TICKS(NODE_PUBLISH_RETRY_DELAY_MS));
        }
    }

    /* Stops Wi-Fi, which must happen before deep sleep. */
    node_transport_deinit();

    node_hal_state_t *st = node_hal_state();

    if (published) {
        st->publish_fail_streak = 0;
    } else {
        st->publish_fail_streak++;
    }

    st->uptime_total_us += (uint64_t)(esp_timer_get_time() - awake_start_us);
    node_hal_state_commit();

    ESP_LOGI(TAG, "cycle done in %lld ms (fail streak %" PRIu32 ")",
             (long long)((esp_timer_get_time() - awake_start_us) / 1000), st->publish_fail_streak);

    /* ---- sleep -------------------------------------------------- */

    sleep_and_never_return(choose_sleep_interval(node_hal_state()));
}