/*
 * Hardware abstraction: pin helpers, deep-sleep entry, and state that must
 * survive across sleep cycles.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_sleep.h"

/*
 * State persisted in RTC slow memory. It survives deep sleep but NOT a full
 * power cycle, so every field is treated as untrusted input and validated by
 * node_hal_state_init().
 *
 * Must contain no pointers. RTC memory is a fixed static region, so a pointer
 * written by a previous boot is stale garbage after reset, and dereferencing it
 * is a silent memory-corruption bug rather than an obvious crash.
 */
typedef struct {
    uint32_t magic;                  /* NODE_STATE_MAGIC once validated            */
    uint32_t version;                /* bump when the layout changes               */
    uint32_t boot_count;             /* wake cycles since last power-on            */
    uint32_t publish_fail_streak;    /* consecutive failures, resets on success    */
    uint32_t last_wakeup_cause;      /* esp_sleep_wakeup_cause_t of the last wake  */
    uint64_t uptime_total_us;        /* accumulated awake time across all cycles   */
    uint32_t checksum;               /* Fletcher-32 over the fields above          */
} node_hal_state_t;

/* Validates magic/version/checksum and resets the struct if anything is off. */
node_hal_state_t *node_hal_state(void);

/* Recomputes the checksum. Must be called after any mutation. */
void node_hal_state_commit(void);

void node_hal_led_init(void);
void node_hal_led_set(bool on);

/*
 * Enters deep sleep and does not return. Enables a timer wakeup, plus a GPIO
 * wakeup on NODE_PIN_SENSOR_INT if node_hal_enable_sensor_wakeup() was called.
 *
 * Callers must ensure Wi-Fi and BT are already stopped; deep sleep drops the
 * radio regardless, but stopping it first keeps the power-down clean.
 */
void node_hal_deep_sleep(uint64_t sleep_us);

/*
 * Arms the sensor interrupt pin as a deep-sleep wake source. Must be called
 * before node_hal_deep_sleep(). Only valid on RTC IOs (GPIO0-5); a compile-time
 * check in node_config.h guarantees the pin is in range.
 */
void node_hal_enable_sensor_wakeup(bool active_low);