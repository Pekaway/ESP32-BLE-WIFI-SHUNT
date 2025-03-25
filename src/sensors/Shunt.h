#ifndef SHUNT_H
#define SHUNT_H

#include <utils/configKeys.h>
#include <Arduino.h>
#include <INA.h>
#include <Logger.h>
#include <constants.h>

class Shunt {
 public:
  static Shunt& getInstance();

  bool init(uint16_t maximumAmps = 1022);
  void update();

  float getBusVoltage();
  float getBusCurrent();
  float getPower();
  float getStateOfCharge();
  [[nodiscard]] uint8_t getChargeEfficiency() const;

  void setMaxCapacity(uint32_t ampHours);
  void setCurrentStateOfCharge(uint8_t percentage);
  void setChargeEfficiency(uint8_t percentage);
  [[nodiscard]] uint32_t getMaxCapacity() const;

 private:
  Shunt();

  Logger logger = Logger(Serial);

  INA_Class ina;
  uint8_t deviceCount = 0;
  uint32_t shuntMicroOhm = SHUNT_MICRO_OHM;
  uint32_t maximumAmps = 1022;

  int64_t maxCapacityMilliAmpMs = 0;
  int64_t currentCapacityMilliAmpMs = 0;
  int64_t lastStoredCapacityMilliAmpMs = 0;
  int64_t capacityThresholdToStore = 360000000;
  uint8_t chargeEfficiency = 100;

  unsigned long lastUpdateMillis = 0;
  unsigned long lastStorageMillis = 0;
  unsigned long const STORAGE_INTERVAL_MS = 30000;

  bool loadConfig();
  bool saveStateToConfig();
  float calculateStateOfCharge();
  void clampStateOfCharge();
};

#endif  // SHUNT_H