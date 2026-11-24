#pragma once
#include <chrono>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#define PROGMEM
#define pgm_read_byte(p) (*(const unsigned char *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#define pgm_read_dword(p) (*(const uint32_t *)(p))
#define memcpy_P memcpy
#define memcmp_P memcmp
uint32_t millis();
void delay(unsigned);
