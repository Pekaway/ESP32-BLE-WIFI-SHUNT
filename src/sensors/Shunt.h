#ifndef SHUNT_H
#define SHUNT_H

#include <utils/configKeys.h>
#include <Arduino.h>
#include <INA.h>
#include <Logger.h>

class Shunt {
 public:
  static Shunt& getInstance();

  bool init();
  void update();

  double getBusVoltage();
  double getBusCurrent();
  double getPower();
  [[nodiscard]] float getStateOfCharge() const;
  [[nodiscard]] float getFullChargeVoltage() const { return fullChargeVoltage; }
  [[nodiscard]] float getFullChargeCurrent() const { return fullChargeCurrent; }
  [[nodiscard]] uint32_t getFullChargeDuration() const { return fullChargeDuration / (60 * 1000); }
  [[nodiscard]] double getChargeEfficiency() const;
  [[nodiscard]] uint16_t calculateStateOfCharge() const;
  double getTTGO();

  void setMaxCapacity(uint16_t ampHours);
  void setCurrentStateOfCharge(uint8_t percentage);
  void setChargeEfficiency(uint8_t percentage);
  void setFullChargeVoltage(float voltage);
  void setFullChargeCurrent(float current);
  void setFullChargeDuration(uint32_t minutes);
  [[nodiscard]] uint64_t getMaxCapacity() const;

 private:
  Shunt();

  Logger logger = Logger(Serial);

  INA_Class ina;
  uint8_t deviceCount = 0;

  uint64_t maxCapacityMilliAmpMs = 0;
  uint64_t currentCapacityMilliAmpMs = 0;
  uint64_t lastStoredCapacityMilliAmpMs = 0;
  uint8_t chargeEfficiency = 0;

  float fullChargeVoltage = 0;
  float fullChargeCurrent = 0;
  uint32_t fullChargeDuration = 0;
  uint32_t fullChargeConditionStartTime = 0;
  bool fullChargeConditionMet = false;

  unsigned long lastUpdateMillis = 0;
  unsigned long lastStorageMillis = 0;

  bool loadConfig();
  void saveStateToConfig() const;
  void clampStateOfCharge();
};

#endif  // SHUNT_H