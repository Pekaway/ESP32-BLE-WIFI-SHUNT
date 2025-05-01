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
  if (!config.get<bool>(ConfigKey::LED_ENABLED)) {
    return;
  }

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
  if (!config.get<bool>(ConfigKey::LED_ENABLED)) {
    pixel.setBrightness(0);
    pixel.show();
    return;
  }

  if (setUpAllowed) {
    blue();
    return;
  }

  auto const currentMicroAmps = shunt.getBusCurrent();

  uint8_t const brightness = map(abs(currentMicroAmps / 1000000.0), 0, 500, 0, 510);

  if (currentMicroAmps < 0) {
    pixel.setPixelColor(0, Adafruit_NeoPixel::Color(0, brightness, 0));
  } else if (currentMicroAmps > 0) {
    pixel.setPixelColor(0, Adafruit_NeoPixel::Color(brightness, 0, 0));
  } else {
    pixel.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
  }

  pixel.show();
}
