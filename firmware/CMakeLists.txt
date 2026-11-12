cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sentinel VERSION 0.1.0)

# Keep the native CMake dependency graph aware of changes to Escudo sources.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/components/escudo/CMakeLists.txt")
