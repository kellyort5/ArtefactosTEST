#include "sentinel.h"
// Each optional component depends only on sentinel_api and exports one factory.
#ifdef CONFIG_SENTINEL_MODULE_ESCUDO
extern "C" sentinel::Module *sentinel_escudo_module();
#endif
#ifdef CONFIG_SENTINEL_MODULE_SANITIZADOR
extern "C" sentinel::Module *sentinel_sanitizador_module();
#endif
#ifdef CONFIG_SENTINEL_MODULE_CEBO
extern "C" sentinel::Module *sentinel_cebo_module();
#endif
namespace sentinel {
void register_optional_modules() {
#ifdef CONFIG_SENTINEL_MODULE_ESCUDO
  if (auto m = sentinel_escudo_module())
    register_module(Mode::Escudo, *m);
#endif
#ifdef CONFIG_SENTINEL_MODULE_SANITIZADOR
  if (auto m = sentinel_sanitizador_module())
    register_module(Mode::Sanitizador, *m);
#endif
#ifdef CONFIG_SENTINEL_MODULE_CEBO
  if (auto m = sentinel_cebo_module())
    register_module(Mode::Cebo, *m);
#endif
}
} // namespace sentinel
