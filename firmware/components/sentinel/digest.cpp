#include "core.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#ifdef ESP_PLATFORM
#include "mbedtls/sha256.h"
#else
// Public OpenSSL SHA256 ABI; the host runner locates the system runtime
// library.
extern "C" unsigned char *SHA256(const unsigned char *, size_t,
                                 unsigned char *);
#endif
namespace sentinel {
static bool sort(cJSON *o) {
  cJSON *items[16];
  size_t n = 0;
  while (o->child && n < 16) {
    auto j = o->child;
    cJSON_DetachItemViaPointer(o, j);
    items[n++] = j;
  }
  std::sort(items, items + n, [](const cJSON *a, const cJSON *b) {
    return strcmp(a->string, b->string) < 0;
  });
  bool ok = true;
  for (size_t i = 0; i < n; ++i) {
    char key[65];
    snprintf(key, sizeof(key), "%s", items[i]->string);
    if (!cJSON_AddItemToObject(o, key, items[i])) {
      cJSON_Delete(items[i]);
      ok = false;
    }
  }
  return ok;
}
static void wipe_strings(cJSON *j) {
  if (cJSON_IsString(j))
    wipe(j->valuestring, strlen(j->valuestring));
  for (auto child = j->child; child; child = child->next)
    wipe_strings(child);
}
bool digest_command(const Command &c, uint8_t out[32]) {
  auto p = cJSON_GetObjectItemCaseSensitive(c.root, "payload");
  auto canonical = cJSON_Duplicate(p, true);
  if (!canonical)
    return false;
  bool ok = sort(cJSON_GetObjectItemCaseSensitive(canonical, "args")) &&
            sort(canonical);
  char json[MaxMessage + 1] = {};
  if (ok)
    ok = cJSON_PrintPreallocated(canonical, json, sizeof(json), false);
  wipe_strings(canonical);
  cJSON_Delete(canonical);
  if (ok) {
#ifdef ESP_PLATFORM
    ok = mbedtls_sha256(reinterpret_cast<const unsigned char *>(json),
                        strlen(json), out, 0) == 0;
#else
    ok = SHA256(reinterpret_cast<const unsigned char *>(json), strlen(json),
                out) != nullptr;
#endif
  }
  wipe(json, sizeof(json));
  return ok;
}
} // namespace sentinel
