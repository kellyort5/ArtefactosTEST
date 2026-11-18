#include "include/core.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#ifdef ESP_PLATFORM
#include "mbedtls/sha256.h"
#else
// ABI pública de OpenSSL: el runner host localiza la biblioteca del sistema.
extern "C" unsigned char *SHA256(const unsigned char *, size_t,
                                 unsigned char *);
#endif
namespace sentinel {
static bool sort_object_keys(cJSON *o) {
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
  // Canonizamos solo la copia del payload validado: el orden de claves no
  // cambia la identidad de la orden y el JSON original conserva su propiedad.
  auto p = cJSON_GetObjectItemCaseSensitive(c.root, "payload");
  auto canonical = cJSON_Duplicate(p, true);
  if (!canonical)
    return false;
  bool ok = sort_object_keys(cJSON_GetObjectItemCaseSensitive(canonical, "args")) &&
            sort_object_keys(canonical);
  // BLE invokes this from NimBLE's 5 KiB host stack. Keep the bounded
  // canonical buffer off that stack; allocator failure rejects the command.
  char *json = static_cast<char *>(cJSON_malloc(MaxMessage + 1));
  if (!json) {
    wipe_strings(canonical);
    cJSON_Delete(canonical);
    return false;
  }
  memset(json, 0, MaxMessage + 1);
  if (ok)
    ok = cJSON_PrintPreallocated(canonical, json, MaxMessage + 1, false);
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
  wipe(json, MaxMessage + 1);
  cJSON_free(json);
  return ok;
}
} // namespace sentinel
