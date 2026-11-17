#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define PROGMEM
#define pgm_read_byte(p) (*(const unsigned char *)(p))
#define memcpy_P memcpy

#define pgm_read_word(p) (*(const uint16_t *)(p))
#define pgm_read_dword(p) (*(const uint32_t *)(p))
#define memcmp_P memcmp
