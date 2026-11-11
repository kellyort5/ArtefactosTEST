// Intentionally includes ONLY the public API: catches a hidden runtime
// dependency.
#include "module_api.hpp"
#include <cstring>
class Consumer final : public sentinel::Module {
  sentinel::ModuleContext *context = nullptr;
  bool running = false;

public:
  bool init(sentinel::ModuleContext &c) override {
    context = &c;
    return true;
  }
  bool start() override {
    running = context != nullptr;
    return running;
  }
  void request_stop() override { running = false; }
  bool stopped() const override { return !running; }
  void handle(const sentinel::ModuleCommand &,
              sentinel::ModuleReply &r) override {
    r.error = "not_implemented";
  }
  void tick(uint64_t) override {}
  void snapshot(sentinel::ModuleSnapshot &s) const override {
    // No services are implemented by this local test consumer.
    std::strcpy(s.state,
                "{\"kind\":\"cebo\",\"http\":{\"available\":false,\"status\":"
                "\"unavailable\"},\"telnet\":{\"available\":false,\"status\":"
                "\"unavailable\"},\"active_connections\":0,\"max_connections\":"
                "4,\"idle_timeout_ms\":10000}");
  }
};
sentinel::Module &test_consumer() {
  static Consumer consumer;
  return consumer;
}
