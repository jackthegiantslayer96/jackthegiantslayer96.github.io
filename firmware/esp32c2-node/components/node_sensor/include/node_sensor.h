/*
 * Pluggable sensor interface.
 *
 * Adding a driver means: write the three functions, expose them via
 * node_sensor_<name>_get_driver(), and add one entry to the table in
 * node_sensor.c. Nothing else in the codebase changes.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/* Which fields of node_readings_t are meaningful for a given reading. */
typedef enum {
    NODE_FIELD_TEMP_C = 1 << 0,
    NODE_FIELD_HUMIDITY_PCT = 1 << 1,
    NODE_FIELD_RAW_MV = 1 << 2,
} node_field_t;

typedef struct {
    uint32_t valid_mask; /* bitwise OR of node_field_t */
    float temp_c;
    float humidity_pct;
    int32_t raw_mv;
} node_readings_t;

typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*read)(node_readings_t *out);
    void (*deinit)(void);
} node_sensor_t;

/* Driver constructors. Exactly one is compiled in per build. */
const node_sensor_t *node_sensor_analog_raw_get_driver(void);
const node_sensor_t *node_sensor_aht21_get_driver(void);

/* Name of the driver selected by NODE_SENSOR, for logging. */
const char *node_sensor_active_name(void);

/* Dispatches to the driver selected at compile time. */
esp_err_t node_sensor_init(void);
esp_err_t node_sensor_read(node_readings_t *out);
void node_sensor_deinit(void);

/* Renders readings as a single-line JSON object. Buffer must be >= 192 bytes. */
void node_sensor_readings_to_json(const node_readings_t *r, char *buf, size_t buf_len);