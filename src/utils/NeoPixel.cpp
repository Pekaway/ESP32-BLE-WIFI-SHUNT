#include "NeoPixel.h"
#include "constants.h"
#include <Adafruit_NeoPixel.h>

NeoPixel::NeoPixel() {
  logger.prependLog = [] { return "NeoPixel"; };

  pixel = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
  pixel.begin();
}

NeoPixel& NeoPixel::getInstance() {
  static NeoPixel instance;
  return instance;
}

void NeoPixel::blue() {
  pixel.setBrightness(255);
  pixel.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 255));
  pixel.show();
}

void NeoPixel::red() {
  pixel.setBrightness(255);
  pixel.setPixelColor(0, Adafruit_NeoPixel::Color(255, 0, 0));
  pixel.show();
}

void NeoPixel::handle() {
  if (setUpAllowed) {
    blue();
    return;
  }

  if (!config.get<bool>(ConfigKey::LED_ENABLED)) {
    pixel.setBrightness(0);
    pixel.show();
    return;
  }

  auto const currentMicroAmps = shunt.getBusCurrent();

  uint8_t const brightness = map(abs(currentMicroAmps), 0, SHUNT_MAXIMUM_AMPS / 2, 0, 255);

  if (currentMicroAmps > 0) {
    pixel.setPixelColor(0, Adafruit_NeoPixel::Color(0, brightness, 0));
  } else if (currentMicroAmps < 0) {
    pixel.setPixelColor(0, Adafruit_NeoPixel::Color(brightness, 0, 0));
  } else {
    pixel.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
  }

  pixel.show();
}
