#include "include/core.h"
#include "include/protocol_v2.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#ifdef ESP_PLATFORM
#include "lwip/inet.h"
#include "lwip/sockets.h"
#else
#include <arpa/inet.h>
#endif
namespace sentinel {
void wipe(void *p, size_t n) {
  // volatile impide que el compilador elimine el borrado de copias secretas.
  volatile uint8_t *v = static_cast<volatile uint8_t *>(p);
  while (n--)
    *v++ = 0;
}
bool id_valid(const char *s) {
  if (!s || !*s || strlen(s) > 64)
    return false;
  for (; *s; ++s)
    if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') ||
          (*s >= '0' && *s <= '9') || strchr("._:-", *s)))
      return false;
  return true;
}
bool integer(const cJSON *j, uint64_t min, uint64_t max) {
  return cJSON_IsNumber(j) && std::isfinite(j->valuedouble) &&
         j->valuedouble >= min && j->valuedouble <= max &&
         floor(j->valuedouble) == j->valuedouble;
}
const char *str(const cJSON *o, const char *k) {
  auto j = cJSON_GetObjectItemCaseSensitive(o, k);
  return cJSON_IsString(j) ? j->valuestring : nullptr;
}
uint64_t number(const cJSON *o, const char *k) {
  auto j = cJSON_GetObjectItemCaseSensitive(o, k);
  return integer(j) ? static_cast<uint64_t>(j->valuedouble) : 0;
}
bool fields(const cJSON *o, const char *names) {
  if (!cJSON_IsObject(o))
    return false;
  size_t wanted = *names ? 1 : 0;
  for (const char *p = names; *p; ++p)
    if (*p == ',')
      ++wanted;
  size_t actual = 0;
  for (auto j = o->child; j; j = j->next) {
    ++actual;
    if (!j->string)
      return false;
    bool found = false;
    for (const char *p = names; *p;) {
      const char *e = strchr(p, ',');
      size_t n = e ? static_cast<size_t>(e - p) : strlen(p);
      if (strlen(j->string) == n && !memcmp(p, j->string, n))
        found = true;
      if (!e)
        break;
      p = e + 1;
    }
    if (!found)
      return false;
    for (auto k = j->next; k; k = k->next)
      if (k->string && !strcmp(j->string, k->string))
        return false;
  }
  return wanted == actual;
}
bool utc_valid(const char *s) {
  if (!s)
    return false;
  // Calendario UTC con fracciones dentro del límite compartido de 32 caracteres.
  size_t n = strlen(s);
  if (n != 20 && (n < 22 || n > 32))
    return false;
  if (s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' ||
      s[16] != ':' || s[n - 1] != 'Z')
    return false;
  for (size_t i = 0; i < 19; ++i)
    if (i != 4 && i != 7 && i != 10 && i != 13 && i != 16 &&
        !isdigit(static_cast<unsigned char>(s[i])))
      return false;
  if (n > 20) {
    if (s[19] != '.')
      return false;
    for (size_t i = 20; i < n - 1; ++i)
      if (!isdigit(static_cast<unsigned char>(s[i])))
        return false;
  }
  int y = atoi(s), m = atoi(s + 5), d = atoi(s + 8), h = atoi(s + 11),
      minute = atoi(s + 14), sec = atoi(s + 17);
  if (y < 1 || m < 1 || m > 12 || h > 23 || minute > 59 || sec > 59)
    return false;
  int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)
    days[1] = 29;
  return d >= 1 && d <= days[m - 1];
}
bool mqtt_uri_valid(const char *s) {
  if (!s || strlen(s) > 256 || strncmp(s, "mqtt://", 7))
    return false;
  const char *p = s + 7;
  if (!*p)
    return false;
  if (*p == '[') {
    const char *end = strchr(p, ']');
    if (!end || end - p > 45)
      return false;
    char literal[46] = {};
    memcpy(literal, p + 1, end - p - 1);
    unsigned char address[16];
    if (inet_pton(AF_INET6, literal, address) != 1)
      return false;
    p = end + 1;
  } else {
    const char *begin = p;
    while (*p && *p != ':') {
      if (!isalnum(static_cast<unsigned char>(*p)) && *p != '-' && *p != '.')
        return false;
      ++p;
    }
    if (p == begin || !isalnum(static_cast<unsigned char>(*begin)) ||
        !isalnum(static_cast<unsigned char>(p[-1])))
      return false;
  }
  if (*p) {
    if (*p++ != ':' || !*p)
      return false;
    unsigned port = 0, digits = 0;
    while (*p) {
      if (!isdigit(static_cast<unsigned char>(*p)) || ++digits > 5)
        return false;
      port = port * 10 + (*p++ - '0');
    }
    if (!port || port > 65535)
      return false;
  }
  return true;
}
static bool text(const cJSON *o, const char *k, size_t min, size_t max) {
  auto s = str(o, k);
  return s && strlen(s) >= min && strlen(s) <= max;
}
static bool characters(const cJSON *o, const char *k, size_t min, size_t max) {
  const char *s = str(o, k);
  if (!s)
    return false;
  size_t n = 0;
  for (; *s; ++s)
    if ((static_cast<unsigned char>(*s) & 0xc0) != 0x80)
      ++n;
  return n >= min && n <= max;
}
bool json_text_valid(const char *bytes, size_t n, bool extended) {
  if (!bytes || !n || n > MaxMessage)
    return false;
  bool quoted = false;
  unsigned depth = 0;
  for (size_t i = 0; i < n; ++i) {
    unsigned char c = bytes[i];
    if (c >= 0x80) {
      unsigned count = 0;
      uint32_t cp = 0, min = 0;
      if (c >= 0xc2 && c <= 0xdf) {
        count = 1;
        cp = c & 31;
        min = 0x80;
      } else if (c >= 0xe0 && c <= 0xef) {
        count = 2;
        cp = c & 15;
        min = 0x800;
      } else if (c >= 0xf0 && c <= 0xf4) {
        count = 3;
        cp = c & 7;
        min = 0x10000;
      } else
        return false;
      if (i + count >= n)
        return false;
      while (count--) {
        unsigned char next = bytes[++i];
        if ((next & 0xc0) != 0x80)
          return false;
        cp = (cp << 6) | (next & 63);
      }
      if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        return false;
      continue;
    }
    if (!c)
      return false;
    if (!quoted) {
      if (!extended && (c == '[' || c == ']'))
        return false;
      if ((c == '{' || (extended && c == '[')) &&
          ++depth > (extended ? 32U : 3U))
        return false;
      if (c == '}' || (extended && c == ']')) {
        if (!depth)
          return false;
        --depth;
      }
    }
    if (!quoted && c < 0x20 && c != '\t' && c != '\r' && c != '\n')
      return false;
    if (!quoted && (c == '-' || (c >= '0' && c <= '9'))) {
      size_t at = i;
      if (bytes[at] == '-' && ++at >= n)
        return false;
      if (bytes[at] == '0')
        ++at;
      else {
        if (bytes[at] < '1' || bytes[at] > '9')
          return false;
        while (at < n && bytes[at] >= '0' && bytes[at] <= '9')
          ++at;
      }
      if (at < n && bytes[at] == '.') {
        ++at;
        size_t start = at;
        while (at < n && bytes[at] >= '0' && bytes[at] <= '9')
          ++at;
        if (at == start)
          return false;
      }
      if (at < n && (bytes[at] == 'e' || bytes[at] == 'E')) {
        ++at;
        if (at < n && (bytes[at] == '+' || bytes[at] == '-'))
          ++at;
        size_t start = at;
        while (at < n && bytes[at] >= '0' && bytes[at] <= '9')
          ++at;
        if (at == start)
          return false;
      }
      if (at < n && !strchr(" ,}]:\t\r\n", bytes[at]))
        return false;
      i = at - 1;
      continue;
    }
    if (quoted && c < 0x20)
      return false;
    if (c == '"')
      quoted = !quoted;
    else if (quoted && c == '\\') {
      if (++i >= n)
        return false;
      if (bytes[i] == 'u' && i + 4 < n && !memcmp(bytes + i + 1, "0000", 4))
        return false;
    }
  }
  return !quoted && !depth;
}
Command::~Command() {
  if (root) {
    auto p = cJSON_GetObjectItemCaseSensitive(root, "payload");
    auto a = cJSON_GetObjectItemCaseSensitive(p, "args");
    if (a)
      for (auto j = a->child; j; j = j->next)
        if (cJSON_IsString(j))
          wipe(j->valuestring, strlen(j->valuestring));
    cJSON_Delete(root);
  }
}
int SlotPool::acquire() {
  for (size_t i = 0; i < QueueSize; ++i)
    if (!slots[i]) {
      slots[i] = true;
      return i;
    }
  return -1;
}
void SlotPool::release(size_t slot) {
  if (slot < QueueSize)
    slots[slot] = false;
}
int PendingCommands::admit(const char *id, const uint8_t hash[32],
                           Channel channel, uint32_t session,
                           uint32_t current) {
  if (id && *id)
    for (auto &e : entries)
      if (*e.id && !strcmp(e.id, id)) {
        if (e.channel == Channel::Ble && e.session != current)
          continue;
        return memcmp(e.digest, hash, 32) ? -3 : -2;
      }
  int index = pool.acquire();
  if (index < 0)
    return -1;
  auto &e = entries[index];
  if (id)
    snprintf(e.id, sizeof(e.id), "%s", id);
  if (hash)
    memcpy(e.digest, hash, 32);
  e.channel = channel;
  e.session = session;
  return index;
}
void PendingCommands::release(size_t slot) {
  if (slot < QueueSize) {
    wipe(entries[slot].id, sizeof(entries[slot].id));
    wipe(entries[slot].digest, sizeof(entries[slot].digest));
    pool.release(slot);
  }
}
const char *Command::parse(const char *bytes, size_t n) {
  if (!bytes || !n || n > MaxMessage || !json_text_valid(bytes, n))
    return "invalid_message";
  const char *end = nullptr;
  root = cJSON_ParseWithLengthOpts(bytes, n + 1, &end, true);
  if (!root || end != bytes + n)
    return "invalid_message";
  auto correlation = cJSON_GetObjectItemCaseSensitive(root, "payload");
  if (id_valid(str(correlation, "command_id")))
    snprintf(id, sizeof(id), "%s", str(correlation, "command_id"));
  auto v = cJSON_GetObjectItemCaseSensitive(root, "version");
  if (!integer(v) || v->valuedouble != 1)
    return "unsupported_version";
  if (!fields(root, "version,type,device_id,boot_id,message_id,seq,uptime_ms,"
                    "timestamp,time_quality,payload") ||
      !str(root, "type") || strcmp(str(root, "type"), "command") ||
      !id_valid(str(root, "device_id")) || !id_valid(str(root, "boot_id")) ||
      !id_valid(str(root, "message_id")) ||
      !integer(cJSON_GetObjectItemCaseSensitive(root, "seq")) ||
      !integer(cJSON_GetObjectItemCaseSensitive(root, "uptime_ms")))
    return "invalid_message";
  auto ts = cJSON_GetObjectItemCaseSensitive(root, "timestamp");
  auto q = str(root, "time_quality");
  if (!q ||
      (strcmp(q, "unknown") && strcmp(q, "app_sync") &&
       strcmp(q, "network_sync")) ||
      (!strcmp(q, "unknown") ? !cJSON_IsNull(ts)
                             : !utc_valid(str(root, "timestamp"))))
    return "invalid_message";
  auto p = cJSON_GetObjectItemCaseSensitive(root, "payload");
  if (!fields(p, "command_id,target_boot_id,expected_revision,expires_at_"
                 "uptime_ms,operation,args") ||
      !id_valid(str(p, "command_id")) || !id_valid(str(p, "target_boot_id")) ||
      !integer(cJSON_GetObjectItemCaseSensitive(p, "expected_revision")) ||
      !integer(cJSON_GetObjectItemCaseSensitive(p, "expires_at_uptime_ms")) ||
      !text(p, "operation", 1, 31))
    return "invalid_message";
  snprintf(id, sizeof(id), "%s", str(p, "command_id"));
  snprintf(operation, sizeof(operation), "%s", str(p, "operation"));
  revision = number(p, "expected_revision");
  expiry = number(p, "expires_at_uptime_ms");
  args = cJSON_GetObjectItemCaseSensitive(p, "args");
  if (!strcmp(operation, "set_mode")) {
    if (!fields(args, "mode") || !str(args, "mode") ||
        (strcmp(str(args, "mode"), "idle") &&
         strcmp(str(args, "mode"), "escudo") &&
         strcmp(str(args, "mode"), "cebo") &&
         strcmp(str(args, "mode"), "sanitizador")))
      return "invalid_message";
  } else if (!strcmp(operation, "configure_connectivity")) {
    if (!fields(args, "ssid,wifi_password,mqtt_uri,mqtt_username,mqtt_password") ||
        !text(args, "ssid", 1, 32) || !text(args, "wifi_password", 0, 63) ||
        !characters(args, "mqtt_username", 1, 64) ||
        !characters(args, "mqtt_password", 1, 128) ||
        !mqtt_uri_valid(str(args, "mqtt_uri")))
      return "invalid_message";
  } else if (!strcmp(operation, "set_time")) {
    if (!fields(args, "timestamp") || !utc_valid(str(args, "timestamp")))
      return "invalid_message";
  } else if (!strcmp(operation, "read_events")) {
    if (!fields(args, "after_cursor,limit") ||
        !integer(cJSON_GetObjectItemCaseSensitive(args, "after_cursor")) ||
        !integer(cJSON_GetObjectItemCaseSensitive(args, "limit"), 1, 8))
      return "invalid_message";
  } else if (!strcmp(operation, "get_state") ||
             !strcmp(operation, "start_capture") ||
             !strcmp(operation, "start_honeypot")) {
    if (!fields(args, ""))
      return "invalid_message";
  } else
    return "invalid_message";
  return nullptr;
}
const char *Command::parse_v2(const char *bytes, size_t n) {
  root = v2::parse(bytes, n);
  if (!root)
    return "invalid_message";
  auto p = cJSON_GetObjectItemCaseSensitive(root, "payload");
  if (id_valid(str(p, "command_id")))
    snprintf(id, sizeof(id), "%s", str(p, "command_id"));
  version = number(root, "version");
  if (version != 2)
    return "unsupported_version";
  if (!v2::validate("command", root))
    return "invalid_message";
  snprintf(operation, sizeof(operation), "%s", str(p, "operation"));
  revision = number(p, "expected_revision");
  expiry = number(p, "expires_at_uptime_ms");
  args = cJSON_GetObjectItemCaseSensitive(p, "args");
  return nullptr;
}
const char *operation_error(const Command &c) {
  if (!strcmp(c.operation, "start_capture") ||
      !strcmp(c.operation, "start_honeypot") ||
      (!strcmp(c.operation, "set_mode") &&
       !strcmp(str(c.args, "mode"), "sanitizador")))
    return "not_implemented";
  return nullptr;
}
const char *CommandGate::check(const Command &c, const char *device,
                               const char *boot, uint64_t rev, uint64_t now,
                               bool authorized, bool retained,
                               const uint8_t digest[32], const Result **cached,
                               const char *admission_error) {
  *cached = nullptr;
  if (!authorized)
    return "unauthorized";
  if (retained)
    return c.version == 2 ? "invalid_message" : "unauthorized";
  if (strcmp(str(c.root, "device_id"), device))
    return "unauthorized";
  auto p = cJSON_GetObjectItemCaseSensitive(c.root, "payload");
  if (strcmp(str(p, "target_boot_id"), boot) ||
      strcmp(str(c.root, "boot_id"), boot))
    return "wrong_boot";
  if (c.expiry <= now || c.expiry - now > 30000)
    return "expired";
  if (admission_error)
    return admission_error;
  // Seguridad y contexto se revalidan antes de reutilizar un resultado. La
  // revisión se comprueba después: repetir una orden aplicada debe ser inocuo.
  for (auto &e : entries)
    if (e.used && !strcmp(e.id, c.id)) {
      if (memcmp(e.digest, digest, 32))
        return "id_conflict";
      *cached = &e.result;
      return nullptr;
    }
  if (c.revision != rev)
    return "revision_conflict";
  return nullptr;
}
const char *CommandGate::lookup(const char *id, const uint8_t digest[32],
                                const Result **result) const {
  *result = nullptr;
  for (const auto &entry : entries)
    if (entry.used && !strcmp(entry.id, id)) {
      if (memcmp(entry.digest, digest, 32))
        return "id_conflict";
      *result = &entry.result;
      break;
    }
  return nullptr;
}
void CommandGate::remember(const Command &c, const uint8_t digest[32],
                           const Result &r) {
  remember_id(c.id, digest, r);
}
void CommandGate::remember_id(const char *id, const uint8_t digest[32],
                              const Result &r) {
  auto &e = entries[next++ % CacheSize];
  e.used = true;
  strcpy(e.id, id);
  memcpy(e.digest, digest, 32);
  e.result = r;
}
void Frame::reset() {
  wipe(bytes, sizeof(bytes));
  transfer = 0;
  total = used = 0;
  started = 0;
}
int Frame::push(const uint8_t *d, size_t n, uint64_t now) {
  auto bad = [this]() {
    reset();
    return -1;
  };
  if (n <= 8 || n > 514)
    return bad();
  uint32_t id = uint32_t(d[0]) | (uint32_t(d[1]) << 8) |
                (uint32_t(d[2]) << 16) | (uint32_t(d[3]) << 24);
  uint16_t offset = d[4] | (uint16_t(d[5]) << 8),
           size = d[6] | (uint16_t(d[7]) << 8);
  if (!size || size > MaxMessage || n - 8 > size || offset + n - 8 > size)
    return bad();
  if (total && now - started >= 10000)
    return bad();
  if (!total) {
    if (offset)
      return bad();
    transfer = id;
    total = size;
    started = now;
  }
  if (id != transfer || size != total || offset != used)
    return bad();
  memcpy(bytes + used, d + 8, n - 8);
  used += n - 8;
  bytes[used] = 0;
  return used == total ? 1 : 0;
}
int Button::update(bool down, uint64_t now) {
  if (down != sampled) {
    sampled = down;
    changed = now;
  }
  if (stable != sampled && now - changed >= 50) {
    stable = sampled;
    if (stable) {
      pressed = now;
      long_sent = false;
    } else if (!long_sent)
      return 1;
  }
  if (stable && !long_sent && now - pressed >= 3000) {
    long_sent = true;
    return 2;
  }
  return 0;
}
// La recuperación acepta v1 histórico y v2 validado, sin convertir formatos ni
// recrear identidades. Una lectura corrupta degrada el journal, no borra NVS.
static bool validate_journal_record(const char *data, size_t n, uint64_t *cursor) {
  if (!n || n > MaxMessage || !json_text_valid(data, n, true))
    return false;
  const char *end = nullptr;
  auto j = cJSON_ParseWithLengthOpts(data, n, &end, false);
  if (j) {
    while (end < data + n && strchr(" \t\r\n", *end))
      ++end;
    if (end != data + n) {
      cJSON_Delete(j);
      return false;
    }
  }
  if (!j)
    return false;
  auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
  const char *name = str(p, "name"), *profile = str(p, "profile");
  auto code = cJSON_GetObjectItemCaseSensitive(p, "code");
  bool ok = fields(j, "version,type,device_id,boot_id,message_id,seq,uptime_ms,"
                      "timestamp,time_quality,payload") &&
            number(j, "version") == 1 && str(j, "type") &&
            !strcmp(str(j, "type"), "event") && id_valid(str(j, "device_id")) &&
            id_valid(str(j, "boot_id")) && id_valid(str(j, "message_id")) &&
            fields(p, "journal_seq,name,profile,code") &&
            integer(cJSON_GetObjectItemCaseSensitive(p, "journal_seq"), 1) &&
            name &&
            (!strcmp(name, "system.booted") ||
             !strcmp(name, "system.profile_changed") ||
             !strcmp(name, "system.error")) &&
            profile &&
            (!strcmp(profile, "idle") || !strcmp(profile, "escudo") ||
             !strcmp(profile, "cebo")) &&
            (cJSON_IsNull(code) || cJSON_IsString(code));
  if (ok && cJSON_IsString(code)) {
    bool match = false;
    for (auto error : {"invalid_message", "unsupported_version", "unauthorized",
                       "wrong_boot", "expired", "revision_conflict", "busy",
                       "id_conflict", "not_implemented", "resource_error"})
      if (!strcmp(error, code->valuestring))
        match = true;
    ok = match;
  }
  ok = ok && integer(cJSON_GetObjectItemCaseSensitive(j, "seq")) &&
       integer(cJSON_GetObjectItemCaseSensitive(j, "uptime_ms"));
  const char *quality = str(j, "time_quality");
  auto timestamp = cJSON_GetObjectItemCaseSensitive(j, "timestamp");
  ok = ok && quality &&
       (!strcmp(quality, "unknown") ? cJSON_IsNull(timestamp)
                                    : ((!strcmp(quality, "app_sync") ||
                                        !strcmp(quality, "network_sync")) &&
                                       utc_valid(str(j, "timestamp"))));
  if (number(j, "version") == 2)
    ok = v2::clean(j) && v2::validate("event", j);
  if (ok)
    *cursor = number(p, "journal_seq");
  cJSON_Delete(j);
  return ok;
}
void Journal::recover() {
  highest = 0;
  memset(slots, 0, sizeof(slots));
  char out[MaxMessage + 1];
  for (size_t i = 0; i < capacity; ++i) {
    size_t n = sizeof(out);
    if (!store.read(i, out, &n)) {
      if (!store.missing())
        degraded = true;
      continue;
    }
    uint64_t seq = 0;
    if (!validate_journal_record(out, n, &seq) || (seq - 1) % capacity != i) {
      degraded = true;
      continue;
    }
    slots[i] = seq;
    if (seq > highest)
      highest = seq;
  }
}
bool Journal::append(const char *data, size_t n, uint64_t seq) {
  uint64_t actual = 0;
  if (seq != highest + 1 || seq > MaxInteger ||
      !validate_journal_record(data, n, &actual) ||
      actual != seq) {
    degraded = true;
    return false;
  }
  size_t slot = (seq - 1) % capacity;
  if (!store.write(slot, data, n)) {
    degraded = true;
    return false;
  }
  // Solo avanzamos el cursor visible tras confirmar la escritura del Store.
  slots[slot] = seq;
  highest = seq;
  return true;
}
bool Journal::read(uint64_t cursor, char *out, size_t *n) {
  if (!cursor || slots[(cursor - 1) % capacity] != cursor)
    return false;
  if (!store.read((cursor - 1) % capacity, out, n)) {
    degraded = true;
    return false;
  }
  uint64_t seq = 0;
  if (!validate_journal_record(out, *n, &seq) || seq != cursor) {
    degraded = true;
    return false;
  }
  return true;
}
uint64_t Journal::first() const {
  uint64_t v = 0;
  for (size_t i = 0; i < capacity; ++i)
    if (slots[i] && (!v || slots[i] < v))
      v = slots[i];
  return v;
}
size_t Journal::count() const {
  size_t n = 0;
  for (size_t i = 0; i < capacity; ++i)
    if (slots[i])
      ++n;
  return n;
}
Result Journal::page(uint64_t after, size_t limit, uint64_t *out,
                     size_t *n) const {
  Result r;
  r.page = true;
  r.from = first();
  r.to = highest;
  r.next = after;
  *n = 0;
  if (limit > 8)
    limit = 8;
  uint64_t previous = after;
  for (uint64_t seq = (r.from > after ? r.from : after + 1);
       seq && seq <= highest; ++seq) {
    if (slots[(seq - 1) % capacity] != seq) {
      r.gap = true;
      continue;
    }
    if (*n == limit) {
      r.more = true;
      break;
    }
    if (seq != previous + 1)
      r.gap = true;
    r.cursors[*n] = seq;
    out[(*n)++] = seq;
    r.count = *n;
    r.next = seq;
    previous = seq;
  }
  return r;
}
const char *profile_name(Profile p) {
  return p == Profile::Escudo        ? "escudo"
         : p == Profile::Cebo        ? "cebo"
         : p == Profile::Sanitizador ? "sanitizador"
                                     : "idle";
}
const char *requested_profile(const char *mode, Profile *out) {
  if (!mode)
    return "invalid_message";
  if (!strcmp(mode, "sanitizador"))
    return "not_implemented";
  if (!strcmp(mode, "idle"))
    *out = Profile::Idle;
  else if (!strcmp(mode, "escudo"))
    *out = Profile::Escudo;
  else if (!strcmp(mode, "cebo"))
    *out = Profile::Cebo;
  else
    return "invalid_message";
  return nullptr;
}
const char *Runtime::transition(Profile target, Resources &r) {
  if (!healthy && target != Profile::Idle)
    return "resource_error";
  phase = "transitioning";
  if (!r.stop_network() || !r.network_released()) {
    phase = "error";
    error = "resource_error";
    return error;
  }
  profile = Profile::Idle;
  if (target == Profile::Cebo && !r.start_cebo()) {
    r.stop_network();
    phase = "error";
    error = "resource_error";
    return error;
  }
  profile = target;
  ++revision;
  phase = target == Profile::Idle ? "idle" : "ready";
  error = nullptr;
  return nullptr;
}
} // namespace sentinel
