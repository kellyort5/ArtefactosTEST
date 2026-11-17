#include "include/protocol_v2.h"
#include <cctype>
#include <cmath>
#include <cstring>
#include <initializer_list>
#ifdef ESP_PLATFORM
#include "lwip/inet.h"
#include "lwip/sockets.h"
#else
#include <arpa/inet.h>
#endif
namespace sentinel::v2 {
static size_t characters(const char *s) {
  size_t n = 0;
  for (; *s; ++s)
    if ((uint8_t(*s) & 0xc0) != 0x80)
      ++n;
  return n;
}
static bool object_keys(const cJSON *j, const char *keys) {
  if (!cJSON_IsObject(j))
    return true;
  for (auto c = j->child; c; c = c->next) {
    bool match = false;
    for (auto p = keys; *p;) {
      auto end = strchr(p, ',');
      size_t n = end ? size_t(end - p) : strlen(p);
      if (strlen(c->string) == n && !memcmp(c->string, p, n))
        match = true;
      if (!end)
        break;
      p = end + 1;
    }
    if (!match)
      return false;
  }
  return true;
}
static bool array_items(const cJSON *j, bool (*test)(const cJSON *)) {
  if (cJSON_IsArray(j))
    for (auto c = j->child; c; c = c->next)
      if (!test(c))
        return false;
  return true;
}
static bool unique(const cJSON *j) {
  if (cJSON_IsArray(j))
    for (auto a = j->child; a; a = a->next)
      for (auto b = a->next; b; b = b->next)
        if (cJSON_Compare(a, b, true))
          return false;
  return true;
}
static bool url(const char *s, const char *scheme) {
  const char *p = strstr(s, "://");
  if (!p)
    return false;
  size_t n = p - s;
  if (!((n == strlen(scheme) && !memcmp(s, scheme, n)) ||
        (!strcmp(scheme, "http") && n == 5 && !memcmp(s, "https", 5))))
    return false;
  const char *host = p + 3, *end = host;
  if (*end == '[') {
    end = strchr(host, ']');
    if (!end)
      return false;
    char addr[46] = {};
    if (end - host - 1 >= 45)
      return false;
    memcpy(addr, host + 1, end - host - 1);
    unsigned char raw[16];
    if (inet_pton(AF_INET6, addr, raw) != 1)
      return false;
    ++end;
  } else {
    while (*end && *end != ':' && *end != '/') {
      if (!isalnum((unsigned char)*end) && *end != '.' && *end != '-')
        return false;
      ++end;
    }
    if (end == host || !isalnum((unsigned char)*host) ||
        !isalnum((unsigned char)end[-1]))
      return false;
  }
  if (*end == ':') {
    unsigned port = 0, count = 0;
    ++end;
    while (isdigit((unsigned char)*end)) {
      if (++count > 5)
        return false;
      port = port * 10 + *end++ - '0';
    }
    if (!count || !port || port > 65535)
      return false;
  }
  if (!strcmp(scheme, "mqtt"))
    return !*end;
  if (*end && *end != '/')
    return false;
  for (; *end; ++end)
    if (isspace((unsigned char)*end) || strchr("?#@", *end))
      return false;
  return true;
}
static bool pattern(const char *s, const char *tag) {
  if (!strcmp(tag, "text"))
    return true;
  if (!strcmp(tag, "utc"))
    return utc_valid(s);
  if (!strcmp(tag, "mqtt"))
    return mqtt_uri_valid(s);
  if (!strcmp(tag, "https"))
    return url(s, "https");
  if (!strcmp(tag, "http"))
    return url(s, "http");
  if (!strcmp(tag, "id") || !strcmp(tag, "cursor")) {
    for (auto p = s; *p; ++p)
      if (!isalnum((unsigned char)*p) &&
          !strchr(!strcmp(tag, "id") ? "._:-" : "._:~-", *p))
        return false;
    return *s;
  }
  if (!strcmp(tag, "mac")) {
    if (strlen(s) != 17)
      return false;
    for (size_t i = 0; i < 17; ++i)
      if (i % 3 == 2
              ? s[i] != ':'
              : !(isdigit((unsigned char)s[i]) || (s[i] >= 'a' && s[i] <= 'f')))
        return false;
    return true;
  }
  if (!strcmp(tag, "hex")) {
    if (strlen(s) % 2)
      return false;
    for (; *s; ++s)
      if (!isdigit((unsigned char)*s) && !(*s >= 'a' && *s <= 'f'))
        return false;
    return true;
  }
  if (!strcmp(tag, "domain")) {
    const char *label = s;
    for (auto p = s;; ++p)
      if (*p == '.' || !*p) {
        size_t n = p - label;
        if (!n || n > 63 || *label == '-' || p[-1] == '-')
          return false;
        if (!*p)
          return true;
        label = p + 1;
      } else if (!(*p >= 'a' && *p <= 'z') && !isdigit((unsigned char)*p) &&
                 *p != '-')
        return false;
  }
  return false;
}
static bool format(const char *s, const char *tag) {
  if (!strcmp(tag, "date-time"))
    return utc_valid(s);
  if (!strcmp(tag, "ipv4")) {
    unsigned char raw[4];
    return inet_pton(AF_INET, s, raw) == 1;
  }
  // El patrón del schema y el predicado URL acotan las URI aceptadas.
  return !strcmp(tag, "uri");
}
bool clean(const cJSON *j, unsigned depth) {
  // Claves repetidas harían ambigua una orden según quién lea el JSON.
  // Se rechazan antes de validar el schema, tanto en entrada como en salida.
  if (!j || depth > 32)
    return false;
  if (cJSON_IsObject(j) || cJSON_IsArray(j))
    for (auto c = j->child; c; c = c->next) {
      if (cJSON_IsObject(j)) {
        if (!c->string)
          return false;
        for (auto b = c->next; b; b = b->next)
          if (b->string && !strcmp(c->string, b->string))
            return false;
        if (cJSON_IsString(c) &&
            ((!strcmp(c->string, "ssid") && strlen(c->valuestring) > 32) ||
             (!strcmp(c->string, "wifi_password") &&
              strlen(c->valuestring) > 63)))
          return false;
      }
      if (!clean(c, depth + 1))
        return false;
    }
  return true;
}
cJSON *parse(const char *s, size_t n) {
  if (!json_text_valid(s, n, true))
    return nullptr;
  const char *end = nullptr;
  auto j = cJSON_ParseWithLengthOpts(s, n + 1, &end, true);
  if (!j || end != s + n || !clean(j)) {
    cJSON_Delete(j);
    return nullptr;
  }
  return j;
}
// Fuente local derivada de los contratos: Arduino la compila sin ejecutar scripts.
#include "v2_validator.h"
bool validate(const char *type, const cJSON *j) {
  if (!type || !clean(j) || !schema(type, j))
    return false;
  if (!strcmp(type, "ack")) {
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    auto r = cJSON_GetObjectItemCaseSensitive(p, "result");
    auto k = str(r, "kind");
    if (k && !strcmp(k, "job") &&
        strcmp(str(r, "job_id"), str(p, "command_id")))
      return false;
  }
  return true;
}
const char *admission(const Command &c, Channel channel, Profile profile,
                      bool auth, bool retained) {
  if (!auth)
    return "unauthorized";
  if (retained)
    return "invalid_message";
  if (channel == Channel::Mqtt && profile != Profile::Cebo)
    return "invalid_state";
  if (channel == Channel::Mqtt &&
      (!strcmp(c.operation, "configure_connectivity") ||
       !strcmp(c.operation, "set_time") ||
       !strncmp(c.operation, "escudo.", 7) ||
       !strncmp(c.operation, "sanitizador.", 12)))
    return "unauthorized";
  return nullptr;
}
const char *new_context(const Command &c, Profile profile, bool selected) {
  for (auto mode : {Profile::Escudo, Profile::Sanitizador, Profile::Cebo}) {
    const char *p = profile_name(mode);
    size_t n = strlen(p);
    if (!strncmp(c.operation, p, n) && c.operation[n] == '.' && profile != mode)
      return "invalid_state";
  }
  return !strcmp(c.operation, "escudo.capture_start") && !selected
             ? "invalid_state"
             : nullptr;
}
bool result_for(const Command &c, const cJSON *j) {
  const char *kind = str(j, "kind");
  const char *expected = nullptr;
  const char *op = c.operation;
  if (!strcmp(op, "escudo.scan") || !strcmp(op, "sanitizador.audit"))
    expected = "job";
  else if (!strcmp(op, "escudo.networks"))
    expected = "escudo.networks";
  else if (!strcmp(op, "escudo.select_target"))
    expected = "escudo.target";
  else if (!strcmp(op, "sanitizador.clients"))
    expected = "sanitizador.clients";
  else if (!strcmp(op, "sanitizador.rules.list"))
    expected = "sanitizador.rules";
  else if (!strcmp(op, "sanitizador.rules.put"))
    expected = "sanitizador.rule";
  else if (!strcmp(op, "sanitizador.rules.remove"))
    expected = "sanitizador.rule_removed";
  else if (!strcmp(op, "sanitizador.audit_result"))
    expected = "sanitizador.audit";
  else if (!strcmp(op, "cebo.stats"))
    expected = "cebo.stats";
  if (!expected)
    return !j || cJSON_IsNull(j);
  if (!kind || strcmp(kind, expected) || !validate("V2AckResult", j))
    return false;
  if (!strcmp(kind, "job"))
    return !strcmp(str(j, "job_id"), c.id);
  if (!strcmp(kind, "escudo.networks") ||
      !strcmp(kind, "sanitizador.clients") ||
      !strcmp(kind, "sanitizador.rules")) {
    const char *key = !strcmp(kind, "escudo.networks")       ? "networks"
                      : !strcmp(kind, "sanitizador.clients") ? "clients"
                                                             : "rules";
    auto next = str(j, "next"), after = str(c.args, "after");
    if (cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(j, key)) >
            int(number(c.args, "limit")) ||
        (next && after && !strcmp(next, after)))
      return false;
  }
  if (!strcmp(kind, "sanitizador.audit")) {
    auto audit = cJSON_GetObjectItemCaseSensitive(j, "audit");
    if (!cJSON_IsNull(audit) &&
        strcmp(str(audit, "job_id"), str(c.args, "job_id")))
      return false;
  }
  if (!strcmp(kind, "sanitizador.rule_removed") &&
      strcmp(str(j, "rule_id"), str(c.args, "rule_id")))
    return false;
  if (!strcmp(kind, "sanitizador.rule") &&
      !cJSON_Compare(cJSON_GetObjectItemCaseSensitive(j, "rule"), c.args, true))
    return false;
  if (!strcmp(kind, "escudo.target")) {
    auto target = cJSON_GetObjectItemCaseSensitive(j, "target");
    if (strcmp(str(target, "bssid"), str(c.args, "bssid")) ||
        number(target, "channel") != number(c.args, "channel"))
      return false;
    auto ssid = [](const cJSON *j, uint8_t out[32]) -> size_t {
      if (cJSON_IsString(j)) {
        size_t n = strlen(j->valuestring);
        memcpy(out, j->valuestring, n);
        return n;
      }
      auto s = str(j, "value");
      size_t n = strlen(s) / 2;
      auto hex = [](char c) { return c >= 'a' ? c - 'a' + 10 : c - '0'; };
      for (size_t i = 0; i < n; ++i)
        out[i] = hex(s[2 * i]) * 16 + hex(s[2 * i + 1]);
      return n;
    };
    uint8_t left[32], right[32];
    size_t a = ssid(cJSON_GetObjectItemCaseSensitive(target, "ssid"), left),
           b = ssid(cJSON_GetObjectItemCaseSensitive(c.args, "ssid"), right);
    if (a != b || memcmp(left, right, a))
      return false;
  }
  return true;
}
} // namespace sentinel::v2
