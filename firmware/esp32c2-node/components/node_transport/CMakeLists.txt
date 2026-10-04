idf_component_register(
    SRCS
        "node_transport.c"
        "drivers/serial_log.c"
        "drivers/http_post.c"
        "drivers/mqtt_publish.c"
    INCLUDE_DIRS "include"
    REQUIRES node_config node_hal node_sensor esp_http_client esp-tls esp_wifi esp_event esp_netif
)