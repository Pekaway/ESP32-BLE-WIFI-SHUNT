#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Logger.h>

class ConfigManager {
public:
  static ConfigManager& getInstance();

  bool init();

  template<typename T>
  T get(const String& key, T defaultValue = T()) {
    if (!jsonDoc[key].is<T>()) {
      return defaultValue;
    }
    return jsonDoc[key].as<T>();
  }

  template<typename T>
  bool set(const String& key, T value) {
    jsonDoc[key] = value;
    configDirty = true;
    return true;
  }

  bool saveConfig();
  bool loadConfig();

  bool resetToDefaults();

  bool hasKey(const String& key);
  String* getKeys(int& count);

private:
  ConfigManager();
  ~ConfigManager();
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  JsonDocument jsonDoc;

  const char* configFilePath = "/config.json";

  Logger logger = Logger(Serial);

  bool configDirty = false;

  bool writeConfigFile();
  bool readConfigFile();
  void createDefaultConfig();
};

#endif //CONFIGMANAGER_H