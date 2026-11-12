#include "escudo.hpp"
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
bool Escudo::init(ModuleContext &c) {
  context = &c;
  return true;
}
bool Escudo::start() {
  if (!context)
    return false;
  active = true;
  has_target = networks_pending = target_pending = false;
  diagnostic = nullptr;
  scan_needs_restart = false;
  scan_status = "idle";
  scan_deadline = 0;
  catalog.reset(nullptr);
  return true;
}
void Escudo::request_stop() {
  // The module has no workers/services; the manager owns/cancels the physical
  // scan when the supervisor stops Wi-Fi, before granting the next mode.
  active = false;
  scan_status = "idle";
  scan_deadline = 0;
  networks_pending = target_pending = false;
  diagnostic = nullptr;
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
             "{\"status_code\":\"%s\",\"resource\":\"wifi\"}", diagnostic);
    if (context->emit_event("system.status", "warning", details))
      diagnostic = nullptr;
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
    cJSON_AddStringToObject(target, "capture_status", "stopped");
    char json[ModuleJsonBytes + 1];
    if (print(target, json, sizeof(json)) &&
        context->emit_event("escudo.target", "info", json))
      target_pending = false;
  }
}
void Escudo::tick(uint64_t now) {
  if (!active)
    return;
  RadioEvent event;
  for (size_t i = 0; i < RadioQueueSize && context->resources().poll(event);
       ++i)
    if (event.kind == RadioEvent::Kind::ScanComplete)
      complete_scan(event);
  if (!strcmp(scan_status, "running") && now >= scan_deadline) {
    scan_status = "error";
    scan_needs_restart = true;
    diagnostic = "timeout";
  }
  flush_event();
}
void Escudo::handle(const ModuleCommand &cmd, ModuleReply &reply) {
  if (!active) {
    reply.error = "invalid_state";
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
    if (!context->resources().scan_passive()) {
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
    auto found = catalog.find(cmd.args);
    if (!found) {
      reply.error = "invalid_args";
      return;
    }
    auto body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "kind", "escudo.target");
    auto target = Catalog::target_json(*found);
    cJSON_AddStringToObject(target, "capture_status", "stopped");
    cJSON_AddItemToObject(body, "target", target);
    if (!print(body, reply.result, sizeof(reply.result))) {
      reply.error = "capacity_exceeded";
      return;
    }
    if (!context->resources().set_channel(found->network.channel)) {
      reply.error = "resource_error";
      return;
    }
    selected = *found;
    has_target = true;
    target_pending = true;
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
  cJSON_AddStringToObject(j, "capture_status", "stopped");
  auto coverage = cJSON_AddObjectToObject(j, "coverage");
  cJSON_AddNullToObject(coverage, "channel");
  cJSON_AddStringToObject(coverage, "pause_reason", "stopped");
  for (auto name :
       {"started_uptime_ms", "duration_ms", "dropped_frames", "dropped_events"})
    cJSON_AddNumberToObject(coverage, name, 0);
  print(j, out.state, sizeof(out.state));
  out.capabilities = {};
  out.target_selected = has_target;
}
} // namespace sentinel::escudo
extern "C" sentinel::Module *sentinel_escudo_module() {
  static sentinel::escudo::Escudo module;
  return &module;
}
