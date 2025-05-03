#include "Shunt.h"
#include "../utils/ConfigManager.h"

Shunt& Shunt::getInstance() {
  static Shunt instance;
  return instance;
}

Shunt::Shunt() {
  logger.prependLog = [] { return "SHUNT"; };
}

bool Shunt::init() {
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

double Shunt::getBusVoltage() { return ina.getBusMilliVolts(0) / 1000.0; }

double Shunt::getBusCurrent() {
  // negative is charging, positive is discharging
  return ina.getBusMicroAmps(0) / 1000000.0 * -1.0;
}

double Shunt::getPower() { return ina.getBusMicroWatts(0) / 1000000.0; }

float Shunt::getStateOfCharge() const { return calculateStateOfCharge(); }

void Shunt::setMaxCapacity(uint32_t const ampHours) {
  maxCapacityMilliAmpMs = ampHours * 60LL * 60LL * 1000LL * 1000LL;

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

double Shunt::getChargeEfficiency() const { return this->chargeEfficiency; }

long long Shunt::getMaxCapacity() const { return maxCapacityMilliAmpMs / (60LL * 60LL * 1000LL * 1000LL); }

bool Shunt::loadConfig() {
  ConfigManager& config = ConfigManager::getInstance();

  uint32_t const maxCapacity = config.get<int>(ConfigKey::MAXIMUM_AMPS, 100);  // Default 100Ah
  maxCapacityMilliAmpMs = static_cast<int64_t>(maxCapacity) * 60LL * 60LL * 1000LL * 1000LL;

  if (config.hasKey(ConfigKey::CURRENT_CAPACITY_MAMS)) {
    currentCapacityMilliAmpMs = config.get<long long>(ConfigKey::CURRENT_CAPACITY_MAMS);
  }

  fullChargeVoltage = config.get<float>(ConfigKey::FULL_CHARGE_VOLTAGE);
  fullChargeCurrent = config.get<float>(ConfigKey::FULL_CHARGE_CURRENT);
  fullChargeDuration = config.get<uint32_t>(ConfigKey::FULL_CHARGE_DURATION) * 60 * 1000;

  lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;

  char message[100];
  snprintf(message, sizeof(message), "Loaded max capacity: %d Ah, SOC: %i%%", maxCapacity, calculateStateOfCharge());
  logger.info(message);

  return true;
}

bool Shunt::saveStateToConfig() {
  ConfigManager& config = ConfigManager::getInstance();

  config.set(ConfigKey::CURRENT_CAPACITY_MAMS, currentCapacityMilliAmpMs);

  bool const result = config.saveConfig();
  if (result) {
    char message[100];
    snprintf(message, sizeof(message), "Stored capacity: %lld mA-ms, SOC: %i%%", currentCapacityMilliAmpMs,
             calculateStateOfCharge());
    logger.info(message);
  } else {
    logger.critical("Failed to store battery state");
  }

  return result;
}

void Shunt::update() {
  auto const currentMillis = millis();
  double capacityDeltaMilliAmpMs = 0;

  if (lastUpdateMillis > 0) {
    // Calculate consumed or recharged capacity since last update
    auto const elapsedMs = currentMillis - lastUpdateMillis;

    if (getBusCurrent() < 0) {
      // Charging
      auto const currentAmps = getBusCurrent() * chargeEfficiency / 100;
      capacityDeltaMilliAmpMs = (currentAmps * 1000 * elapsedMs);

      currentCapacityMilliAmpMs -= capacityDeltaMilliAmpMs;
    } else if (getBusCurrent() > 0) {
      // Discharging
      capacityDeltaMilliAmpMs = (getBusCurrent() * 1000 * elapsedMs);
    }

    currentCapacityMilliAmpMs -= capacityDeltaMilliAmpMs;

    clampStateOfCharge();
  }

  auto const voltage = getBusVoltage();
  auto const current = getBusCurrent();
  auto const soc = calculateStateOfCharge();

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

  if (constexpr auto autoSaveInterval = AUTO_SAVE_INTERVAL * 1000;
      currentMillis - lastStorageMillis >= autoSaveInterval) {
    lastStorageMillis = currentMillis;

    if (fullChargeConditionMet) {
      currentCapacityMilliAmpMs = maxCapacityMilliAmpMs;
    }

    saveStateToConfig();
    lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;
  }

  lastUpdateMillis = currentMillis;
}

uint16_t Shunt::calculateStateOfCharge() const {
  if (maxCapacityMilliAmpMs <= 0) return 0;

  return static_cast<uint16_t>(static_cast<double>(currentCapacityMilliAmpMs) / maxCapacityMilliAmpMs * 100.0);
}

double Shunt::getTTGO() {
  double ttgo = 0.0;

  if (double const busCurrent = getBusCurrent(); busCurrent < -0.01f) {
    // Charging - calculate time to full
    int64_t const remainingCapacity = maxCapacityMilliAmpMs - currentCapacityMilliAmpMs;

    if (double const chargingCurrentMilliA =
            abs(busCurrent) * 1000.0f * (static_cast<float>(chargeEfficiency) / 100.0f);
        chargingCurrentMilliA > 0) {
      ttgo = static_cast<double>(remainingCapacity) / (chargingCurrentMilliA);
      ttgo /= (60.0 * 60.0 * 1000.0);
    } else {
      ttgo = 9999.0;
    }
  } else if (busCurrent > 0.01f) {
    // Discharging - calculate time remaining

    if (double const dischargingCurrentMilliA = busCurrent * 1000.0f; dischargingCurrentMilliA > 0) {
      ttgo = static_cast<double>(currentCapacityMilliAmpMs) / dischargingCurrentMilliA;
      ttgo /= (60.0 * 60.0 * 1000.0);
    } else {
      ttgo = 9999.0;
    }
  } else {
    // Current near zero - consider it infinite or N/A
    ttgo = 9999.0;
  }

  // Convert hours to milliseconds for return value
  return ttgo * 60.0 * 60.0 * 1000.0;
}

void Shunt::clampStateOfCharge() {
  if (currentCapacityMilliAmpMs > maxCapacityMilliAmpMs) {
    currentCapacityMilliAmpMs = maxCapacityMilliAmpMs;
  }

  if (currentCapacityMilliAmpMs < 0) {
    currentCapacityMilliAmpMs = 0;
  }
}

void Shunt::setFullChargeVoltage(float const voltage) {
  ConfigManager& config = ConfigManager::getInstance();
  fullChargeVoltage = voltage;
  config.set<float>(ConfigKey::FULL_CHARGE_VOLTAGE, voltage);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Full charge voltage set to %.1fV", voltage);
  logger.info(message);
}

void Shunt::setFullChargeCurrent(float const current) {
  ConfigManager& config = ConfigManager::getInstance();
  fullChargeCurrent = current;
  config.set<float>(ConfigKey::FULL_CHARGE_CURRENT, current);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Full charge current set to %.1fA", current);
  logger.info(message);
}

void Shunt::setFullChargeDuration(uint32_t const minutes) {
  ConfigManager& config = ConfigManager::getInstance();
  fullChargeDuration = minutes * 60 * 1000;  // Convert minutes to ms
  config.set<uint32_t>(ConfigKey::FULL_CHARGE_DURATION, minutes);
  config.saveConfig();

  char message[50];
  snprintf(message, sizeof(message), "Full charge duration set to %d min", minutes);
  logger.info(message);
}