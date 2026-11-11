idf_component_register(SRCS "module.cpp" "discovery.cpp" INCLUDE_DIRS "include" REQUIRES sentinel_api esp_wifi)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
