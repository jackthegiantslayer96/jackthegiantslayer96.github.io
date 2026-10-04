/*
 * AHT21 / AHT20 / AM2320 temperature + humidity over I2C.
 *
 * All AHT-family parts share the same register protocol; the driver fits any of
 * them since only the power-on delay differs and the generous one covers all.
 *
 * The CRC on the 6-byte data block is not optional. Without it the sensor
 * returns its previous sample on a failed I2C transaction, which shows up as
 * plausible but stale readings rather than an obvious error.
 */
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "node_config.h"
#include "node_sensor.h"

static const char *TAG = "sensor:aht21";

#define AHT_I2C_ADDR 0x38
#define AHT_I2C_TIMEOUT_MS 100
#define AHT_CMD_INIT 0xBE
#define AHT_CMD_MEASURE 0xAC
#define AHT_MEASURE_MS 85
#define AHT_STATUS_BUSY 0x80

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;
static bool s_initialized;

/*
 * The AHT checksum is the XOR of the first five data bytes.
 */
static bool aht_checksum_valid(const uint8_t *buf)
{
    return buf[5] == (uint8_t)(buf[0] ^ buf[1] ^ buf[2] ^ buf[3] ^ buf[4]);
}

static esp_err_t aht_write(uint8_t cmd)
{
    const uint8_t payload[3] = {cmd, cmd, cmd};
    return i2c_master_transmit(s_dev, payload, sizeof(payload), AHT_I2C_TIMEOUT_MS);
}

static esp_err_t aht_read_block(uint8_t *buf, size_t len)
{
    return i2c_master_receive(s_dev, buf, len, AHT_I2C_TIMEOUT_MS);
}

static esp_err_t aht_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    /*
     * The I2C master driver is powered down across deep sleep, so a fresh bus
     * handle is required on every wake. It is not a one-time setup.
     */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = NODE_PIN_SDA,
        .scl_io_num = NODE_PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c bus init failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev), TAG, "i2c add failed");

    /*
     * 100ms soft reset on every wake. The part also needs up to 40ms after
     * power-on before it answers; skipping this is the most common cause of
     * "works on the bench, fails from deep sleep".
     */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_RETURN_ON_ERROR(aht_write(AHT_CMD_INIT), TAG, "soft reset failed");

    bool ready = false;
    for (int i = 0; i < 10; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t status = 0;
        if (aht_read_block(&status, 1) == ESP_OK) {
            ESP_LOGI(TAG, "sensor ready, status byte 0x%02x", status);
            ready = true;
            break;
        }
    }

    if (!ready) {
        ESP_LOGE(TAG, "AHT21 did not respond on addr 0x%02x after reset", AHT_I2C_ADDR);
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
        return ESP_ERR_NOT_FOUND;
    }

    s_initialized = true;
    return ESP_OK;
}

static esp_err_t aht_read_once(node_readings_t *out)
{
    ESP_RETURN_ON_ERROR(aht_write(AHT_CMD_MEASURE), TAG, "trigger failed");

    vTaskDelay(pdMS_TO_TICKS(AHT_MEASURE_MS));

    /*
     * The sensor NACKs while converting, so poll until it answers rather than
     * assuming the fixed delay above was sufficient.
     */
    uint8_t buf[6];
    esp_err_t err = ESP_ERR_TIMEOUT;

    for (int attempt = 0; attempt < 5; attempt++) {
        err = aht_read_block(buf, sizeof(buf));
        if (err != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if ((buf[0] & AHT_STATUS_BUSY) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_RETURN_ON_ERROR(err, TAG, "read failed");

    if (buf[0] & AHT_STATUS_BUSY) {
        ESP_LOGE(TAG, "sensor still busy after %d attempts", 5);
        return ESP_ERR_TIMEOUT;
    }

    if (!aht_checksum_valid(buf)) {
        ESP_LOGE(TAG, "checksum mismatch, discarding sample");
        return ESP_ERR_INVALID_CRC;
    }

    /* 20-bit values: raw[0..1] = humidity, raw[2..4] = temperature. */
    uint32_t raw_hum = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | (buf[2] >> 4);
    uint32_t raw_temp = ((uint32_t)(buf[2] & 0x0F) << 16) | ((uint32_t)buf[3] << 8) | buf[4];

    out->humidity_pct = (float)raw_hum * 100.0f / 1048576.0f;
    out->temp_c = (float)raw_temp * 200.0f / 1048576.0f - 50.0f;
    out->valid_mask |= NODE_FIELD_TEMP_C | NODE_FIELD_HUMIDITY_PCT;

    return ESP_OK;
}

static esp_err_t aht_read(node_readings_t *out)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return aht_read_once(out);
}

static void aht_deinit(void)
{
    if (s_dev != NULL) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    if (s_bus != NULL) {
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
    }
    s_initialized = false;
}

const node_sensor_t *node_sensor_aht21_get_driver(void)
{
    static const node_sensor_t driver = {
        .init = aht_init,
        .read = aht_read,
        .deinit = aht_deinit,
    };
    return &driver;
}