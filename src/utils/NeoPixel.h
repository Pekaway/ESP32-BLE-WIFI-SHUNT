#ifndef NEOPIXEL_H
#define NEOPIXEL_H
#include "ConfigManager.h"
#include <sensors/Shunt.h>
#include <Adafruit_NeoPixel.h>
#include <Logger.h>

class NeoPixel {
 public:
  NeoPixel(NeoPixel const&) = delete;
  NeoPixel& operator=(NeoPixel const&) = delete;

  static NeoPixel& getInstance();

  void init();
  void blue();
  void red();
  void handle();
  void closeSetup() { setUpAllowed = false; }
  void enable();
  void disable();

 private:
  NeoPixel();
  ~NeoPixel() = default;

  Adafruit_NeoPixel pixel;
  Shunt& shunt = Shunt::getInstance();
  ConfigManager& config = ConfigManager::getInstance();
  bool setUpAllowed;

  Logger logger = Logger(Serial);
};

#endif  // NEOPIXEL_H
