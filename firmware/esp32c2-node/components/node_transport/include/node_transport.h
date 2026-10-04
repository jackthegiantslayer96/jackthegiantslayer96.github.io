/*
 * Pluggable publish transport. Same dispatcher shape as the sensor layer.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "node_sensor.h"

typedef struct {
    esp_err_t (*init)(void);

    /*
     * Publishes a reading. The payload is a JSON object body, built by the
     * transport itself from the readings plus device metadata.
     *
     * Returns ESP_OK only on confirmed delivery (HTTP 2xx, serial write ok).
     * Anything else must be reported as a failure so the caller can escalate
     * the backoff interval.
     */
    esp_err_t (*publish)(const node_readings_t *readings);

    /* Must tear the network down; deep sleep needs the radio stopped. */
    void (*deinit)(void);
} node_transport_t;

const node_transport_t *node_transport_serial_log_get_driver(void);
const node_transport_t *node_transport_http_post_get_driver(void);
const node_transport_t *node_transport_mqtt_get_driver(void);

const char *node_transport_active_name(void);

esp_err_t node_transport_init(void);
esp_err_t node_transport_publish(const node_readings_t *readings);
void node_transport_deinit(void);