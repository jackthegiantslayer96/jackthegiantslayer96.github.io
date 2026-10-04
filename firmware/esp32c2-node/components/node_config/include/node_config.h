/*
 * Central configuration for the ESP32-C2-MINI-1 sensor node.
 *
 * Every value below can be overridden from the build without editing this file,
 * e.g. idf.py build -DNODE_SLEEP_INTERVAL_US=60000000
 * The #ifndef guards make that work.
 */
#pragma once

/* ------------------------------------------------------------------ */
/* Credentials                                                         */
/* ------------------------------------------------------------------ */

/*
 * TODO: replace before first networked run.
 *
 * These are compile-time constants, so the SSID and password end up in the
 * binary in plain text. That is acceptable for a bench prototype and
 * unacceptable for anything deployed where the image could leak. NVS-backed
 * provisioning is the fix and is deliberately out of scope for phase 1.
 */
#ifndef NODE_WIFI_SSID
#define NODE_WIFI_SSID "your-ssid"
#endif

#ifndef NODE_WIFI_PASSWORD
#define NODE_WIFI_PASSWORD "your-password"
#endif

/* ------------------------------------------------------------------ */
/* Transport                                                           */
/* ------------------------------------------------------------------ */

typedef enum {
    NODE_TRANSPORT_SERIAL = 0, /* bench bring-up: print JSON, touch no network */
    NODE_TRANSPORT_HTTP,      /* POST JSON to NODE_PUBLISH_URL              */
    NODE_TRANSPORT_MQTT,      /* phase 2 stub                              */
} node_transport_kind_t;

#ifndef NODE_TRANSPORT
#define NODE_TRANSPORT NODE_TRANSPORT_SERIAL
#endif

/*
 * Only used when NODE_TRANSPORT == NODE_TRANSPORT_HTTP.
 * Point this at something that logs requests, e.g. httpbin or a local endpoint.
 */
#ifndef NODE_PUBLISH_URL
#define NODE_PUBLISH_URL "http://example.invalid/api/telemetry"
#endif

/* ------------------------------------------------------------------ */
/* Sensor                                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    NODE_SENSOR_ANALOG_RAW = 0, /* ADC1 calibrated mV on NODE_PIN_ADC */
    NODE_SENSOR_AHT21,          /* I2C temperature + humidity         */
} node_sensor_kind_t;

#ifndef NODE_SENSOR
#define NODE_SENSOR NODE_SENSOR_ANALOG_RAW
#endif

/* ------------------------------------------------------------------ */
/* Pin map - all pins are RTC IOs (GPIO0-5) on purpose                 */
/*                                                                     */
/* The C3 has three boot strapping pins - GPIO2, GPIO8 and GPIO9 -     */
/* sampled at reset to pick the boot mode. GPIO11-17 are the flash.    */
/* GPIO6/7 are analog-only and GPIO18/19 carry the native USB, so     */
/* none of those belong in application logic.                         */
/*                                                                     */
/* SCL is deliberately on GPIO5 rather than GPIO2: GPIO2 is a strapping */
/* pin, and holding it high through an I2C pull-up at reset can push  */
/* the chip toward the wrong boot mode. GPIO5 is RTC-capable like the  */
/* rest of this map, so nothing is lost by moving off GPIO2.           */
/* ------------------------------------------------------------------ */

#ifndef NODE_PIN_ADC
#define NODE_PIN_ADC 0 /* ADC1 channel 0 */
#endif

#ifndef NODE_PIN_SDA
#define NODE_PIN_SDA 1
#endif

#ifndef NODE_PIN_SCL
#define NODE_PIN_SCL 5
#endif

#ifndef NODE_PIN_LED
#define NODE_PIN_LED 3
#endif

#ifndef NODE_PIN_SENSOR_INT
#define NODE_PIN_SENSOR_INT 4 /* optional sensor interrupt / wake source */
#endif

/* Set to 0 to disable the status LED entirely. */
#ifndef NODE_LED_ENABLED
#define NODE_LED_ENABLED 1
#endif

/* ------------------------------------------------------------------ */
/* Power budget                                                        */
/* ------------------------------------------------------------------ */

/*
 * Bring-up hold. While non-zero the node stays awake for this long after
 * finishing its work instead of entering deep sleep immediately.
 *
 * On this board it exists for one reason: deep sleep powers down the C3's
 * native USB peripheral, so a node that samples and sleeps within a second can
 * vanish from the USB bus part-way through a flash or a monitor session. The
 * symptoms are "Invalid head of packet" or "Unable to verify flash chip
 * connection", which point nowhere near the real cause.
 *
 * Defaults to 0 because normal operation should sleep, and because once you are
 * on an external USB-UART adapter this is unnecessary. Enable per build with
 *   idf.py build -DNODE_BRINGUP_HOLD_MS=10000
 */
#ifndef NODE_BRINGUP_HOLD_MS
#define NODE_BRINGUP_HOLD_MS 0
#endif

/* Default 5 minutes between wake cycles. */
#ifndef NODE_SLEEP_INTERVAL_US
#define NODE_SLEEP_INTERVAL_US (5ULL * 60ULL * 1000000ULL)
#endif

/*
 * Interval used once consecutive failures pass NODE_MAX_PUBLISH_ATTEMPTS.
 * Without this a dead server or an absent AP turns the node into a hot loop
 * that drains the battery much faster than the sleep interval suggests.
 */
#ifndef NODE_BACKOFF_INTERVAL_US
#define NODE_BACKOFF_INTERVAL_US (30ULL * 60ULL * 1000000ULL)
#endif

#ifndef NODE_MAX_PUBLISH_ATTEMPTS
#define NODE_MAX_PUBLISH_ATTEMPTS 3
#endif

#ifndef NODE_PUBLISH_RETRY_DELAY_MS
#define NODE_PUBLISH_RETRY_DELAY_MS 2000
#endif

/* ------------------------------------------------------------------ */
/* Networking timeouts                                                 */
/* ------------------------------------------------------------------ */

/*
 * Generous on purpose: this runs on a cold boot with no retained DHCP lease,
 * so every cycle pays a cold association. A short timeout here shows up as
 * intermittent failures that look like server problems.
 */
#ifndef NODE_WIFI_CONNECT_TIMEOUT_MS
#define NODE_WIFI_CONNECT_TIMEOUT_MS 20000
#endif

#ifndef NODE_WIFI_GET_IP_TIMEOUT_MS
#define NODE_WIFI_GET_IP_TIMEOUT_MS 10000
#endif

#ifndef NODE_HTTP_TIMEOUT_MS
#define NODE_HTTP_TIMEOUT_MS 10000
#endif

/* ------------------------------------------------------------------ */
/* Build-time guards                                                   */
/* ------------------------------------------------------------------ */

/*
 * GPIO wakeup from deep sleep is only valid on RTC IOs (GPIO0-5) on the C2.
 * Catching this at compile time beats discovering it when the wakeup source
 * silently refuses to register.
 */
_Static_assert(NODE_PIN_ADC <= 5, "NODE_PIN_ADC must be an RTC IO (0-5)");
_Static_assert(NODE_PIN_SDA <= 5, "NODE_PIN_SDA must be an RTC IO (0-5)");
_Static_assert(NODE_PIN_SCL <= 5, "NODE_PIN_SCL must be an RTC IO (0-5)");
_Static_assert(NODE_PIN_LED <= 5, "NODE_PIN_LED must be an RTC IO (0-5)");
_Static_assert(NODE_PIN_SENSOR_INT <= 5, "NODE_PIN_SENSOR_INT must be an RTC IO (0-5)");

_Static_assert(NODE_PIN_ADC != NODE_PIN_SDA, "ADC pin collides with SDA");
_Static_assert(NODE_PIN_ADC != NODE_PIN_SCL, "ADC pin collides with SCL");
_Static_assert(NODE_PIN_ADC != NODE_PIN_LED, "ADC pin collides with LED");
_Static_assert(NODE_PIN_ADC != NODE_PIN_SENSOR_INT, "ADC pin collides with sensor INT");
_Static_assert(NODE_PIN_SDA != NODE_PIN_SCL, "SDA collides with SCL");
_Static_assert(NODE_PIN_SDA != NODE_PIN_LED, "SDA collides with LED");
_Static_assert(NODE_PIN_SDA != NODE_PIN_SENSOR_INT, "SDA collides with sensor INT");
_Static_assert(NODE_PIN_SCL != NODE_PIN_LED, "SCL collides with LED");
_Static_assert(NODE_PIN_SCL != NODE_PIN_SENSOR_INT, "SCL collides with sensor INT");
_Static_assert(NODE_PIN_LED != NODE_PIN_SENSOR_INT, "LED collides with sensor INT");