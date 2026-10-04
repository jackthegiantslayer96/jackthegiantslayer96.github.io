/*
 * Default bring-up transport: prints the payload to the serial console and
 * touches no network at all.
 *
 * This is the highest-value driver to get working first because it validates
 * the sensor, the HAL and the whole cycle with zero network, zero credentials
 * and zero server involvement.
 */
#include <string.h>

#include "esp_log.h"
#include "node_config.h"
#include "node_transport.h"
#include "node_transport_internal.h"

static const char *TAG = "publish:serial";

static esp_err_t serial_init(void)
{
    ESP_LOGI(TAG, "serial transport ready (no network used)");
    return ESP_OK;
}

static esp_err_t serial_publish(const node_readings_t *readings)
{
    char payload[NODE_TRANSPORT_PAYLOAD_MAX];

    node_transport_build_payload(readings, payload, sizeof(payload));

    /* Printed unescaped and unformatted: it is a debug line, not the wire format. */
    ESP_LOGI(TAG, "telemetry: %s", payload);
    return ESP_OK;
}

static void serial_deinit(void)
{
    /* Nothing to tear down. */
}

const node_transport_t *node_transport_serial_log_get_driver(void)
{
    static const node_transport_t driver = {
        .init = serial_init,
        .publish = serial_publish,
        .deinit = serial_deinit,
    };
    return &driver;
}