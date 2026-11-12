idf_component_register(SRCS "module.cpp" "discovery.cpp" "capture.cpp" "frame_parser.cpp" "channel_scheduler.cpp" "detectors.cpp" INCLUDE_DIRS "include" REQUIRES sentinel_api esp_wifi)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
