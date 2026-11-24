#pragma once
#include "esp_err.h"
#include <cstddef>
#include <cstdint>
using nvs_handle_t=unsigned;
using nvs_iterator_t=void *;
using nvs_type_t=unsigned;
#define NVS_READONLY 0
#define NVS_TYPE_ANY 0
#define NVS_TYPE_BLOB 0x42
struct nvs_entry_info_t { char key[16]; nvs_type_t type; };
esp_err_t nvs_open(const char *,int,nvs_handle_t *);
void nvs_close(nvs_handle_t);
esp_err_t nvs_entry_find(const char *,const char *,nvs_type_t,nvs_iterator_t *);
esp_err_t nvs_entry_info(nvs_iterator_t,nvs_entry_info_t *);
esp_err_t nvs_entry_next(nvs_iterator_t *);
void nvs_release_iterator(nvs_iterator_t);
esp_err_t nvs_get_blob(nvs_handle_t,const char *,void *,size_t *);
// Deliberately no write/erase API: production preflight must remain read-only.
