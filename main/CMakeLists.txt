idf_component_register(
    SRCS
        "main.c"
        "src/common.c"
        "src/modbus.c"
        "src/parser.c"
        "src/registers.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        bt
        nvs_flash
    PRIV_REQUIRES
        esp_driver_uart
        esp_driver_gpio
)