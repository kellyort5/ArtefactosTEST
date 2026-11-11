set(SENTINEL_MODULE_COMPONENTS "")
foreach(MODE ESCUDO SANITIZADOR CEBO)
  if(CONFIG_SENTINEL_MODULE_${MODE})
    string(TOLOWER "${MODE}" MODULE_NAME)
    list(APPEND SENTINEL_MODULE_COMPONENTS "${MODULE_NAME}")
  endif()
endforeach()
idf_component_register(SRCS "core.cpp" "module_runtime.cpp" "optional_modules.cpp" "protocol_v2.cpp" "wire.cpp" "digest.cpp" "runtime.cpp" "peripherals.cpp" "ble.cpp" "network.cpp"
 INCLUDE_DIRS "include"
 REQUIRES ${SENTINEL_MODULE_COMPONENTS} sentinel_api json nvs_flash esp_wifi esp_netif esp_event esp_timer esp_psram esp_driver_gpio esp_driver_i2c esp_lcd bt mqtt mbedtls esp_system spi_flash)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-misleading-indentation)

# Compile predicates from the versioned source schemas; no generated files in Git.
set(SENTINEL_CONTRACTS "${CMAKE_CURRENT_LIST_DIR}/../../../contracts/schemas/v2")
file(GLOB SENTINEL_SCHEMAS "${SENTINEL_CONTRACTS}/*.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SENTINEL_SCHEMAS} "${CMAKE_CURRENT_LIST_DIR}/../../tools/generate_validator.py")
execute_process(COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../../tools/generate_validator.py" "${CMAKE_CURRENT_BINARY_DIR}/v2_validator.inc"
  RESULT_VARIABLE SENTINEL_VALIDATOR_RESULT)
if(NOT SENTINEL_VALIDATOR_RESULT EQUAL 0)
  message(FATAL_ERROR "Cannot generate Sentinel v2 validator")
endif()
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
