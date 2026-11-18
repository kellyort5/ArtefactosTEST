#pragma once
#include "core.h"

namespace sentinel {
// Only locally provisioned Wi-Fi and broker URI. Never mode, UTC or honeypot input.
struct ConnectivityConfig {
  char ssid[33] = {}, wifi_password[64] = {}, uri[257] = {};
  ~ConnectivityConfig() { wipe(this, sizeof(*this)); }
  bool from_args(const cJSON *);
  bool valid() const;
};
enum class ConnectivityLoad { Missing, Loaded, Error };
ConnectivityLoad load_connectivity(ConnectivityConfig &);
bool save_connectivity(const ConnectivityConfig &);
} // namespace sentinel
