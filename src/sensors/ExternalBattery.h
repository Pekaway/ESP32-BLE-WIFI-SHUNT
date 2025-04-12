#ifndef EXTERNAL_BATTERY_H
#define EXTERNAL_BATTERY_H

#include <Arduino.h>
#include <constants.h>

class ExternalBattery {
 public:
  ExternalBattery(ExternalBattery const&) = delete;
  ExternalBattery& operator=(ExternalBattery const&) = delete;

  static ExternalBattery& getInstance() {
    static ExternalBattery instance;
    return instance;
  }

  static void init() { pinMode(EXTERNAL_BATTERY_PIN, INPUT); }

  static float readVoltage() {
    int const rawValue = analogRead(EXTERNAL_BATTERY_PIN);
    return (rawValue * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
  }

 private:
  ExternalBattery() = default;
  ~ExternalBattery() = default;

  static constexpr float REFERENCE_VOLTAGE = 5.0;
  static constexpr int ADC_RESOLUTION = 1023;
};

#endif  // EXTERNAL_BATTERY_H
