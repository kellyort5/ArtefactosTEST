#pragma once
#include "core.h"
#include "module_runtime.h"
namespace sentinel {
struct Envelope {
  const char *device, *boot;
  uint64_t seq, uptime;
  const char *timestamp = nullptr, *quality = "unknown";
  unsigned version = 1;
};
cJSON *envelope(const Envelope &, const char *type, cJSON *payload);
cJSON *state_payload(const Runtime &, const char *ble, const char *wifi,
                     const char *mqtt, bool oled, bool button, bool journal,
                     bool ble_available);
cJSON *ack_payload(const Result &, const char *status);
cJSON *event_payload(uint64_t cursor, const char *name, Profile,
                     const char *code);
cJSON *module_state_payload(const Runtime &, const ModuleSupervisor &,
                            const char *ble, const char *wifi, const char *mqtt,
                            bool oled, bool button, bool journal);
cJSON *module_ack_payload(const Result &, const char *status);
cJSON *module_event_payload(uint64_t, const char *, Profile, const char *code,
                            const char *details, const char *severity = "info",
                            bool simulated = false);
bool serialize(cJSON *owned, char out[MaxMessage + 1]);
} // namespace sentinel
