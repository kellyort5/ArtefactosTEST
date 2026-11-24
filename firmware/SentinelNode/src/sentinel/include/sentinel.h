#pragma once
#include "core.h"
#include "esp_err.h"
namespace sentinel {
enum class Kind {
  ShortPress,
  PairWindow,
  Passkey,
  BleUp,
  BleDown,
  WifiUp,
  WifiDown,
  MqttUp,
  MqttDown,
  MqttError
};
struct Signal {
  Kind kind;
  uint32_t value = 0;
  uint16_t connection = 0;
};
void start();
bool register_module(Mode, Module &);
void register_optional_modules();
bool post(Signal signal);
bool submit(Channel channel, const char *json, size_t n, bool authorized,
            bool retained = false, uint32_t session = 0);
bool metadata(char *out, size_t capacity);
bool ble_init();
bool ble_send(const char *json, size_t n, uint32_t session = 0);
void ble_window(uint64_t deadline);
uint64_t ble_deadline();
void ble_passkey(uint16_t connection, uint32_t value, bool displayed);
const char *ble_status();
uint32_t ble_session();
bool oled_init();
bool oled_show(const char *a, const char *b, const char *c, const char *d);
bool button_init();
bool button_down();
} // namespace sentinel
