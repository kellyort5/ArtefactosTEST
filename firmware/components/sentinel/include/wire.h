#pragma once
#include "core.h"
namespace sentinel {
struct Envelope {
  const char *device, *boot;
  uint64_t seq, uptime;
  const char *timestamp = nullptr, *quality = "unknown";
};
cJSON *envelope(const Envelope &, const char *type, cJSON *payload);
cJSON *state_payload(const Runtime &, const char *ble, const char *wifi,
                     const char *mqtt, bool oled, bool button, bool journal,
                     bool ble_available);
cJSON *ack_payload(const Result &, const char *status);
cJSON *event_payload(uint64_t cursor, const char *name, Profile,
                     const char *code);
bool serialize(cJSON *owned, char out[MaxMessage + 1]);
} // namespace sentinel
