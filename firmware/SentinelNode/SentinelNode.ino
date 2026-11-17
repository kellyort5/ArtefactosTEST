#include <Arduino.h>
// Its constructor reserves BLE memory before initArduino(). Sentinel owns the
// NimBLE host; do not initialize BLEDevice or another BLE wrapper here.
// Select official ESP32S3 Dev Module / Sentinel preserved NVS; see README.
#include <esp32-hal-alloc-ble-mem.h>
#include "SentinelConfig.h"
#include "src/sentinel/include/sentinel.h"

void setup() { sentinel::start(); }
void loop() { delay(20); }
