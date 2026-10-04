/*
 * Reference driver: reads a raw analog voltage on NODE_PIN_ADC and reports
 * calibrated millivolts.
 *
 * This driver needs no external hardware, so it is the fastest way to prove the
 * whole sample -> publish -> sleep pipeline works. Start here.
 *
 * Note the C2 has a single ADC unit and no ADC2, so unlike the classic ESP32
 * there is no conflict between the ADC and an active Wi-Fi radio.
 */
#include <string.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "node_config.h"
#include "node_sensor.h"
#include "soc/adc_channel.h"

static const char *TAG = "sensor:analog";

static adc_oneshot_unit_handle_t s_unit;
static adc_cali_handle_t s_cali;
static bool s_initialized;

/* GPIO0 -> ADC1 channel 0, GPIO1 -> ch1 ... GPIO4 -> ch4. */
static adc_channel_t pin_to_channel(int gpio)
{
    switch (gpio) {
    case 0:
        return ADC_CHANNEL_0;
    case 1:
        return ADC_CHANNEL_1;
    case 2:
        return ADC_CHANNEL_2;
    case 3:
        return ADC_CHANNEL_3;
    case 4:
        return ADC_CHANNEL_4;
    default:
        return ADC_CHANNEL_0;
    }
}

static esp_err_t analog_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_unit), TAG, "oneshot unit init failed");

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12, /* full 0-3.3V-ish input range */
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_unit, pin_to_channel(NODE_PIN_ADC), &chan_cfg),
                        TAG, "channel config failed");

    /*
     * Vref varies per part, so raw counts are not usable as a voltage without
     * calibration. Curve fitting depends on eFuse bits that are not fitted on
     * every module, so a failure here degrades to uncalibrated counts rather
     * than failing the whole cycle - the reading is still logged, just as a
     * count instead of a millivolt value.
     */
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali) != ESP_OK) {
        ESP_LOGW(TAG, "no ADC calibration available; reporting raw counts. "
                      "Values are counts, not millivolts.");
        s_cali = NULL;
    }

    s_initialized = true;
    return ESP_OK;
}

static esp_err_t analog_read(node_readings_t *out)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    ESP_RETURN_ON_ERROR(adc_oneshot_read(s_unit, pin_to_channel(NODE_PIN_ADC), &raw), TAG,
                        "adc read failed");

    if (s_cali != NULL) {
        int mv = 0;
        if (adc_cali_raw_to_voltage(s_cali, raw, &mv) == ESP_OK) {
            out->raw_mv = mv;
        } else {
            ESP_LOGW(TAG, "raw->voltage conversion failed; reporting raw count");
            out->raw_mv = raw;
        }
    } else {
        out->raw_mv = raw;
    }

    out->valid_mask |= NODE_FIELD_RAW_MV;
    return ESP_OK;
}

static void analog_deinit(void)
{
    if (s_cali != NULL) {
        adc_cali_delete_scheme_curve_fitting(s_cali);
        s_cali = NULL;
    }
    if (s_unit != NULL) {
        adc_oneshot_del_unit(s_unit);
        s_unit = NULL;
    }
    s_initialized = false;
}

const node_sensor_t *node_sensor_analog_raw_get_driver(void)
{
    static const node_sensor_t driver = {
        .init = analog_init,
        .read = analog_read,
        .deinit = analog_deinit,
    };
    return &driver;
}