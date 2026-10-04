#include "node_sensor.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "node_config.h"

static const char *TAG = "node_sensor";

/*
 * Drivers are linked in unconditionally so switching NODE_SENSOR needs only a
 * reconfigure and a rebuild, with no CMake edits.
 */
static const node_sensor_t *active_driver(void)
{
    switch (NODE_SENSOR) {
    case NODE_SENSOR_AHT21:
        return node_sensor_aht21_get_driver();
    case NODE_SENSOR_ANALOG_RAW:
    default:
        return node_sensor_analog_raw_get_driver();
    }
}

const char *node_sensor_active_name(void)
{
    switch (NODE_SENSOR) {
    case NODE_SENSOR_AHT21:
        return "aht21";
    case NODE_SENSOR_ANALOG_RAW:
    default:
        return "analog_raw";
    }
}

esp_err_t node_sensor_init(void)
{
    const node_sensor_t *drv = active_driver();
    if (drv == NULL || drv->init == NULL) {
        ESP_LOGE(TAG, "sensor driver table is incomplete");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "initialising sensor driver '%s'", node_sensor_active_name());

    esp_err_t err = drv->init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensor '%s' init failed: %s", node_sensor_active_name(), esp_err_to_name(err));
    }
    return err;
}

esp_err_t node_sensor_read(node_readings_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    const node_sensor_t *drv = active_driver();
    if (drv == NULL || drv->read == NULL) {
        return ESP_FAIL;
    }

    return drv->read(out);
}

void node_sensor_deinit(void)
{
    const node_sensor_t *drv = active_driver();
    if (drv != NULL && drv->deinit != NULL) {
        drv->deinit();
    }
}

void node_sensor_readings_to_json(const node_readings_t *r, char *buf, size_t buf_len)
{
    if (r == NULL || buf == NULL || buf_len == 0) {
        return;
    }

    buf[0] = '\0';

    if (r->valid_mask & NODE_FIELD_TEMP_C) {
        /* Concatenated rather than snprintf-per-field to keep one allocation
         * off the heap on a 272KB-SRAM part. */
        snprintf(buf, buf_len, "\"temp_c\":%.2f", (double)r->temp_c);
    }

    if (r->valid_mask & NODE_FIELD_HUMIDITY_PCT) {
        size_t used = strlen(buf);
        snprintf(buf + used, buf_len - used, "%s\"humidity_pct\":%.2f",
                 used > 0 ? "," : "", (double)r->humidity_pct);
    }

    if (r->valid_mask & NODE_FIELD_RAW_MV) {
        size_t used = strlen(buf);
        snprintf(buf + used, buf_len - used, "%s\"raw_mv\":%ld", used > 0 ? "," : "",
                 (long)r->raw_mv);
    }

    if (buf[0] == '\0') {
        snprintf(buf, buf_len, "\"sensor\":null");
    }
}