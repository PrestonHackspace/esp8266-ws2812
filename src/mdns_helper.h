#pragma once

#include <functional>

#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#endif

struct MdnsService
{
    IPAddress ip;
    uint16_t port;
};

bool mdns_init(const char *deviceName);
bool mdns_discover(const char serviceName[], uint8_t attempts, MdnsService &service);
