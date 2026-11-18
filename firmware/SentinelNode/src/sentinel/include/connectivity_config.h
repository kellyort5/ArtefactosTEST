#pragma once
#include "core.h"

namespace sentinel {
// Only operator-provisioned Wi-Fi/MQTT material. Never mode, UTC or honeypot input.
struct ConnectivityConfig {
  char ssid[33] = {}, wifi_password[64] = {}, uri[257] = {}, username[257] = {},
       password[513] = {};
  ~ConnectivityConfig() { wipe(this, sizeof(*this)); }
  bool from_args(const cJSON *);
  bool valid() const;
};
enum class ConnectivityLoad { Missing, Loaded, Error };
ConnectivityLoad load_connectivity(ConnectivityConfig &);
bool save_connectivity(const ConnectivityConfig &);
} // namespace sentinel
