# ESP32-C3-MINI-1 sensor node

Battery-oriented Wi-Fi sensor node firmware for the **ESP32-C3-MINI-1** (RISC-V).

Each wake does one cycle: **sample → connect → publish → deep sleep**. Because deep sleep
discards RAM and powers the radio down, this is a one-shot cycle rather than a daemon.

> The directory is named `esp32c2-node` from the original brief, but the hardware in hand
> was identified as a **C3** from its native USB Serial/JTAG peripheral (`VID_303A&PID_1001`),
> which the C2 does not have. Both chips are RISC-V and share the ADC / GPIO / deep-sleep
> layout this firmware uses, so only the target name changed. Renaming the directory is a
> one-liner if you want it to match.

> This directory is entirely self-contained. It is not part of the Next.js build and
> requires no changes to `src/`, `package.json`, or `tsconfig.json`.

## Requirements

- **ESP-IDF v5.x or v6.x** with the `esp32c3` target. On Windows, install via the
  ESP-IDF Installation Manager (EIM) and run the project from an **ESP-IDF prompt** so the
  RISC-V toolchain is on `PATH`.
- Arduino is deliberately not used: on the C3 it works, but ESP-IDF is the better-supported
  path and is what the build is written against.

## Build

```powershell
idf.py set-target esp32c3
idf.py build
idf.py -p COM3 flash monitor
```

`COM3` is the C3's **native USB** port — no external USB-UART bridge is needed, and the
C3 enters download mode automatically.

Override any config value from the command line without editing headers:

```powershell
idf.py build -DNODE_SENSOR=NODE_SENSOR_AHT21 -DNODE_TRANSPORT=NODE_TRANSPORT_HTTP
```

## Configuration

Everything tunable lives in
[`components/node_config/include/node_config.h`](components/node_config/include/node_config.h),
each value guarded by `#ifndef` so it can be overridden with `-D`.

Before the first networked run you must set:

| Value | Purpose |
|---|---|
| `NODE_WIFI_SSID` / `NODE_WIFI_PASSWORD` | AP credentials |
| `NODE_PUBLISH_URL` | HTTP endpoint (only used by the HTTP transport) |

**Credentials are compiled into the binary in plain text.** Fine for a bench prototype,
not acceptable for a deployed image — NVS-backed provisioning is the fix and is out of scope.

### Pin map

Every pin is an **RTC IO (GPIO0–5)**, which is required for GPIO wakeup from deep sleep.
The C3 has **three** boot strapping pins — **GPIO2, GPIO8 and GPIO9** — sampled at reset to
pick the boot mode. `GPIO11–17` are the flash. A `_Static_assert` block in `node_config.h`
fails the build if a pin is out of RTC range or two pins collide.

| Function | GPIO |
|---|---|
| Analog in (ADC1 ch0) | 0 |
| I2C SDA | 1 |
| Status LED | 3 |
| Sensor INT (wake) | 4 |
| I2C SCL | 5 |

SCL is on **GPIO5, not GPIO2**: the C3 samples GPIO2 at reset as a strapping pin, and an
I2C pull-up holding it high at power-on can select the wrong boot mode. GPIO5 is
RTC-capable like the rest of the map, so the move costs nothing.

## Bring-up order

Each step is independently observable, so failures localise quickly.

1. **Serial transport, no network.** The default config. Expect a wake cause, a JSON
   reading, a clean sleep, then silence. Proves sensor + HAL + the cycle.
2. **Sleep actually happens.** Confirm current drops and the log goes quiet between cycles.
   If the LED never goes dark, something is blocking before `esp_deep_sleep_start()`.
3. **Wi-Fi connect.** Confirm association and a DHCP address from a *cold* boot. This is
   where a too-short connect timeout shows up.
4. **Publish.** Point `NODE_PUBLISH_URL` at something that logs requests; confirm the JSON
   body arrives and is well-formed.

## Layout

```
components/
  node_config/     all tunables, one header
  node_hal/        pin helpers, deep sleep, RTC-retained state
  node_sensor/     node_sensor_t interface + dispatch
    drivers/       analog_raw.c (ADC1 mV), aht21.c (I2C temp+humidity)
  node_transport/  node_transport_t interface + dispatch
    drivers/       serial_log.c (default), http_post.c, mqtt_publish.c (stub)
  node_main/       the wake cycle
```

Adding a sensor: write `init`/`read`/`deinit`, expose it via `node_sensor_<name>_get_driver()`,
add it to the switch in `node_sensor.c`, and list it in the component `CMakeLists.txt`.
Nothing else changes.

## C2 specifics worth knowing

- **Deep sleep kills Wi-Fi.** Every cycle pays a cold association and a full DHCP handshake.
  This dominates the power budget — measure awake and sleep current separately before
  trusting any battery-life estimate.
- **Only timer and GPIO wakeup** exist. There is no `EXT0`/`EXT1`; deep-sleep GPIO wakeup is
  restricted to RTC IOs.
- **ADC `Vref` is ~1100 mV**, varying 1000–1200 mV per part, so the curve-fitting
  calibration scheme is required, not optional.
- **No ADC2**, so unlike the classic ESP32 the ADC and Wi-Fi can be used simultaneously.

## Not implemented (phase 2)

- MQTT (stub only — see the reasoning in `drivers/mqtt_publish.c`)
- OTA updates
- Light-sleep / Wi-Fi modem-sleep variant, which is the right answer for a higher publish rate
- NVS credential provisioning
- Secure boot and flash encryption