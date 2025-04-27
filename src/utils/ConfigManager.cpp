#include "ConfigManager.h"
#include <constants.h>

ConfigManager& ConfigManager::getInstance() {
  static ConfigManager instance;
  return instance;
}

ConfigManager::ConfigManager() {
  logger.prependLog = [] { return "CONFIG"; };
}

ConfigManager::~ConfigManager() {
  if (configDirty) {
    saveConfig();
  }
}

bool ConfigManager::init() {
  if (!LittleFS.begin(true)) {
    logger.info("Failed to mount LittleFS");
    return false;
  }

  if (!loadConfig()) {
    logger.info("Creating default configuration");
    createDefaultConfig();
    return saveConfig();
  }

  return true;
}

bool ConfigManager::saveConfig() {
  if (writeConfigFile()) {
    configDirty = false;
    logger.info("Configuration saved successfully");
    return true;
  }

  logger.info("Failed to save configuration");
  return false;
}

bool ConfigManager::loadConfig() {
  if (readConfigFile()) {
    logger.info("Configuration loaded successfully");
    return true;
  }

  logger.info("Failed to load configuration");
  return false;
}

bool ConfigManager::resetToDefaults() {
  createDefaultConfig();
  configDirty = true;
  return saveConfig();
}

bool ConfigManager::hasKey(ConfigKey const key) {
  char const* keyStr = ConfigKeys::toString(key);
  return jsonDoc[keyStr].is<JsonVariant>();
}

String* ConfigManager::getKeys(int& count) {
  count = 0;
  for (JsonPair kv : jsonDoc.as<JsonObject>()) {
    count++;
  }

  auto* keys = new String[count];
  int i = 0;
  for (JsonPair kv : jsonDoc.as<JsonObject>()) {
    keys[i++] = kv.key().c_str();
  }

  return keys;
}

bool ConfigManager::writeConfigFile() {
  File file = LittleFS.open(CONFIG_FILE_PATH, "w");
  if (!file) {
    logger.info("Failed to open config file for writing");
    return false;
  }

  size_t const bytesWritten = serializeJson(jsonDoc, file);
  file.close();

  if (bytesWritten == 0) {
    logger.info("Failed to write to config file");
    return false;
  }

  return true;
}

bool ConfigManager::readConfigFile() {
  if (!LittleFS.exists(CONFIG_FILE_PATH)) {
    logger.info("Config file doesn't exist");
    return false;
  }

  File file = LittleFS.open(CONFIG_FILE_PATH, "r");
  if (!file) {
    logger.info("Failed to open config file for reading");
    return false;
  }

  DeserializationError const error = deserializeJson(jsonDoc, file);
  file.close();

  if (error) {
    logger.info("Failed to parse config file");
    return false;
  }

  return true;
}

void ConfigManager::createDefaultConfig() {
  jsonDoc.clear();

  jsonDoc[ConfigKeys::toString(ConfigKey::DEVICE_NAME)] = BLE_SERVER_NAME;
  jsonDoc[ConfigKeys::toString(ConfigKey::INITIAL_SOC)] = SHUNT_INITIAL_SOC;
  jsonDoc[ConfigKeys::toString(ConfigKey::SHUNT_MICRO_OHM)] = SHUNT_MICRO_OHM;
  jsonDoc[ConfigKeys::toString(ConfigKey::MAXIMUM_AMPS)] = SHUNT_MAXIMUM_AMPS;
  jsonDoc[ConfigKeys::toString(ConfigKey::AUTO_SAVE_INTERVAL)] = AUTO_SAVE_INTERVAL;
  jsonDoc[ConfigKeys::toString(ConfigKey::MQTT_USER)] = "";
  jsonDoc[ConfigKeys::toString(ConfigKey::MQTT_PASSWORD)] = "";
  jsonDoc[ConfigKeys::toString(ConfigKey::MQTT_SERVER)] = "";
  jsonDoc[ConfigKeys::toString(ConfigKey::MQTT_PORT)] = 1883;
  jsonDoc[ConfigKeys::toString(ConfigKey::CURRENT_SOC)] = 0;
  jsonDoc[ConfigKeys::toString(ConfigKey::CURRENT_CAPACITY_MAMS)] = 0;
  jsonDoc[ConfigKeys::toString(ConfigKey::CHARGE_EFFICIENCY)] = 100;
  jsonDoc[ConfigKeys::toString(ConfigKey::FULL_CHARGE_VOLTAGE)] = 0;
  jsonDoc[ConfigKeys::toString(ConfigKey::FULL_CHARGE_VOLTAGE)] = 0;
  jsonDoc[ConfigKeys::toString(ConfigKey::FULL_CHARGE_DURATION)] = 0;
  jsonDoc[ConfigKeys::toString(ConfigKey::WIFI_PASSWORD)] = nullptr;
  jsonDoc[ConfigKeys::toString(ConfigKey::WIFI_SSID)] = nullptr;
}
