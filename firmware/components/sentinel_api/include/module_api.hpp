#pragma once
#include "cJSON.h"
#include <cstddef>
#include <cstdint>
struct esp_netif_obj;
namespace sentinel {
// No runtime dependency. All pointers are borrowed for the duration of a call.
// Modules keep their own bounded copies; they never retain command args/context
// input buffers. Module/context instances live for the entire boot.
enum class Mode { Idle, Escudo, Cebo, Sanitizador };
constexpr size_t ModuleJsonBytes = 1536, RadioQueueSize = 8;
struct ModuleCommand {
  const char *operation;
  const cJSON *args;
  const char *command_id;
};
struct ModuleReply {
  const char *error = nullptr; // static contract error code; never user input
  char result[ModuleJsonBytes + 1] =
      {};           // JSON result (including kind), or empty=null
  bool job = false; // core supplies job_id=command_id
};
struct ModuleCapabilities {
  bool capture = false, detectors = false, honeypot_http = false,
       honeypot_telnet = false;
  bool notifications = false, sanitizador = false;
};
struct ModuleSnapshot {
  char state[ModuleJsonBytes + 1] =
      {}; // exact V2{Escudo,Sanitizador,Cebo}Module
  ModuleCapabilities capabilities;
  bool target_selected = false;
};
struct UpstreamConfig {
  char ssid[33] = {}, password[64] = {};
};
struct ApConfig {
  char ssid[33] = {}, password[64] = {};
  uint32_t ipv4 = 0, netmask = 0;
};
struct ScanNetwork {
  char ssid[33] = {};
  uint8_t bssid[6] = {}, channel = 0;
  int8_t rssi = 0;
  unsigned auth = 0, pairwise_cipher = 0, group_cipher = 0;
};
struct RadioEvent {
  enum class Kind {
    Frame,
    ScanComplete,
    Connected,
    Disconnected,
    GotIp,
    ApClientConnected,
    ApClientDisconnected
  } kind;
  uint32_t ipv4 = 0, netmask = 0, gateway = 0;
  uint8_t peer[6] = {};
  uint64_t uptime_ms = 0;
  uint8_t channel = 0;
  int8_t rssi = 0;
  uint16_t length = 0, original_length = 0;
  uint8_t bytes[512] = {};
};
class ModuleResources {
public:
  virtual ~ModuleResources() = default;
  virtual bool set_channel(uint8_t) = 0;
  virtual bool scan_passive() = 0;
  virtual size_t scan_results(ScanNetwork *, size_t capacity) = 0;
  virtual bool capture(bool enabled) = 0;
  virtual bool configure_upstream(const UpstreamConfig &) = 0;
  virtual bool configure_ap(const ApConfig &) = 0;
  virtual bool napt(bool enabled) = 0;
  // Borrowed handles: never destroy/replace them or start/stop Wi-Fi drivers.
  virtual esp_netif_obj *sta_netif() const = 0;
  virtual esp_netif_obj *ap_netif() const = 0;
  virtual bool
  poll(RadioEvent &) = 0; // supervisor thread only; copied bounded data
  virtual uint64_t dropped() const = 0;
};
class ModuleContext {
public:
  virtual ~ModuleContext() = default;
  virtual uint64_t now_ms() const = 0;
  virtual ModuleResources &resources() = 0;
  // Copy/validate before returning; no flash or transport writes on this call.
  // Details/values are exact v2 JSON objects; false means
  // invalid/inactive/full.
  virtual bool emit_event(const char *name, const char *severity,
                          const char *details, bool simulated = false) = 0;
  virtual bool publish_metrics(uint32_t interval_ms, const char *values) = 0;
};
class Module {
public:
  virtual ~Module() = default;
  virtual bool init(ModuleContext &) = 0;
  virtual bool start() = 0;
  virtual void request_stop() = 0;
  virtual bool stopped() const = 0;
  virtual void handle(const ModuleCommand &, ModuleReply &) = 0;
  virtual void tick(uint64_t) = 0;
  virtual void snapshot(ModuleSnapshot &) const = 0;
};
// Hooks execute on the supervisor thread and must return promptly. request_stop
// initiates cleanup; stopped stays false until sockets/tasks release resources.
// Callback workers may only enqueue through context hooks, never invoke
// drivers.
} // namespace sentinel
