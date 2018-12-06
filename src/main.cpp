#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WS2812FX.h>
#else
#include <WiFi.h>
#include <WiFiClient.h>
#endif

#include "config.h"
#include "mdns_helper.h"
#include "mqtt.h"

extern const char index_html[];
extern const char main_js[];

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define WIFI_TIMEOUT 30000 // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80

#define DEFAULT_COLOR 0x0F0F0F
#define DEFAULT_BRIGHTNESS 127
#define DEFAULT_SPEED 1000
#define DEFAULT_MODE FX_MODE_STATIC

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
String modes = "";
uint8_t myModes[] = {}; // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(HTTP_PORT);

/*
 * Connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
 */
void wifi_setup()
{
  WiFiManager wifiManager;

  wifiManager.autoConnect("AutoConnectAP");

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

/*
 * Build <li> string for all modes.
 */
void modes_setup()
{
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for (uint8_t i = 0; i < num_modes; i++)
  {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#' class='m' id='";
    modes += m;
    modes += "'>";
    modes += ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found()
{
  server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html()
{
  server.send_P(200, "text/html", index_html);
}

void srv_handle_main_js()
{
  server.send_P(200, "application/javascript", main_js);
}

void srv_handle_modes()
{
  server.send(200, "text/plain", modes);
}

void apply_raw(uint8_t *data, unsigned int length)
{
  if (ws2812fx.getMode() != FX_MODE_CUSTOM)
  {
    ws2812fx.setMode(FX_MODE_CUSTOM);
  }

  for (auto idx = 0; idx < length; idx += 1, data += 3)
  {
    auto r = *(data + 0);
    auto g = *(data + 1);
    auto b = *(data + 2);

    auto col = (r * 65536) + (g * 256) + b;

    ws2812fx.setPixelColor(idx, col);
  }
}

void apply_effect(String name, String value)
{
  if (name == "p")
  {
    auto sep = value.indexOf(":");

    auto idx = value.substring(0, sep).toInt();
    auto col = value.substring(sep + 1);

    uint32_t tmp = (uint32_t)strtol(&col[0], NULL, 16);

    if (ws2812fx.getMode() != FX_MODE_CUSTOM)
    {
      ws2812fx.setMode(FX_MODE_CUSTOM);
    }

    ws2812fx.setPixelColor(idx, tmp);
  }

  if (name == "c")
  {
    uint32_t tmp = (uint32_t)strtol(&value[0], NULL, 16);
    if (tmp >= 0x000000 && tmp <= 0xFFFFFF)
    {
      ws2812fx.setColor(tmp);
    }
  }

  if (name == "m")
  {
    uint8_t tmp = (uint8_t)strtol(&value[0], NULL, 10);
    ws2812fx.setMode(tmp % ws2812fx.getModeCount());
    Serial.print("mode is ");
    Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
  }

  if (name == "b")
  {
    if (value[0] == '-')
    {
      ws2812fx.setBrightness(ws2812fx.getBrightness() * 0.8);
    }
    else if (value[0] == ' ')
    {
      ws2812fx.setBrightness(min(max(ws2812fx.getBrightness(), 5) * 1.2, 255));
    }
    else
    { // set brightness directly
      uint8_t tmp = (uint8_t)strtol(&value[0], NULL, 10);
      ws2812fx.setBrightness(tmp);
    }
    Serial.print("brightness is ");
    Serial.println(ws2812fx.getBrightness());
  }

  if (name == "s")
  {
    if (value[0] == '-')
    {
      ws2812fx.setSpeed(max(ws2812fx.getSpeed(), 5) * 1.2);
    }
    else
    {
      ws2812fx.setSpeed(ws2812fx.getSpeed() * 0.8);
    }
    Serial.print("speed is ");
    Serial.println(ws2812fx.getSpeed());
  }

  if (name == "a")
  {
    if (value[0] == '-')
    {
      auto_cycle = false;
    }
    else
    {
      auto_cycle = true;
      auto_last_change = 0;
    }
  }
}

void srv_handle_set()
{
  for (uint8_t i = 0; i < server.args(); i++)
  {
    apply_effect(server.argName(i), server.arg(i));
  }

  server.send(200, "text/plain", "OK");
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Starting...");

  modes.reserve(5000);
  modes_setup();

  ws2812fx.setCustomMode([]() { return (uint16_t)0; });

  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setMode(DEFAULT_MODE);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();

  Serial.println("Wifi setup");
  wifi_setup();

  Serial.println("HTTP server setup");
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/set", srv_handle_set);
  server.onNotFound(srv_handle_not_found);
  server.begin();
  Serial.println("HTTP server started.");

  Serial.println("ready!");

  if (mdns_init(DEVICE_TYPE_NAME))
  {
    MdnsService mqttService;

    if (mdns_discover("mqtt", 5, mqttService))
    {
      mqtt_init(DEVICE_TYPE_NAME, mqttService.ip, mqttService.port, [](const char *topic, uint8_t *payload, unsigned int length) {
        if (strcmp(topic, LED_CHANNEL) == 0)
        {
          auto message = new char[length + 1];

          strcpy(message, (char *)payload);
          message[length] = 0;

          auto msg = String(message);

          // Example message: "m=12"
          auto name = msg.substring(0, 1);
          auto value = msg.substring(2);

          delete message;

          apply_effect(name, value);
        }

        if (strcmp(topic, LED_RAW_CHANNEL) == 0)
        {
          // Serial.println("Raw:" + String(length));

          apply_raw(payload, length / 3);
        }
      });

      mqtt_subscribe(LED_CHANNEL);
      mqtt_subscribe(LED_RAW_CHANNEL);
    }
  }
}

void loop()
{
  unsigned long now = millis();

  server.handleClient();
  ws2812fx.service();

  mqtt_loop();

  if (now - last_wifi_check_time > WIFI_TIMEOUT)
  {
    Serial.print("Checking WiFi... ");

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi connection lost. Restarting...");

      ESP.reset();
    }
    else
    {
      Serial.println("OK");
    }

    last_wifi_check_time = now;
  }

  // if (auto_cycle && (now - auto_last_change > 10000))
  // { // cycle effect mode every 10 seconds
  //   uint8_t next_mode = (ws2812fx.getMode() + 1) % ws2812fx.getModeCount();

  //   if (sizeof(myModes) > 0)
  //   { // if custom list of modes exists
  //     for (uint8_t i = 0; i < sizeof(myModes); i++)
  //     {
  //       if (myModes[i] == ws2812fx.getMode())
  //       {
  //         next_mode = ((i + 1) < sizeof(myModes)) ? myModes[i + 1] : myModes[0];
  //         break;
  //       }
  //     }
  //   }

  //   ws2812fx.setMode(next_mode);
  //   Serial.print("mode is ");
  //   Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
  //   auto_last_change = now;
  // }
}
