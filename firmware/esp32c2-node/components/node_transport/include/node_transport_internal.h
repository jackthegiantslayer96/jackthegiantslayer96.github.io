/* Private to the node_transport component; not part of the public API. */
#pragma once

#include <stddef.h>

#include "node_sensor.h"

/*
 * Assembles the flat JSON payload for a reading, including device metadata.
 * buf must be at least NODE_TRANSPORT_PAYLOAD_MAX bytes.
 */
#define NODE_TRANSPORT_PAYLOAD_MAX 320

void node_transport_build_payload(const node_readings_t *r, char *buf, size_t buf_len);