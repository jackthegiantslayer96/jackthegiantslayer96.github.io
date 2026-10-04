/*
 * HTTP POST transport: brings up Wi-Fi, waits for an IP, POSTs the payload as
 * JSON, then tears the radio down.
 *
 * TLS to an arbitrary endpoint needs the server's certificate or bundle, which
 * is out of scope here. Use an http:// endpoint during bring-up and add
 * esp_crt_bundle_attach() before pointing this at production.
 */
#include <string.h>

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "esp_check.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "node_config.h"
#include "node_transport.h"
#include "node_transport_internal.h"

static const char *TAG = "publish:http";

#define WIFI_ASSOC_BIT BIT0
#define WIFI_IP_BIT BIT1

static EventGroupHandle_t s_wifi_events;
static bool s_netif_ready;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        xEventGroupSetBits(s_wifi_events, WIFI_ASSOC_BIT);
        ESP_LOGI(TAG, "associated with AP");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_events, WIFI_ASSOC_BIT | WIFI_IP_BIT);
        ESP_LOGW(TAG, "disconnected from AP");
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_events, WIFI_IP_BIT);
    }
}

static esp_err_t http_init(void)
{
    if (s_netif_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop failed");

    esp_netif_create_default_wifi_sta();

    /*
     * WIFI_INIT_MEMORY_STATIC_MATCH is too tight once esp_http_client is in the
     * binary; without it long TLS handshakes fail with an out-of-memory error.
     */
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_cfg), TAG, "wifi init failed");

    s_wifi_events = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(s_wifi_events != NULL, ESP_ERR_NO_MEM, TAG, "event group alloc failed");

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                                                        NULL, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t sta_cfg = {
        .sta = {
            .ssid = NODE_WIFI_SSID,
            .password = NODE_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg), TAG, "set config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    s_netif_ready = true;

    /*
     * Association and DHCP are separate waits. Collapsing them into one timeout
     * hides the difference between "the AP is not there" and "the AP is there but
     * the DHCP server is not answering", which are very different problems.
     */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_ASSOC_BIT, pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(NODE_WIFI_CONNECT_TIMEOUT_MS));
    if ((bits & WIFI_ASSOC_BIT) == 0) {
        ESP_LOGE(TAG, "timed out after %d ms waiting for association", NODE_WIFI_CONNECT_TIMEOUT_MS);
        return ESP_ERR_TIMEOUT;
    }

    bits = xEventGroupWaitBits(s_wifi_events, WIFI_IP_BIT, pdFALSE, pdFALSE,
                               pdMS_TO_TICKS(NODE_WIFI_GET_IP_TIMEOUT_MS));
    if ((bits & WIFI_IP_BIT) == 0) {
        ESP_LOGE(TAG, "associated, but no DHCP lease after %d ms", NODE_WIFI_GET_IP_TIMEOUT_MS);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

static esp_err_t http_publish(const node_readings_t *readings)
{
    char payload[NODE_TRANSPORT_PAYLOAD_MAX];
    node_transport_build_payload(readings, payload, sizeof(payload));

    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%d", (int)strlen(payload));

    esp_http_client_config_t cfg = {
        .url = NODE_PUBLISH_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = NODE_HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "http client init failed");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Content-Length", content_length);

    esp_err_t err = esp_http_client_open(client, (int)strlen(payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "http open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    /*
     * write_len is capped to the payload length so a short write cannot walk
     * past the buffer; the loop exists because esp_http_client may write fewer
     * bytes than requested.
     */
    size_t written = 0;
    while (written < strlen(payload)) {
        int chunk = esp_http_client_write(client, payload + written, strlen(payload) - written);
        if (chunk < 0) {
            ESP_LOGE(TAG, "http write failed after %u bytes", (unsigned)written);
            err = ESP_FAIL;
            break;
        }
        if (chunk == 0) {
            ESP_LOGE(TAG, "http write made no progress");
            err = ESP_ERR_TIMEOUT;
            break;
        }
        written += (size_t)chunk;
    }

    if (err == ESP_OK) {
        int status = esp_http_client_fetch_headers(client);
        if (status < 0) {
            ESP_LOGE(TAG, "header fetch failed: %s", esp_err_to_name(status));
            err = ESP_FAIL;
        } else if (status < 200 || status >= 300) {
            /* Anything outside 2xx is a publish failure that must escalate backoff. */
            ESP_LOGE(TAG, "server returned HTTP %d", status);
            err = ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "published %u bytes, HTTP %d", (unsigned)written, status);
        }
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return err;
}

static void http_deinit(void)
{
    if (!s_netif_ready) {
        return;
    }

    /*
     * Explicit stop before deep sleep. Deep sleep drops the radio either way,
     * but stopping it first keeps the power-down clean and avoids leaving the
     * driver in a half-initialised state across the next wake.
     */
    esp_wifi_stop();
    s_netif_ready = false;

    if (s_wifi_events != NULL) {
        vEventGroupDelete(s_wifi_events);
        s_wifi_events = NULL;
    }
}

const node_transport_t *node_transport_http_post_get_driver(void)
{
    static const node_transport_t driver = {
        .init = http_init,
        .publish = http_publish,
        .deinit = http_deinit,
    };
    return &driver;
}