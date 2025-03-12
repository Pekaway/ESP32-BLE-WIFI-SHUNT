#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Logger.h>

class ConfigManager {
 public:
  static ConfigManager& getInstance();

  bool init();

  template <typename T>
  T get(String const& key, T defaultValue = T()) {
    if (!jsonDoc[key].is<T>()) {
      return defaultValue;
    }
    return jsonDoc[key].as<T>();
  }

  template <typename T>
  bool set(String const& key, T value) {
    jsonDoc[key] = value;
    configDirty = true;
    return true;
  }

  bool saveConfig();
  bool loadConfig();

  bool resetToDefaults();

  bool hasKey(String const& key);
  String* getKeys(int& count);

 private:
  ConfigManager();
  ~ConfigManager();
  ConfigManager(ConfigManager const&) = delete;
  ConfigManager& operator=(ConfigManager const&) = delete;

  JsonDocument jsonDoc;

  char const* configFilePath = "/config.json";

  Logger logger = Logger(Serial);

  bool configDirty = false;

  bool writeConfigFile();
  bool readConfigFile();
  void createDefaultConfig();
};

#endif  // CONFIGMANAGER_H