idf_component_register(
    SRCS
        "node_sensor.c"
        "drivers/analog_raw.c"
        "drivers/aht21.c"
    INCLUDE_DIRS "include"
    REQUIRES node_config esp_adc esp_driver_i2c esp_driver_gpio esp_timer
)