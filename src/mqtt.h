#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>

#include "config.h"

typedef std::function<void(const char *topic, uint8_t *payload, unsigned int length)> MqttMessageCallback;

void mqtt_init(const char *deviceName, IPAddress ip, uint16_t port, MqttMessageCallback callback);
void mqtt_subscribe(const char *topic);
void mqtt_publish(const char *topic, const char *payload);
void mqtt_loop();
