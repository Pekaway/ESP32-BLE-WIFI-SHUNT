#ifndef EXTERNAL_BATTERY_H
#define EXTERNAL_BATTERY_H

#include <driver/adc.h>
#include <Arduino.h>
#include <Logger.h>

class ExternalBattery {
 public:
  ExternalBattery(ExternalBattery const&) = delete;
  ExternalBattery& operator=(ExternalBattery const&) = delete;

  static ExternalBattery& getInstance() {
    static ExternalBattery instance;
    return instance;
  }

  void init() {
    adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_12);
    logger.info("ADC2 channel configured for external battery voltage measurement");
  }

  float readVoltage() {
    int rawValue = 0;
    if (esp_err_t const result = adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &rawValue); result != ESP_OK) {
      logger.critical("ADC read failed!");
      return 0.0;
    }

    // see also
    // https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c3/api-reference/peripherals/adc_continuous.html
    return (rawValue * Vmax) / Dmax;
  }

 private:
  ExternalBattery() {
    logger.prependLog = [] { return "EXTERNAL_BATTERY"; };
  }
  ~ExternalBattery() = default;

  static constexpr float Vmax = 5.0;
  // 2^12 = 4096
  static constexpr float Dmax = 4096.0;

  Logger logger = Logger(Serial);
};

#endif  // EXTERNAL_BATTERY_H
