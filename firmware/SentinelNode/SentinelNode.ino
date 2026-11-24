#include <Arduino.h>
// Its constructor reserves BLE memory before initArduino(). Sentinel owns the
// NimBLE host; do not initialize BLEDevice or another BLE wrapper here.
// Select official ESP32S3 Dev Module / Sentinel standard USB; see README.
#include <esp32-hal-alloc-ble-mem.h>
#include "SentinelConfig.h"
#include "src/sentinel/include/sentinel.h"

void setup() {
#if SENTINEL_SERIAL_TEST_DISPLAY
  Serial.begin(115200);
#endif
  sentinel::start();
}
void loop() { delay(20); }
