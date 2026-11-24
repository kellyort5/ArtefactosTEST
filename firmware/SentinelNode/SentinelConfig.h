#pragma once
// Application settings only. The official board menu selects the verified SDK;
// transport and BLE adapters enforce their security policies at runtime.
#ifndef SENTINEL_VERSION
#define SENTINEL_VERSION "0.1.0"
#endif
#ifndef SENTINEL_SDA
#define SENTINEL_SDA 8
#endif
#ifndef SENTINEL_SCL
#define SENTINEL_SCL 9
#endif
#ifndef SENTINEL_OLED_ADDRESS
#define SENTINEL_OLED_ADDRESS 0x3c
#endif
#ifndef SENTINEL_BUTTON
#define SENTINEL_BUTTON 4
#endif

#ifndef SENTINEL_MODULE_ESCUDO
#define SENTINEL_MODULE_ESCUDO 1
#endif
#ifndef SENTINEL_MODULE_SANITIZADOR
#define SENTINEL_MODULE_SANITIZADOR 0
#endif
#ifndef SENTINEL_MODULE_CEBO
#define SENTINEL_MODULE_CEBO 0
#endif
#if (SENTINEL_MODULE_ESCUDO != 0 && SENTINEL_MODULE_ESCUDO != 1) || \
    (SENTINEL_MODULE_SANITIZADOR != 0 && SENTINEL_MODULE_SANITIZADOR != 1) || \
    (SENTINEL_MODULE_CEBO != 0 && SENTINEL_MODULE_CEBO != 1)
#error Sentinel module flags must be 0 or 1
#endif

// Temporary local display for P1 tests without OLED. Never enable in public logs.
#ifndef SENTINEL_SERIAL_TEST_DISPLAY
#define SENTINEL_SERIAL_TEST_DISPLAY 0
#endif
#if SENTINEL_SERIAL_TEST_DISPLAY != 0 && SENTINEL_SERIAL_TEST_DISPLAY != 1
#error Sentinel serial test display flag must be 0 or 1
#endif
#if SENTINEL_SERIAL_TEST_DISPLAY && ARDUINO_USB_CDC_ON_BOOT
#error Use USB CDC On Boot Disabled and the USB-UART connector for the P1 display
#endif
