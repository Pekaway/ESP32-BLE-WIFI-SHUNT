#include "Shunt.h"
#include "../utils/ConfigManager.h"

Shunt& Shunt::getInstance() {
  static Shunt instance;
  return instance;
}

Shunt::Shunt() {
  logger.prependLog = [] { return "SHUNT"; };
}

bool Shunt::init(uint32_t shuntMicroOhm, uint16_t maximumAmps) {
  ConfigManager& config = ConfigManager::getInstance();

  this->maximumAmps =
      config.get<uint32_t>(ConfigKey::MAXIMUM_AMPS, maximumAmps);

  deviceCount = ina.begin(this->maximumAmps, this->shuntMicroOhm);
  if (deviceCount == 0) {
    logger.critical("No INA device found");
    return false;
  }

  char message[50];
  snprintf(message, sizeof(message), "Detected %d INA devices on the I2C bus",
           deviceCount);
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

float Shunt::getStateOfCharge() { return calculateStateOfCharge(); }

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

uint32_t Shunt::getMaxCapacity() {
  return maxCapacityMilliAmpMs / (60LL * 60LL * 1000LL * 1000LL);
}

bool Shunt::loadConfig() {
  ConfigManager& config = ConfigManager::getInstance();

  uint32_t maxCapacity =
      config.get<int>(ConfigKey::MAXIMUM_AMPS, 100);  // Default 100Ah
  maxCapacityMilliAmpMs =
      static_cast<int64_t>(maxCapacity) * 60LL * 60LL * 1000LL * 1000LL;

  int socPercentage = config.get<int>(
      ConfigKey::CURRENT_SOC, config.get<int>(ConfigKey::INITIAL_SOC, 80));
  currentCapacityMilliAmpMs = (maxCapacityMilliAmpMs / 100) * socPercentage;

  if (config.hasKey(ConfigKey::CURRENT_CAPACITY_MAMS)) {
    auto const capacityStr =
        config.get<char const*>(ConfigKey::CURRENT_CAPACITY_MAMS, "0");
    currentCapacityMilliAmpMs = strtoll(capacityStr, nullptr, 10);
  }

  lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;

  char message[100];
  snprintf(message, sizeof(message), "Loaded max capacity: %d Ah, SOC: %d%%",
           maxCapacity, socPercentage);
  logger.info(message);

  return true;
}

bool Shunt::saveStateToConfig() {
  ConfigManager& config = ConfigManager::getInstance();

  int socPercentage = static_cast<int>(calculateStateOfCharge());
  config.set(ConfigKey::CURRENT_SOC, socPercentage);

  // store current capacity in milliamp-milliseconds to preserve precision
  char capacityStr[32];
  snprintf(capacityStr, sizeof(capacityStr), "%lld", currentCapacityMilliAmpMs);
  config.set(ConfigKey::CURRENT_CAPACITY_MAMS, capacityStr);

  bool const result = config.saveConfig();

  if (result) {
    char message[100];
    snprintf(message, sizeof(message), "Stored SOC: %d%%, capacity: %lld mA-ms",
             socPercentage, currentCapacityMilliAmpMs);

    logger.info(message);
  } else {
    logger.critical("Failed to store battery state");
  }

  return result;
}

void Shunt::update() {
  unsigned long currentMillis = millis();

  if (lastUpdateMillis > 0) {
    // Calculate consumed or recharged capacity since last update
    long const elapsedMs = currentMillis - lastUpdateMillis;
    float const currentAmps = getBusCurrent();
    auto const capacityDeltaMilliAmpMs = (currentAmps * 1000 * elapsedMs);

    // Update the current capacity
    currentCapacityMilliAmpMs += capacityDeltaMilliAmpMs;
    clampStateOfCharge();
  }

  ConfigManager& config = ConfigManager::getInstance();
  uint32_t const autoSaveInterval =
      config.get<uint32_t>(ConfigKey::AUTO_SAVE_INTERVAL, 30) * 1000;

  if (currentMillis - lastStorageMillis >= autoSaveInterval) {
    lastStorageMillis = currentMillis;

    int64_t capacityChange =
        lastStoredCapacityMilliAmpMs - currentCapacityMilliAmpMs;
    if (capacityChange < 0) capacityChange = -capacityChange;

    if (capacityChange > capacityThresholdToStore) {
      saveStateToConfig();
      lastStoredCapacityMilliAmpMs = currentCapacityMilliAmpMs;
    }
  }

  lastUpdateMillis = currentMillis;
}

float Shunt::calculateStateOfCharge() {
  if (maxCapacityMilliAmpMs <= 0) return 0;

  return (static_cast<float>(currentCapacityMilliAmpMs) /
          static_cast<float>(maxCapacityMilliAmpMs)) *
         100.0f;
}

void Shunt::clampStateOfCharge() {
  if (currentCapacityMilliAmpMs > maxCapacityMilliAmpMs) {
    currentCapacityMilliAmpMs = maxCapacityMilliAmpMs;
  }

  if (currentCapacityMilliAmpMs < 0) {
    currentCapacityMilliAmpMs = 0;
  }
}