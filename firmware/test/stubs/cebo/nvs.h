#pragma once
// Frontera del SDK para el almacén de configuración de Cebo. Las pruebas de
// host la implementan con un NVS en memoria con inyección de fallos.
#include "esp_err.h"
#include <cstddef>
using nvs_handle_t = unsigned;
constexpr int NVS_READONLY = 0, NVS_READWRITE = 1;
esp_err_t nvs_open(const char *, int, nvs_handle_t *);
void nvs_close(nvs_handle_t);
esp_err_t nvs_get_blob(nvs_handle_t, const char *, void *, size_t *);
esp_err_t nvs_set_blob(nvs_handle_t, const char *, const void *, size_t);
esp_err_t nvs_commit(nvs_handle_t);
