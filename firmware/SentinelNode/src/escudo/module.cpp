#include "include/escudo.hpp"
#include "../sentinel/include/test_diagnostics.h"
#include "../sentinel/include/escudo_target.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
namespace sentinel::escudo {
static bool print(cJSON *owned, char *out, size_t capacity) {
  bool ok = owned && cJSON_PrintPreallocated(owned, out, capacity, false);
  cJSON_Delete(owned);
  if (!ok)
    *out = 0;
  return ok;
}
static void add_mac(cJSON *j, const char *key, const uint8_t *mac) {
  char value[18];
  snprintf(value, sizeof(value), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  cJSON_AddStringToObject(j, key, value);
}
void Escudo::detection(const Detection &d) {
  const auto &n = d.network;
  auto j = cJSON_CreateObject();
  cJSON_AddStringToObject(j, "detector", detector_name(d.detector));
  // Hex conserva los bytes del SSID, incluidos NUL; no se fuerza texto UTF-8.
  char hex[65] = {};
  for (size_t i = 0; i < n.ssid_length; ++i)
    snprintf(hex + 2 * i, 3, "%02x", n.ssid[i]);
  auto name = cJSON_AddObjectToObject(j, "ssid");
  cJSON_AddStringToObject(name, "encoding", "hex");
  cJSON_AddStringToObject(name, "value", hex);
  add_mac(j, "bssid", n.bssid);
  if (d.related)
    add_mac(j, "related_bssid", d.related_bssid);
  cJSON_AddNumberToObject(j, "channel", n.channel);
  cJSON_AddNumberToObject(j, "rssi", n.rssi);
  cJSON_AddStringToObject(j, "auth", n.auth);
  cJSON_AddStringToObject(j, "cipher", n.cipher);
  cJSON_AddStringToObject(j, "pmf", n.pmf);
  cJSON_AddNumberToObject(j, "count", d.count);
  cJSON_AddNumberToObject(j, "window_ms", d.window_ms);
  cJSON_AddStringToObject(j, "suspicion", "medium");
  auto evidence = cJSON_AddArrayToObject(j, "evidence");
  const struct {
    uint16_t bit;
    const char *code;
  } codes[] = {{DeauthRate, "deauth_rate"},
               {DisassocRate, "disassoc_rate"},
               {DisconnectSequence, "disconnect_sequence"},
               {SameSsid, "same_ssid"},
               {DifferentBssid, "different_bssid"},
               {ChannelChange, "channel_change"},
               {RssiChange, "rssi_change"},
               {SecurityDowngrade, "security_downgrade"},
               {PmfChange, "pmf_change"},
               {MultipleSsids, "multiple_ssids"},
               {PassiveOnly, "passive_only"}};
  for (auto &c : codes)
    if (d.evidence & c.bit)
      cJSON_AddItemToArray(evidence, cJSON_CreateString(c.code));
  char details[1024];
  bool ok = print(j, details, sizeof(details)) &&
            context->emit_event("escudo.detection", "warning", details);
  capture_engine.detection(size_t(d.detector), ok);
}
bool Escudo::init(ModuleContext &c) {
  context = &c;
  capture_engine.init(c);
  capture_engine.observe(
      [](void *arg, const Observation &o) {
        auto &self = *static_cast<Escudo *>(arg);
        self.detectors.ingest(o);
        note_detector_input();
        if (self.external_observer)
          self.external_observer(self.external_observer_arg, o);
      },
      this);
  detectors.observe(
      [](void *arg, const Detection &d) {
        static_cast<Escudo *>(arg)->detection(d);
      },
      this);
  return true;
}
bool Escudo::start() {
  if (!context)
    return false;
  active = true;
  stop_requested = resume_after_scan = false;
  networks_pending = target_pending = capture_cleanup = false;
  diagnostic = nullptr;
  diagnostic_resource = "wifi";
  scan_needs_restart = false;
  scan_status = "idle";
  scan_deadline = 0;
  catalog.reset(nullptr);
  detectors.reset();
  ScanNetwork restored;
  auto loaded = load_escudo_target(restored);
  if (loaded == EscudoTargetLoad::Loaded) {
    selected = {restored, 0};
    has_target = true;
  } else if (loaded == EscudoTargetLoad::Error) {
    diagnostic = "resource_error";
    diagnostic_resource = "system";
  }
  // A transient read failure must not discard a selection already accepted in
  // this boot. Even cached configuration is no longer a current observation.
  if (has_target) {
    selected.seen = 0;
    selected.network.rssi = 0;
    select_baseline(false);
  }
  return true;
}
void Escudo::select_baseline(bool observed) {
  Observation baseline;
  baseline.ssid_present = baseline.bssid_known = true;
  baseline.ssid_length = strnlen(selected.network.ssid, 32);
  memcpy(baseline.ssid, selected.network.ssid, baseline.ssid_length);
  memcpy(baseline.bssid, selected.network.bssid, 6);
  baseline.channel = selected.network.channel;
  baseline.rssi = selected.network.rssi;
  baseline.auth = auth_label(selected.network.auth);
  baseline.cipher = cipher_label(selected.network.pairwise_cipher,
                                 selected.network.group_cipher);
  // UINT64_MAX marks configuration without a temporal baseline. compare()
  // rejects it as later than any frame, until a real target beacon/response
  // replaces it. Deauth/Karma still use the selected identity/security context.
  baseline.uptime_ms = observed ? selected.seen : UINT64_MAX;
  detectors.select(baseline);
}
void Escudo::finish_stop(uint64_t now) {
  if (!capture_engine.stop(now))
    return;
  active = false;
  scan_status = "idle";
  scan_deadline = 0;
  networks_pending = target_pending = false;
  diagnostic = nullptr;
}
void Escudo::request_stop() {
  stop_requested = true;
  resume_after_scan = false;
  // El gestor posee el scan; captura debe confirmar primero la liberación de su
  // generación. tick reintenta una parada fallida, con plazo del supervisor.
  finish_stop(context ? context->now_ms() : 0);
}
void Escudo::complete_scan(const RadioEvent &event) {
  if (strcmp(scan_status, "running"))
    return;
  if (event.scan_status) {
    scan_status = "error";
    diagnostic = "resource_error";
    return;
  }
  ScanNetwork records[CatalogCapacity];
  size_t count = 0;
  if (!context->resources().read_scan_results(records, CatalogCapacity,
                                              count) ||
      count > CatalogCapacity) {
    scan_status = "error";
    diagnostic = "resource_error";
    return;
  }
  for (size_t i = 0; i < count; ++i)
    catalog.add(records[i], event.uptime_ms);
  scan_status = "complete";
  networks_pending = true;
  if (event.scan_count > CatalogCapacity || catalog.dropped())
    diagnostic = "capacity_exceeded";
}
void Escudo::flush_event() {
  if (diagnostic) {
    char details[100];
    snprintf(details, sizeof(details),
             "{\"status_code\":\"%s\",\"resource\":\"%s\"}", diagnostic,
             diagnostic_resource);
    if (context->emit_event("system.status", "warning", details)) {
      diagnostic = nullptr;
      diagnostic_resource = "wifi";
    }
    return;
  }
  if (networks_pending) {
    cJSON after = {};
    after.type = cJSON_NULL;
    cJSON *page = nullptr;
    if (auto error = catalog.page(&after, 4, page)) {
      diagnostic = error;
      networks_pending = false;
      return;
    }
    char json[ModuleJsonBytes + 1];
    if (!print(page, json, sizeof(json))) {
      diagnostic = "capacity_exceeded";
      networks_pending = false;
      return;
    }
    if (context->emit_event("escudo.networks", "info", json))
      networks_pending = false;
  } else if (target_pending) {
    auto target = Catalog::target_json(selected);
    cJSON_AddStringToObject(target, "capture_status", capture_engine.status());
    char json[ModuleJsonBytes + 1];
    if (print(target, json, sizeof(json)) &&
        context->emit_event("escudo.target", "info", json))
      target_pending = false;
  }
}
void Escudo::tick(uint64_t now) {
  if (!active)
    return;
  if (stop_requested) {
    finish_stop(now);
    return;
  }
  if (capture_cleanup) {
    capture_cleanup = !capture_engine.stop(now);
    if (capture_cleanup) {
      flush_event();
      return;
    }
  }
  RadioEvent event;
  for (size_t i = 0; i < RadioQueueSize && context->resources().poll(event);
       ++i)
    if (event.kind == RadioEvent::Kind::ScanComplete)
      complete_scan(event);
    else if (event.kind == RadioEvent::Kind::Frame)
      capture_engine.ingest(event);
  if (!strcmp(scan_status, "running") && now >= scan_deadline) {
    scan_status = "error";
    scan_needs_restart = true;
    diagnostic = "timeout";
    resume_after_scan = false;
  }
  if (resume_after_scan && strcmp(scan_status, "running") &&
      !scan_needs_restart) {
    resume_after_scan = false;
    if (!capture_engine.start(selected.network.channel, now))
      diagnostic = "resource_error";
  }
  capture_engine.tick(now);
  flush_event();
}
void Escudo::handle(const ModuleCommand &cmd, ModuleReply &reply) {
  if (!active) {
    reply.error = "invalid_state";
    return;
  }
  if (capture_cleanup && strcmp(cmd.operation, "escudo.capture_stop")) {
    reply.error = "resource_error";
    return;
  }
  if (!strcmp(cmd.operation, "escudo.scan")) {
    if (scan_needs_restart) {
      reply.error = "invalid_state";
      return;
    }
    if (!strcmp(scan_status, "running") || networks_pending || target_pending ||
        diagnostic) {
      reply.error = "busy";
      return;
    }
    resume_after_scan = capture_engine.running();
    if (resume_after_scan &&
        !capture_engine.stop(context->now_ms(), "channel_hop", true)) {
      resume_after_scan = false;
      reply.error = "resource_error";
      return;
    }
    if (!context->resources().scan_passive()) {
      if (resume_after_scan) {
        resume_after_scan = false;
        if (!capture_engine.start(selected.network.channel, context->now_ms()))
          diagnostic = "resource_error";
      }
      reply.error = "resource_error";
      return;
    }
    catalog.reset(cmd.command_id);
    scan_status = "running";
    scan_deadline = context->now_ms() + 10000;
    reply.job = true;
  } else if (!strcmp(cmd.operation, "escudo.networks")) {
    if (strcmp(scan_status, "complete")) {
      reply.error = !strcmp(scan_status, "running") ? "busy" : "invalid_state";
      return;
    }
    auto limit = cJSON_GetObjectItemCaseSensitive(cmd.args, "limit");
    if (!cJSON_IsNumber(limit) || limit->valuedouble < 1 ||
        limit->valuedouble > 4 || limit->valuedouble != limit->valueint) {
      reply.error = "invalid_args";
      return;
    }
    cJSON *page = nullptr;
    reply.error =
        catalog.page(cJSON_GetObjectItemCaseSensitive(cmd.args, "after"),
                     limit->valueint, page);
    if (!reply.error) {
      cJSON_AddStringToObject(page, "kind", "escudo.networks");
      if (!print(page, reply.result, sizeof(reply.result)))
        reply.error = "capacity_exceeded";
    }
  } else if (!strcmp(cmd.operation, "escudo.select_target")) {
    if (strcmp(scan_status, "complete")) {
      reply.error = !strcmp(scan_status, "running") ? "busy" : "invalid_state";
      return;
    }
    if (target_pending) {
      reply.error = "busy";
      return;
    }
    if (capture_engine.needs_release() && !capture_engine.running()) {
      reply.error = "invalid_state";
      return;
    }
    auto found = catalog.find(cmd.args);
    if (!found) {
      reply.error = "invalid_args";
      return;
    }
    auto body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "kind", "escudo.target");
    auto target = Catalog::target_json(*found);
    cJSON_AddStringToObject(target, "capture_status", capture_engine.status());
    cJSON_AddItemToObject(body, "target", target);
    if (!print(body, reply.result, sizeof(reply.result))) {
      reply.error = "capacity_exceeded";
      return;
    }
    bool was_running = capture_engine.running();
    if (was_running) {
      if (!capture_engine.stop(context->now_ms(), "target_change", true)) {
        capture_cleanup = true;
        reply.error = "resource_error";
        return;
      }
      if (!capture_engine.start(found->network.channel, context->now_ms())) {
        capture_cleanup = !capture_engine.stop(context->now_ms());
        reply.error = "resource_error";
        return;
      }
    } else if (!context->resources().set_channel(found->network.channel)) {
      reply.error = "resource_error";
      return;
    }
    // The supervisor cannot dispatch frames while this bounded NVS operation
    // runs. Stage radio first, commit before publishing the new target, and
    // invalidate/stop that capture if persistence fails (retry failed release).
    if (!save_escudo_target(found->network)) {
      resume_after_scan = false;
      if (was_running)
        capture_cleanup = !capture_engine.stop(context->now_ms());
      diagnostic = "resource_error";
      diagnostic_resource = "system";
      reply.error = "resource_error";
      return;
    }
    selected = *found;
    select_baseline(true);
    has_target = true;
    target_pending = true;
  } else if (!strcmp(cmd.operation, "escudo.capture_start")) {
    if (!has_target || scan_needs_restart) {
      reply.error = "invalid_state";
      return;
    }
    if (!strcmp(scan_status, "running")) {
      reply.error = "busy";
      return;
    }
    if (!capture_engine.start(selected.network.channel, context->now_ms()))
      reply.error = "resource_error";
  } else if (!strcmp(cmd.operation, "escudo.capture_stop")) {
    resume_after_scan = false;
    capture_cleanup = !capture_engine.stop(context->now_ms());
    if (capture_cleanup)
      reply.error = "resource_error";
  } else
    reply.error = "not_implemented";
}
void Escudo::snapshot(ModuleSnapshot &out) const {
  auto j = cJSON_CreateObject();
  cJSON_AddStringToObject(j, "kind", "escudo");
  cJSON_AddStringToObject(j, "scan_status", scan_status);
  if (*catalog.id())
    cJSON_AddStringToObject(j, "scan_id", catalog.id());
  else
    cJSON_AddNullToObject(j, "scan_id");
  if (has_target)
    cJSON_AddItemToObject(j, "target", Catalog::target_json(selected));
  else
    cJSON_AddNullToObject(j, "target");
  cJSON_AddStringToObject(j, "capture_status", capture_engine.status());
  cJSON_AddItemToObject(
      j, "coverage", capture_engine.coverage(context ? context->now_ms() : 0));
  print(j, out.state, sizeof(out.state));
  out.capabilities = {};
  out.capabilities.capture = true;
  out.capabilities.detectors = true;
  out.target_selected = has_target;
}
} // namespace sentinel::escudo
extern "C" sentinel::Module *sentinel_escudo_module() {
  static sentinel::escudo::Escudo module;
  return &module;
}
