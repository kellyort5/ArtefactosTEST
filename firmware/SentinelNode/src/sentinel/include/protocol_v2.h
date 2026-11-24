#pragma once
#include "core.h"
namespace sentinel::v2 {
bool validate(const char *type, const cJSON *);
bool clean(const cJSON *, unsigned depth = 0);
cJSON *parse(const char *, size_t);
const char *admission(const Command &, Channel, Profile, bool authorized,
                      bool retained);
const char *new_context(const Command &, Profile, bool target_selected);
bool result_for(const Command &, const cJSON *);
} // namespace sentinel::v2
