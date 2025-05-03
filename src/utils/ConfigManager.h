#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "configKeys.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Logger.h>

class ConfigManager {
 public:
  ConfigManager(ConfigManager const&) = delete;
  ConfigManager& operator=(ConfigManager const&) = delete;

  static ConfigManager& getInstance();

  bool init();
  bool saveConfig();
  bool loadConfig();
  bool resetToDefaults();
  bool hasKey(ConfigKey key);
  String* getKeys(int& count);

  template <typename T>
  T get(ConfigKey key, T defaultValue = T()) {
    char const* keyStr = ConfigKeys::toString(key);
    if (!jsonDoc[keyStr].is<T>()) {
      return defaultValue;
    }
    return jsonDoc[keyStr].as<T>();
  }

  template <typename T>
  bool set(ConfigKey key, T value) {
    char const* keyStr = ConfigKeys::toString(key);
    jsonDoc[keyStr] = value;
    configDirty = true;
    return true;
  }

 private:
  ConfigManager();
  ~ConfigManager();

  Logger logger = Logger(Serial);

  char const* CONFIG_FILE_PATH = "/config.json";

  JsonDocument jsonDoc;
  bool configDirty = false;

  bool writeConfigFile();
  bool readConfigFile();
  void createDefaultConfig();
};

#endif  // CONFIGMANAGER_H