idf_component_register(SRCS "core.cpp" "wire.cpp" "digest.cpp" "runtime.cpp" "peripherals.cpp" "ble.cpp" "network.cpp"
 INCLUDE_DIRS "include"
 REQUIRES json nvs_flash esp_wifi esp_netif esp_event esp_timer esp_psram esp_driver_gpio esp_driver_i2c esp_lcd bt mqtt mbedtls esp_system spi_flash)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-misleading-indentation)
