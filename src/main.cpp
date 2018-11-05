#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <NeoPixelBus.h>

const uint16_t PixelCount = 4;
const uint8_t PixelPin = 2;

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>* strip;

RgbColor white(255);
RgbColor black(0);

void setup() {
  strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>(PixelCount, PixelPin);

  strip->Begin();
  strip->ClearTo(black);
  strip->Show();
}

void loop() {
  // put your main code here, to run repeatedly:

  strip->SetPixelColor(0, white);
  strip->Show();

  delay(1000);
  
  strip->SetPixelColor(0, black);
  strip->Show();

  delay(1000);
}
