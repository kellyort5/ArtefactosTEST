#pragma once
#include <ESP_SSLClient.h>
#include <wolfMQTT.h>
// These are the actual source-built library settings, not SDK flag overrides.
#if !defined(WOLFMQTT_V5) || defined(ENABLE_MQTT_TLS)
#error Sentinel requires MQTT5 with the mandatory external verified TLS transport
#endif
#if ESP_SSLCLIENT_VERSION_MAJOR != 3 || ESP_SSLCLIENT_VERSION_MINOR != 1 ||    \
    ESP_SSLCLIENT_VERSION_PATCH != 3 || defined(SSLCLIENT_INSECURE_ONLY) ||    \
    defined(BEARSSL_SSL_BASIC)
#error Sentinel requires the verified complete ESP_SSLClient 3.1.3 X509 engine
#endif
