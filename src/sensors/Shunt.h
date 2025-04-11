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

  bool init();
  void update();

  float getBusVoltage();
  float getBusCurrent();
  float getPower();
  [[nodiscard]] float getStateOfCharge() const;
  [[nodiscard]] float getFullChargeVoltage() const { return fullChargeVoltage; }
  [[nodiscard]] float getFullChargeCurrent() const { return fullChargeCurrent; }
  [[nodiscard]] uint32_t getFullChargeDuration() const { return fullChargeDuration / (60 * 1000); }
  [[nodiscard]] uint8_t getChargeEfficiency() const;
  [[nodiscard]] float calculateStateOfCharge() const;

  void setMaxCapacity(uint32_t ampHours);
  void setCurrentStateOfCharge(uint8_t percentage);
  void setChargeEfficiency(uint8_t percentage);
  void setFullChargeVoltage(float voltage);
  void setFullChargeCurrent(float current);
  void setFullChargeDuration(uint32_t minutes);
  [[nodiscard]] uint32_t getMaxCapacity() const;

 private:
  Shunt();

  Logger logger = Logger(Serial);

  INA_Class ina;
  uint8_t deviceCount = 0;
  uint32_t shuntMicroOhm = SHUNT_MICRO_OHM;
  uint32_t maximumAmps = SHUNT_MAXIMUM_AMPS;

  int64_t maxCapacityMilliAmpMs = 0;
  int64_t currentCapacityMilliAmpMs = 0;
  int64_t lastStoredCapacityMilliAmpMs = 0;
  int64_t capacityThresholdToStore = 360000000;
  uint8_t chargeEfficiency = 100;

  float fullChargeVoltage = 14.2;
  float fullChargeCurrent = 4.0;
  uint32_t fullChargeDuration = 180000;
  uint32_t fullChargeConditionStartTime = 0;
  bool fullChargeConditionMet = false;

  unsigned long lastUpdateMillis = 0;
  unsigned long lastStorageMillis = 0;
  unsigned long const STORAGE_INTERVAL_MS = 30000;

  bool loadConfig();
  bool saveStateToConfig();
  void clampStateOfCharge();
};

#endif  // SHUNT_H