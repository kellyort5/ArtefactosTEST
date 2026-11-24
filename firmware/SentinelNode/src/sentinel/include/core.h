#pragma once
#include "cJSON.h"
#include "../../sentinel_api/include/module_api.hpp"
#include <cstddef>
#include <cstdint>
namespace sentinel {
constexpr size_t MaxMessage = 2048, QueueSize = 8, CacheSize = 32,
                 RingSize = 64;
constexpr uint64_t MaxInteger = 9007199254740991ULL;
uint64_t monotonic_ms();
void wipe(void *p, size_t n);
bool id_valid(const char *s);
bool json_text_valid(const char *, size_t, bool extended = false);
bool utc_valid(const char *s);
bool mqtt_uri_valid(const char *s);
bool integer(const cJSON *j, uint64_t min = 0, uint64_t max = MaxInteger);
bool fields(const cJSON *o,
            const char *names); // exact, no duplicate/unknown keys
const char *str(const cJSON *o, const char *key);
uint64_t number(const cJSON *o, const char *key);
enum class Channel { Ble, Mqtt };
class SlotPool {
  bool slots[QueueSize] = {};

public:
  int acquire();
  void release(size_t slot);
};
class PendingCommands {
  struct Entry {
    char id[65] = {};
    uint8_t digest[32] = {};
    Channel channel = Channel::Ble;
    uint32_t session = 0;
  };
  SlotPool pool;
  Entry entries[QueueSize];

public:
  // >=0 reserved slot; -1 full; -2 same command already queued; -3 conflicting
  // ID.
  int admit(const char *id, const uint8_t digest[32], Channel, uint32_t session,
            uint32_t current_ble_session);
  void release(size_t slot);
};
struct Command {
  cJSON *root = nullptr, *args = nullptr;
  char id[65] = {}, operation[32] = {};
  uint64_t revision = 0, expiry = 0;
  unsigned version = 1;
  const char *parse_v2(const char *, size_t);
  ~Command();
  const char *parse(const char *bytes, size_t n);
};
bool digest_command(const Command &, uint8_t out[32]);
const char *operation_error(const Command &);
struct Result {
  char id[65] = {};
  const char *error = nullptr;
  uint64_t revision = 0;
  bool page = false, state = false;
  uint64_t next = 0, from = 0, to = 0;
  bool more = false, gap = false;
  uint64_t cursors[8] = {};
  size_t count = 0;
  char body[ModuleJsonBytes + 1] = {};
};
struct CacheEntry {
  bool used = false;
  char id[65] = {};
  uint8_t digest[32] = {};
  Result result;
};
class CommandGate {
  CacheEntry entries[CacheSize] = {};
  size_t next = 0;

public:
  // Freshness is checked before cache. Revision is checked only for new orders.
  const char *check(const Command &, const char *device, const char *boot,
                    uint64_t revision, uint64_t now, bool authorized,
                    bool retained, const uint8_t digest[32],
                    const Result **cached,
                    const char *admission_error = nullptr);
  void remember(const Command &, const uint8_t digest[32], const Result &);
  void remember_id(const char *id, const uint8_t digest[32], const Result &);
  const char *lookup(const char *id, const uint8_t digest[32],
                     const Result **result) const;
};
struct Frame {
  uint8_t bytes[MaxMessage + 1] = {};
  uint32_t transfer = 0;
  uint16_t total = 0, used = 0;
  uint64_t started = 0;
  void reset();
  // -1 invalid; 0 incomplete; 1 complete. Caller consumes and resets complete
  // frame.
  int push(const uint8_t *data, size_t n, uint64_t now);
};
struct Button {
  bool sampled = false, stable = false, long_sent = false;
  uint64_t changed = 0, pressed = 0;
  // 0 no action, 1 short release, 2 long hold
  int update(bool down, uint64_t now);
};
class Store {
public:
  virtual ~Store() = default;
  virtual bool read(size_t slot, char *out, size_t *n) = 0;
  virtual bool write(size_t slot, const char *data, size_t n) = 0;
  virtual bool missing() const = 0;
};
class Journal {
  Store &store;
  size_t capacity;
  uint64_t slots[RingSize] = {};

public:
  bool degraded = false;
  uint64_t highest = 0;
  explicit Journal(Store &s, size_t cap = RingSize)
      : store(s), capacity(cap <= RingSize && cap ? cap : RingSize) {}
  void recover();
  bool append(const char *event, size_t n, uint64_t cursor);
  bool read(uint64_t cursor, char *out, size_t *n);
  uint64_t first() const;
  size_t count() const;
  Result page(uint64_t after, size_t limit, uint64_t *cursors, size_t *n) const;
};
using Profile = Mode;
const char *profile_name(Profile);
const char *requested_profile(const char *mode, Profile *out);
struct Resources {
  virtual ~Resources() = default;
  virtual bool stop_network() = 0;
  // Solo indica una parada pendiente; false conserva los adaptadores síncronos.
  virtual bool network_stopping() const { return false; }
  virtual bool network_released() const = 0;
  virtual bool start_cebo() = 0;
  virtual bool start_profile(Profile p) {
    return p != Profile::Cebo || start_cebo();
  }
};
struct Runtime {
  Profile profile = Profile::Idle;
  const char *phase = "idle", *error = nullptr;
  uint64_t revision = 0;
  bool healthy = true;
  char last_command[65] = {};
  const char *transition(Profile target, Resources &resources);
};
} // namespace sentinel
