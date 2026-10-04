idf_component_register(
    SRCS "node_main.c"
    INCLUDE_DIRS "."
    REQUIRES node_config node_hal node_sensor node_transport
)