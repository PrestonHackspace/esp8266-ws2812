#include "mqtt.h"

WiFiClient *_espClient = NULL;
PubSubClient *_mqttClient = NULL;

void mqtt_init(const char *deviceName, IPAddress ip, uint16_t port, MqttMessageCallback callback)
{
  _espClient = new WiFiClient();
  _mqttClient = new PubSubClient(*_espClient);

  Serial.println("Connect to MQTT: " + ip.toString());

  _mqttClient->setServer(ip, port);

  _mqttClient->setCallback([callback](const char *topic, uint8_t *payload, unsigned int length) {
    // Serial.println(String("Message arrived [") + String(topic) + String("]"));

    callback(topic, payload, length);
  });

  if (_mqttClient->connect(deviceName))
  {
    Serial.println("Connected");
  }
  else
  {
    Serial.println("Failed to connect to MQTT broker!");

    // delete _espClient;
    // _espClient = NULL;

    // delete _mqttClient;
    _mqttClient = NULL;
  }
}

void mqtt_subscribe(const char *topic)
{
  if (_mqttClient)
  {
    _mqttClient->subscribe(topic);
  }
}

void mqtt_publish(const char *topic, const char *payload)
{
  if (_mqttClient)
  {
    bool success = _mqttClient->publish(topic, payload);

    if (!success)
    {
      Serial.println("Failed to publish MQTT message!");
    }
  }
}

void mqtt_loop()
{
  if (_mqttClient)
  {
    if (!_mqttClient->connected())
    {
      Serial.println("MQTT no longer connected!");

      ESP.reset();

      // while (!_mqttClient->connect("RECONNECT"))
      // {
      //   Serial.println("Reconnecting...");
      //   delay(1000);
      // }

      // delay(1000);

      // Serial.println("Resubscribing...");
      // _mqttClient->subscribe(LED_CHANNEL);
      // Serial.println("Resubscribed!");
    }

    _mqttClient->loop();
  }
}
