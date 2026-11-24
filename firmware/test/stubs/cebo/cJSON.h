#pragma once
// El módulo Cebo no llama a cJSON: module_api.hpp solo necesita el tipo opaco.
// Las pruebas de host de Cebo usan este stub para no depender de las fuentes
// de cJSON ni de OpenSSL.
struct cJSON;
