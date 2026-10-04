idf_component_register(
    SRCS "node_hal.c"
    INCLUDE_DIRS "include"
    REQUIRES node_config esp_driver_gpio esp_timer
)