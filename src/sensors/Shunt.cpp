#include "Shunt.h"
#include "../utils/ConfigManager.h"

Shunt& Shunt::getInstance() {
  static Shunt instance;
  return instance;
}

Shunt::Shunt() {
  logger.prependLog = [] { return "SHUNT"; };
}

bool Shunt::init(uint16_t const maximumAmps) {
  ConfigManager& config = ConfigManager::getInstance();

  this->maximumAmps = config.get<uint32_t>(ConfigKey::MAXIMUM_AMPS, maximumAmps);
  this->chargeEfficiency = config.get<uint8_t>(ConfigKey::CHARGE_EFFICIENCY);

  deviceCount = ina.begin(this->maximumAmps, this->shuntMicroOhm, 255, 7, 6);
  while (deviceCount == 0) {
    logger.critical("No INA device found, retrying in 10 seconds...");
    delay(10000);
    deviceCount = ina.begin(this->maximumAmps, this->shuntMicroOhm);
  }

  char message[50];
  snprintf(message, sizeof(message), "Detected %d INA devices on the I2C bus", deviceCount);
  logger.info(message);

  ina.setBusConversion(8500);
  ina.setShuntConversion(8500);
  ina.setAveraging(128);
  ina.setMode(INA_MODE_CONTINUOUS_BOTH);
  ina.alertOnBusOverVoltage(true, 16000);

  loadConfig();

  return true;
}

float Shunt::getBusVoltage() { return ina.getBusMilliVolts(0) / 1000.0; }

float Shunt::getBusCurrent() {
  // negative is charging, positive is discharging
  return ina.getBusMicroAmps(0) / 1000000.0 * -1;
}

float Shunt::getPower() { return ina.getBusMicroWatts(0) / 1000000.0; }

float Shunt::getStateOfCharge() const { return calculateStateOfCharge(); }

void Shunt::setMaxCapacity(uint32_t ampHours) {
  maxCapacityMilliAmpMs = (ampHours) * 60LL * 60LL * 1000LL * 1000LL;

  ConfigManager& config = ConfigManager::getInstance();
  config.set<uint32_t>(ConfigKey::MAXIMUM_AMPS, ampHours);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Max capacity set to %d Ah", ampHours);
  logger.info(message);
}

void Shunt::setCurrentStateOfCharge(uint8_t percentage) {
  if (percentage > 100) percentage = 100;
  currentCapacityMilliAmpMs = (maxCapacityMilliAmpMs / 100) * percentage;

  saveStateToConfig();
  lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;

  char message[50];
  snprintf(message, sizeof(message), "SOC set to %d%%", percentage);
  logger.info(message);
}

void Shunt::setChargeEfficiency(uint8_t percentage) {
  if (percentage > 100) percentage = 100;

  ConfigManager& config = ConfigManager::getInstance();
  config.set<uint8_t>(ConfigKey::CHARGE_EFFICIENCY, percentage);
  config.saveConfig();

  this->chargeEfficiency = percentage;

  char message[50];
  snprintf(message, sizeof(message), "Charge efficiency set to %d%%", percentage);
  logger.info(message);
}

uint8_t Shunt::getChargeEfficiency() const { return this->chargeEfficiency; }

uint32_t Shunt::getMaxCapacity() const { return maxCapacityMilliAmpMs / (60LL * 60LL * 1000LL * 1000LL); }

bool Shunt::loadConfig() {
  ConfigManager& config = ConfigManager::getInstance();

  uint32_t maxCapacity = config.get<int>(ConfigKey::MAXIMUM_AMPS, 100);  // Default 100Ah
  maxCapacityMilliAmpMs = static_cast<int64_t>(maxCapacity) * 60LL * 60LL * 1000LL * 1000LL;

  int socPercentage = config.get<int>(ConfigKey::CURRENT_SOC, config.get<int>(ConfigKey::INITIAL_SOC, 80));
  currentCapacityMilliAmpMs = (maxCapacityMilliAmpMs / 100) * socPercentage;

  if (config.hasKey(ConfigKey::CURRENT_CAPACITY_MAMS)) {
    auto const capacityStr = config.get<char const*>(ConfigKey::CURRENT_CAPACITY_MAMS, "0");
    currentCapacityMilliAmpMs = strtoll(capacityStr, nullptr, 10);
  }

  fullChargeVoltage = config.get<float>(ConfigKey::FULL_CHARGE_VOLTAGE, 14.2);
  fullChargeCurrent = config.get<float>(ConfigKey::FULL_CHARGE_CURRENT, 4.0);
  fullChargeDuration = config.get<uint32_t>(ConfigKey::FULL_CHARGE_DURATION, 3) * 60 * 1000;

  lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;

  char message[100];
  snprintf(message, sizeof(message), "Loaded max capacity: %d Ah, SOC: %d%%", maxCapacity, socPercentage);
  logger.info(message);

  return true;
}

bool Shunt::saveStateToConfig() {
  ConfigManager& config = ConfigManager::getInstance();

  int const socPercentage = static_cast<int>(calculateStateOfCharge());
  config.set(ConfigKey::CURRENT_SOC, socPercentage);

  // store current capacity in milliamp-milliseconds to preserve precision
  char capacityStr[32];
  snprintf(capacityStr, sizeof(capacityStr), "%lld", currentCapacityMilliAmpMs);
  config.set(ConfigKey::CURRENT_CAPACITY_MAMS, capacityStr);

  bool const result = config.saveConfig();

  if (result) {
    char message[100];
    snprintf(message, sizeof(message), "Stored SOC: %d%%, capacity: %lld mA-ms", socPercentage,
             currentCapacityMilliAmpMs);

    logger.info(message);
  } else {
    logger.critical("Failed to store battery state");
  }

  return result;
}

void Shunt::update() {
  auto const currentMillis = millis();

  if (lastUpdateMillis > 0) {
    // Calculate consumed or recharged capacity since last update
    auto const elapsedMs = currentMillis - lastUpdateMillis;
    auto const currentAmps = getBusCurrent() * chargeEfficiency / 100;
    auto const capacityDeltaMilliAmpMs = (currentAmps * 1000 * elapsedMs);

    // Update the current capacity
    currentCapacityMilliAmpMs += capacityDeltaMilliAmpMs;
    clampStateOfCharge();
  }

  float const voltage = getBusVoltage();
  float const current = getBusCurrent();
  float const soc = calculateStateOfCharge();

  // Both conditions must be met:
  // 1. Voltage must be at or above threshold
  // 2. Current must be below threshold but greater than zero
  bool const chargingConditionsMet = (voltage >= fullChargeVoltage) && (current < fullChargeCurrent) &&
                                     (current > 0.0) && (soc < 99.0);  // Only trigger if not already at 100%

  if (chargingConditionsMet) {
    if (!fullChargeConditionMet) {
      fullChargeConditionMet = true;
      fullChargeConditionStartTime = currentMillis;
      logger.info("Full charge conditions detected, starting timer");
    } else if (currentMillis - fullChargeConditionStartTime >= fullChargeDuration) {
      logger.info("Battery full charge criteria met for required duration");
      setCurrentStateOfCharge(100);
      fullChargeConditionMet = false;
    }
  } else if (fullChargeConditionMet) {
    fullChargeConditionMet = false;
    logger.info("Full charge conditions no longer met, resetting timer");
  }

  ConfigManager& config = ConfigManager::getInstance();

  if (auto const autoSaveInterval = config.get<uint32_t>(ConfigKey::AUTO_SAVE_INTERVAL, 30) * 1000;
      currentMillis - lastStorageMillis >= autoSaveInterval) {
    lastStorageMillis = currentMillis;

    if (fullChargeConditionMet) {
      currentCapacityMilliAmpMs = maxCapacityMilliAmpMs;
    }

    int64_t capacityChange = lastStoredCapacityMilliAmpMs - currentCapacityMilliAmpMs;
    if (capacityChange < 0) capacityChange = -capacityChange;

    if (capacityChange > capacityThresholdToStore) {
      saveStateToConfig();
      lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;
    }
  }

  lastUpdateMillis = currentMillis;
}

float Shunt::calculateStateOfCharge() const {
  if (maxCapacityMilliAmpMs <= 0) return 0;

  return (static_cast<float>(currentCapacityMilliAmpMs) / static_cast<float>(maxCapacityMilliAmpMs)) * 100.0f;
}

void Shunt::clampStateOfCharge() {
  if (currentCapacityMilliAmpMs > maxCapacityMilliAmpMs) {
    currentCapacityMilliAmpMs = maxCapacityMilliAmpMs;
  }

  if (currentCapacityMilliAmpMs < 0) {
    currentCapacityMilliAmpMs = 0;
  }
}

void Shunt::setFullChargeVoltage(float voltage) {
  ConfigManager& config = ConfigManager::getInstance();
  fullChargeVoltage = voltage;
  config.set<float>(ConfigKey::FULL_CHARGE_VOLTAGE, voltage);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Full charge voltage set to %.1fV", voltage);
  logger.info(message);
}

void Shunt::setFullChargeCurrent(float current) {
  ConfigManager& config = ConfigManager::getInstance();
  fullChargeCurrent = current;
  config.set<float>(ConfigKey::FULL_CHARGE_CURRENT, current);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Full charge current set to %.1fA", current);
  logger.info(message);
}

void Shunt::setFullChargeDuration(uint32_t minutes) {
  ConfigManager& config = ConfigManager::getInstance();
  fullChargeDuration = minutes * 60 * 1000;  // Convert minutes to ms
  config.set<uint32_t>(ConfigKey::FULL_CHARGE_DURATION, minutes);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Full charge duration set to %d min", minutes);
  logger.info(message);
}