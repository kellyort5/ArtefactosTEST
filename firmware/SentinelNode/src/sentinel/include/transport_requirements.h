#pragma once
#include <wolfMQTT.h>
// MQTT5 is source-built using the provider's Arduino layout, with TCP callbacks.
#if !defined(WOLFMQTT_V5) || defined(ENABLE_MQTT_TLS)
#error Sentinel requires MQTT5 over the external Arduino TCP transport
#endif
