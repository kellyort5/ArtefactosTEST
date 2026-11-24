#pragma once
// Subconjunto mínimo de cJSON para las pruebas de host de Cebo: la misma
// disposición de la estructura y la misma semántica de las pocas funciones que
// usa el módulo (lectura de los argumentos de cebo.configure). En el
// dispositivo se usa el cJSON real del núcleo; esto evita depender de sus
// fuentes y de OpenSSL en las pruebas de host.
#include <cstring>

constexpr int cJSON_False = 1 << 0, cJSON_True = 1 << 1, cJSON_NULL = 1 << 2,
              cJSON_Number = 1 << 3, cJSON_String = 1 << 4,
              cJSON_Array = 1 << 5, cJSON_Object = 1 << 6;
using cJSON_bool = int;

struct cJSON {
  cJSON *next = nullptr, *prev = nullptr, *child = nullptr;
  int type = 0;
  char *valuestring = nullptr;
  int valueint = 0;
  double valuedouble = 0;
  char *string = nullptr; // nombre de la clave dentro de un objeto
};

inline cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object,
                                               const char *name) {
  if (!object || !name)
    return nullptr;
  for (cJSON *c = object->child; c; c = c->next)
    if (c->string && !strcmp(c->string, name))
      return c;
  return nullptr;
}
inline cJSON_bool cJSON_IsNumber(const cJSON *item) {
  return item && (item->type & 0xFF) == cJSON_Number;
}
inline cJSON_bool cJSON_IsTrue(const cJSON *item) {
  return item && (item->type & 0xFF) == cJSON_True;
}
inline cJSON_bool cJSON_IsObject(const cJSON *item) {
  return item && (item->type & 0xFF) == cJSON_Object;
}
