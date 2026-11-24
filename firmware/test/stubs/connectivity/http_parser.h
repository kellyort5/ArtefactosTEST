#pragma once
#include <cstddef>
#include <cstdint>
enum { UF_HOST, UF_PORT };
struct http_parser_url {
  uint16_t field_set = 0, port = 0;
  struct { uint16_t off = 0, len = 0; } field_data[2];
};
int http_parser_parse_url(const char *, size_t, int, http_parser_url *);
