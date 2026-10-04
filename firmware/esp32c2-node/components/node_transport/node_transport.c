#include "node_transport.h"

#include <inttypes.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "node_config.h"
#include "node_hal.h"
#include "node_sensor.h"

static const char *TAG = "node_transport";

static const node_transport_t *active_driver(void)
{
    switch (NODE_TRANSPORT) {
    case NODE_TRANSPORT_HTTP:
        return node_transport_http_post_get_driver();
    case NODE_TRANSPORT_MQTT:
        return node_transport_mqtt_get_driver();
    case NODE_TRANSPORT_SERIAL:
    default:
        return node_transport_serial_log_get_driver();
    }
}

const char *node_transport_active_name(void)
{
    switch (NODE_TRANSPORT) {
    case NODE_TRANSPORT_HTTP:
        return "http_post";
    case NODE_TRANSPORT_MQTT:
        return "mqtt";
    case NODE_TRANSPORT_SERIAL:
    default:
        return "serial_log";
    }
}

esp_err_t node_transport_init(void)
{
    const node_transport_t *drv = active_driver();
    ESP_LOGI(TAG, "using transport '%s'", node_transport_active_name());

    if (drv == NULL || drv->init == NULL) {
        return ESP_FAIL;
    }

    esp_err_t err = drv->init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "transport '%s' init failed: %s", node_transport_active_name(),
                 esp_err_to_name(err));
    }
    return err;
}

esp_err_t node_transport_publish(const node_readings_t *readings)
{
    if (readings == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const node_transport_t *drv = active_driver();
    if (drv == NULL || drv->publish == NULL) {
        return ESP_FAIL;
    }

    return drv->publish(readings);
}

void node_transport_deinit(void)
{
    const node_transport_t *drv = active_driver();
    if (drv != NULL && drv->deinit != NULL) {
        drv->deinit();
    }
}

/*
 * Shared by the serial and HTTP drivers: assembles the JSON payload.
 *
 * Written by hand rather than with cJSON because the payload is a flat object
 * of scalars and cJSON would cost several KB of flash plus a heap allocation
 * for no benefit on a 2MB part.
 */
void node_transport_build_payload(const node_readings_t *r, char *buf, size_t buf_len);
void node_transport_build_payload(const node_readings_t *r, char *buf, size_t buf_len)
{
    char fields[192];
    node_sensor_readings_to_json(r, fields, sizeof(fields));

    const node_hal_state_t *st = node_hal_state();

    /* Cast rather than rely on PRIu64: on this target uint64_t is not
     * unsigned long long, so the inttypes macro would emit a mismatched
     * conversion and trip -Wformat under -Werror. */
    snprintf(buf, buf_len,
             "{\"device\":\"esp32c3-node\",\"boot\":%lu,\"fail_streak\":%lu"
             ",\"uptime_us\":%llu,\"%s\"}",
             (unsigned long)st->boot_count, (unsigned long)st->publish_fail_streak,
             (unsigned long long)st->uptime_total_us, fields);
}