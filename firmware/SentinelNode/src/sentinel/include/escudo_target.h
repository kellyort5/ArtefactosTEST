#pragma once
#include "../../sentinel_api/include/module_api.hpp"

namespace sentinel {
enum class EscudoTargetLoad { Missing, Loaded, Error };
// Supervisor only. The core owns NVS; Escudo never opens a store or journal.
// Persist identity/channel and selection-time security context, never RSSI,
// uptime, catalog, capture state or detector evidence. Loaded RSSI is unset (0).
EscudoTargetLoad load_escudo_target(ScanNetwork &);
bool save_escudo_target(const ScanNetwork &);
} // namespace sentinel
