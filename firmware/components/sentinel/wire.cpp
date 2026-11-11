#include "wire.h"
#include "protocol_v2.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
namespace sentinel {
cJSON *envelope(const Envelope &e, const char *type, cJSON *payload) {
  auto j = cJSON_CreateObject();
  char id[65];
  snprintf(id, sizeof(id), "%.42s:%llu", e.boot,
           static_cast<unsigned long long>(e.seq));
  cJSON_AddNumberToObject(j, "version", e.version);
  cJSON_AddStringToObject(j, "type", type);
  cJSON_AddStringToObject(j, "device_id", e.device);
  cJSON_AddStringToObject(j, "boot_id", e.boot);
  cJSON_AddStringToObject(j, "message_id", id);
  cJSON_AddNumberToObject(j, "seq", e.seq);
  cJSON_AddNumberToObject(j, "uptime_ms", e.uptime);
  if (e.timestamp)
    cJSON_AddStringToObject(j, "timestamp", e.timestamp);
  else
    cJSON_AddNullToObject(j, "timestamp");
  cJSON_AddStringToObject(j, "time_quality", e.quality);
  cJSON_AddItemToObject(j, "payload", payload);
  return j;
}
cJSON *state_payload(const Runtime &r, const char *ble, const char *wifi,
                     const char *mqtt, bool oled, bool button, bool journal,
                     bool ble_available) {
  auto p = cJSON_CreateObject();
  cJSON_AddStringToObject(p, "profile", profile_name(r.profile));
  cJSON_AddStringToObject(p, "phase", r.phase);
  cJSON_AddNumberToObject(p, "revision", r.revision);
  if (*r.last_command)
    cJSON_AddStringToObject(p, "last_command_id", r.last_command);
  else
    cJSON_AddNullToObject(p, "last_command_id");
  auto t = cJSON_AddObjectToObject(p, "transport");
  cJSON_AddStringToObject(t, "ble", ble);
  cJSON_AddStringToObject(t, "wifi", wifi);
  cJSON_AddStringToObject(t, "mqtt", mqtt);
  auto per = cJSON_AddObjectToObject(p, "peripherals");
  cJSON_AddBoolToObject(per, "oled", oled);
  cJSON_AddBoolToObject(per, "button", button);
  auto cap = cJSON_AddObjectToObject(p, "capabilities");
  cJSON_AddBoolToObject(cap, "ble_control", ble_available);
  cJSON_AddBoolToObject(cap, "mqtt_control", true);
  cJSON_AddBoolToObject(cap, "event_journal", journal);
  cJSON_AddBoolToObject(cap, "oled", oled);
  cJSON_AddBoolToObject(cap, "button", button);
  for (auto name : {"capture", "detectors", "honeypot_http", "honeypot_telnet",
                    "notifications", "sanitizador"})
    cJSON_AddFalseToObject(cap, name);
  if (r.error)
    cJSON_AddStringToObject(p, "error", r.error);
  else
    cJSON_AddNullToObject(p, "error");
  return p;
}
cJSON *ack_payload(const Result &r, const char *status) {
  auto p = cJSON_CreateObject();
  cJSON_AddStringToObject(p, "command_id", r.id);
  cJSON_AddStringToObject(p, "status", status);
  cJSON_AddNumberToObject(p, "revision", r.revision);
  if (r.error)
    cJSON_AddStringToObject(p, "error", r.error);
  else
    cJSON_AddNullToObject(p, "error");
  if (r.page && !r.error) {
    auto q = cJSON_AddObjectToObject(p, "result");
    cJSON_AddNumberToObject(q, "next_cursor", r.next);
    cJSON_AddNumberToObject(q, "retained_from", r.from);
    cJSON_AddNumberToObject(q, "retained_to", r.to);
    cJSON_AddBoolToObject(q, "more", r.more);
    cJSON_AddBoolToObject(q, "gap", r.gap);
  } else
    cJSON_AddNullToObject(p, "result");
  return p;
}
cJSON *event_payload(uint64_t cursor, const char *name, Profile profile,
                     const char *code) {
  auto p = cJSON_CreateObject();
  cJSON_AddNumberToObject(p, "journal_seq", cursor);
  cJSON_AddStringToObject(p, "name", name);
  cJSON_AddStringToObject(p, "profile", profile_name(profile));
  if (code)
    cJSON_AddStringToObject(p, "code", code);
  else
    cJSON_AddNullToObject(p, "code");
  return p;
}
cJSON *module_state_payload(const Runtime &r, const ModuleSupervisor &modules,
                            const char *ble, const char *wifi, const char *mqtt,
                            bool oled, bool button, bool journal) {
  auto p = state_payload(r, ble, wifi, mqtt, oled, button, journal,
                         strcmp(ble, "stopped") != 0);
  auto cap = cJSON_GetObjectItemCaseSensitive(p, "capabilities");
  auto c = modules.capabilities();
  const char *names[] = {"capture",         "detectors",     "honeypot_http",
                         "honeypot_telnet", "notifications", "sanitizador"};
  bool flags[] = {c.capture,         c.detectors,     c.honeypot_http,
                  c.honeypot_telnet, c.notifications, c.sanitizador};
  for (unsigned i = 0; i < 6; ++i)
    cJSON_ReplaceItemInObjectCaseSensitive(cap, names[i],
                                           cJSON_CreateBool(flags[i]));
  cJSON_ReplaceItemInObjectCaseSensitive(
      cap, "mqtt_control", cJSON_CreateBool(modules.available(Profile::Cebo)));
  auto module = cJSON_CreateObject();
  cJSON_AddStringToObject(module, "kind", "idle");
  if (auto m = modules.get(r.profile)) {
    ModuleSnapshot s;
    m->snapshot(s);
    cJSON_Delete(module);
    module = v2::parse(s.state, strnlen(s.state, sizeof(s.state)));
  }
  cJSON_AddItemToObject(p, "module", module);
  return p;
}
cJSON *module_ack_payload(const Result &r, const char *status) {
  auto p = ack_payload(r, status);
  if (r.page && !r.error && strcmp(status, "accepted")) {
    auto q = cJSON_GetObjectItemCaseSensitive(p, "result");
    cJSON_AddStringToObject(q, "kind", "events");
  } else if (*r.body && !r.error && strcmp(status, "accepted")) {
    auto body = v2::parse(r.body, strnlen(r.body, sizeof(r.body)));
    cJSON_ReplaceItemInObjectCaseSensitive(p, "result", body);
  }
  return p;
}
cJSON *module_event_payload(uint64_t cursor, const char *name, Profile profile,
                            const char *code, const char *details,
                            const char *severity, bool simulated) {
  auto p = event_payload(cursor, name, profile, code);
  cJSON_AddStringToObject(p, "origin", simulated ? "simulated" : "real");
  cJSON_AddStringToObject(p, "severity", severity);
  cJSON_AddItemToObject(p, "details", v2::parse(details, strlen(details)));
  return p;
}
bool serialize(cJSON *j, char out[MaxMessage + 1]) {
  bool ok = j && cJSON_PrintPreallocated(j, out, MaxMessage + 1, false);
  if (ok && number(j, "version") == 2) {
    const char *type = str(j, "type");
    ok = type && v2::clean(j) && v2::validate(type, j);
  }
  cJSON_Delete(j);
  if (!ok)
    out[0] = 0;
  return ok;
}
} // namespace sentinel
