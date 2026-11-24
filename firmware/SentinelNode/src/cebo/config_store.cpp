// Persistencia de la configuración aceptada de Cebo en NVS estándar, con
// espacio de nombres propio y el mismo patrón que la selección de Escudo: un
// registro acotado, validado al leer, y confirmación solo tras nvs_commit. No
// se guarda nada recibido por HTTP ni Telnet; solo los dos límites del
// contrato. NVS estándar no promete cifrado físico (docs/persistence-policy.md).
#include "include/cebo.hpp"
#include "nvs.h"
#include <cstring>

namespace sentinel::cebo {
namespace {
constexpr uint8_t Header[] = {'S', 'C', 'E', 'B', 1, 0, 0, 0};
constexpr const char *Namespace = "cebo_config", *Key = "settings";
// Solo campos de bytes: sin relleno de ABI, marcas de tiempo ni estructuras del
// SDK. El tiempo de inactividad se guarda en little-endian explícito.
struct Record {
  uint8_t header[8] = {};
  uint8_t max_connections = 0;
  uint8_t idle_timeout_ms[4] = {};
};
static_assert(sizeof(Record) == 13, "record has no padding");

class NvsConfigStore final : public ConfigStore {
public:
  Load load(Config &out) override {
    nvs_handle_t handle;
    esp_err_t status = nvs_open(Namespace, NVS_READONLY, &handle);
    if (status == ESP_ERR_NVS_NOT_FOUND)
      return Load::Missing; // nunca configurado: valores por defecto
    if (status != ESP_OK)
      return Load::Error;
    size_t size = 0;
    status = nvs_get_blob(handle, Key, nullptr, &size);
    if (status == ESP_ERR_NVS_NOT_FOUND) {
      nvs_close(handle);
      return Load::Missing;
    }
    Record record;
    if (status == ESP_OK && size == sizeof(record)) {
      status = nvs_get_blob(handle, Key, &record, &size);
      if (status == ESP_OK && (size != sizeof(record) ||
                              memcmp(record.header, Header, sizeof(Header))))
        status = ESP_FAIL;
    } else if (status == ESP_OK) {
      status = ESP_FAIL; // tamaño inesperado: registro ajeno o dañado
    }
    nvs_close(handle);
    if (status != ESP_OK)
      return Load::Error;
    Config config;
    config.max_connections = record.max_connections;
    config.idle_timeout_ms = uint32_t(record.idle_timeout_ms[0]) |
                             uint32_t(record.idle_timeout_ms[1]) << 8 |
                             uint32_t(record.idle_timeout_ms[2]) << 16 |
                             uint32_t(record.idle_timeout_ms[3]) << 24;
    if (!valid(config))
      return Load::Error; // un valor fuera de rango nunca se aplica
    out = config;
    return Load::Loaded;
  }

  bool save(const Config &config) override {
    if (!valid(config))
      return false;
    Record record;
    memcpy(record.header, Header, sizeof(Header));
    record.max_connections = uint8_t(config.max_connections);
    for (unsigned n = 0; n < 4; ++n)
      record.idle_timeout_ms[n] = uint8_t(config.idle_timeout_ms >> (8 * n));
    nvs_handle_t handle;
    if (nvs_open(Namespace, NVS_READWRITE, &handle) != ESP_OK)
      return false;
    esp_err_t status = nvs_set_blob(handle, Key, &record, sizeof(record));
    if (status == ESP_OK)
      status = nvs_commit(handle); // solo tras el commit se confirma
    nvs_close(handle);
    return status == ESP_OK;
  }
};
} // namespace

ConfigStore &nvs_config_store() {
  static NvsConfigStore store;
  return store;
}

} // namespace sentinel::cebo
