#include <Arduino.h>
#include <NeoPixelBus.h>

const uint16_t PixelCount = 4;
const uint8_t PixelPin = 2;

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount, PixelPin);

RgbColor white(255);
RgbColor black(0);

void setup() {
  // put your setup code here, to run once:
  strip.Begin();
  strip.ClearTo(black);
  strip.Show();
}

void loop() {
  // put your main code here, to run repeatedly:

  strip.SetPixelColor(0, white);
  strip.Show();

  delay(1000);
  
  strip.SetPixelColor(0, black);
  strip.Show();

  delay(1000);
}
