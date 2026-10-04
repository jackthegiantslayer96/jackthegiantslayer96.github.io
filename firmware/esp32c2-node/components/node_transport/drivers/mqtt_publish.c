/*
 * MQTT transport - PHASE 2 STUB, not implemented.
 *
 * Deliberately left unimplemented rather than half-built, because the deep-sleep
 * cycle interacts badly with MQTT in a way that needs a decision, not a driver:
 *
 *   - The broker sees the connection drop every cycle. With a normal keepalive
 *     the session dies and each wake is a full reconnect with CONNACK and a
 *     will message. Persistent sessions plus a last-will set to "offline" is the
 *     usual answer.
 *   - A sensor that sleeps for five minutes and holds a TCP session open between
 *     readings is strictly worse than one that connects, publishes and leaves,
 *     unless readings become frequent enough that reconnect cost dominates.
 *   - Queued offline messages need bounded storage in RAM, since the RTC slow
 *   memory is tiny and publishing stale readings from hours ago is rarely what
 *     you want anyway.
 *
 * Publishing with a short-lived session (clean session, short keepalive, no LWT)
 * sidesteps all of it and is what this stub will do when implemented.
 */
#include "esp_log.h"
#include "node_config.h"
#include "node_transport.h"
#include "node_transport_internal.h"

static const char *TAG = "publish:mqtt";

static esp_err_t mqtt_init(void)
{
    ESP_LOGE(TAG, "MQTT transport is a phase 2 stub and is not implemented");
    ESP_LOGE(TAG, "set NODE_TRANSPORT to NODE_TRANSPORT_HTTP or NODE_TRANSPORT_SERIAL");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t mqtt_publish(const node_readings_t *readings)
{
    (void)readings;
    return ESP_ERR_NOT_SUPPORTED;
}

static void mqtt_deinit(void)
{
}

const node_transport_t *node_transport_mqtt_get_driver(void)
{
    static const node_transport_t driver = {
        .init = mqtt_init,
        .publish = mqtt_publish,
        .deinit = mqtt_deinit,
    };
    return &driver;
}