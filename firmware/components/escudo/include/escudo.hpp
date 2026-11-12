#pragma once
#include "capture.hpp"
#include "detectors.hpp"
#include "module_api.hpp"
namespace sentinel::escudo {
constexpr size_t CatalogCapacity = 32;
struct Entry {
  ScanNetwork network;
  uint64_t seen = 0;
};
class Catalog {
  Entry entries[CatalogCapacity] = {};
  size_t used = 0;
  uint64_t overflow = 0;
  char scan[65] = {};
  uint64_t issued = 0;

public:
  void reset(const char *scan_id);
  void add(const ScanNetwork &, uint64_t seen);
  size_t size() const { return used; }
  uint64_t dropped() const { return overflow; }
  const char *id() const { return scan; }
  const Entry *find(const cJSON *target) const;
  const char *page(const cJSON *after, size_t limit, cJSON *&result);
  static cJSON *network_json(const Entry &);
  static cJSON *target_json(const Entry &);
};
class Escudo final : public Module {
  ModuleContext *context = nullptr;
  Catalog catalog;
  Capture capture_engine;
  Detectors detectors;
  ObservationSink external_observer = nullptr;
  void *external_observer_arg = nullptr;
  void detection(const Detection &);
  bool stop_requested = false, resume_after_scan = false;
  bool active = false, has_target = false, networks_pending = false,
       target_pending = false;
  bool scan_needs_restart = false;
  const char *scan_status = "idle";
  uint64_t scan_deadline = 0;
  Entry selected = {};
  const char *diagnostic = nullptr;
  void complete_scan(const RadioEvent &);
  void finish_stop(uint64_t);
  void flush_event();

public:
  void observe(ObservationSink sink, void *arg) {
    external_observer = sink;
    external_observer_arg = arg;
  }
  bool init(ModuleContext &) override;
  bool start() override;
  void request_stop() override;
  bool stopped() const override { return !active; }
  void handle(const ModuleCommand &, ModuleReply &) override;
  void tick(uint64_t) override;
  void snapshot(ModuleSnapshot &) const override;
};
const char *auth_label(unsigned);
const char *cipher_label(unsigned pairwise, unsigned group);
} // namespace sentinel::escudo
extern "C" sentinel::Module *sentinel_escudo_module();
