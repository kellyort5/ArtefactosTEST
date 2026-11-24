#include "include/sentinel.h"
#include "../../SentinelConfig.h"
// Cada módulo opcional depende de sentinel_api y expone una única factory.
#if SENTINEL_MODULE_ESCUDO
extern "C" sentinel::Module *sentinel_escudo_module();
#endif
#if SENTINEL_MODULE_SANITIZADOR
extern "C" sentinel::Module *sentinel_sanitizador_module();
#endif
#if SENTINEL_MODULE_CEBO
extern "C" sentinel::Module *sentinel_cebo_module();
#endif
namespace sentinel {
void register_optional_modules() {
#if SENTINEL_MODULE_ESCUDO
  if (auto m = sentinel_escudo_module())
    register_module(Mode::Escudo, *m);
#endif
#if SENTINEL_MODULE_SANITIZADOR
  if (auto m = sentinel_sanitizador_module())
    register_module(Mode::Sanitizador, *m);
#endif
#if SENTINEL_MODULE_CEBO
  if (auto m = sentinel_cebo_module())
    register_module(Mode::Cebo, *m);
#endif
}
} // namespace sentinel
