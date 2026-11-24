// Punto de entrada del módulo Cebo para el registro de sentinel_api. El núcleo
// lo enlaza solo cuando SENTINEL_MODULE_CEBO=1 (optional_modules.cpp).
#include "include/cebo.hpp"

extern "C" sentinel::Module *sentinel_cebo_module() {
  static sentinel::cebo::Cebo module(sentinel::cebo::esp_http_platform());
  return &module;
}
