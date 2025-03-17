#ifndef SHUNT_H
#define SHUNT_H

#include <utils/configKeys.h>
#include <Arduino.h>
#include <INA.h>
#include <Logger.h>
#include <SPIFFS.h>
#include <constants.h>

class Shunt {
 public:
  static Shunt& getInstance();

  bool init(uint32_t shuntMicroOhm = 375, uint16_t maximumAmps = 1022);

  // Read current values from the INA sensor
  float getBusVoltage();
  float getBusCurrent();
  float getPower();
  float getStateOfCharge();

  // Battery capacity management
  void setMaxCapacity(uint32_t ampHours);
  void setCurrentStateOfCharge(uint8_t percentage);
  uint32_t getMaxCapacity();

  // Processing
  void update();

 private:
  Shunt();
  ~Shunt() = default;
  Shunt(Shunt const&) = delete;
  Shunt& operator=(Shunt const&) = delete;

  Logger logger = Logger(Serial);

  // INA sensor
  INA_Class ina;
  uint8_t deviceCount = 0;
  uint32_t shuntMicroOhm = SHUNT_MICRO_OHM;
  uint32_t maximumAmps = 1022;

  // Battery parameters
  int64_t maxCapacityMilliAmpMs = 0;
  int64_t currentCapacityMilliAmpMs = 0;
  int64_t lastStoredCapacityMilliAmpMs = 0;
  int64_t capacityThresholdToStore = 360000000;

  unsigned long lastUpdateMillis = 0;
  unsigned long lastStorageMillis = 0;
  unsigned long const STORAGE_INTERVAL_MS = 30000;  // 30 seconds

  bool loadConfig();
  bool saveStateToConfig();

  float calculateStateOfCharge();
  void clampStateOfCharge();
};

#endif  // SHUNT_H