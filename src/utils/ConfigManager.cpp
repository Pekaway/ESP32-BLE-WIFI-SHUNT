#include "ConfigManager.h"

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    logger.prependLog = [] {
        return "CONFIG";
    };
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

bool ConfigManager::hasKey(const String& key) {
    return jsonDoc[key].is<JsonVariant>();
}

String* ConfigManager::getKeys(int& count) {
    count = 0;
    for (JsonPair kv : jsonDoc.as<JsonObject>()) {
        count++;
    }

    String* keys = new String[count];
    int i = 0;
    for (JsonPair kv : jsonDoc.as<JsonObject>()) {
        keys[i++] = kv.key().c_str();
    }

    return keys;
}

bool ConfigManager::writeConfigFile() {
    File file = LittleFS.open(configFilePath, "w");
    if (!file) {
        logger.info("Failed to open config file for writing");
        return false;
    }

    size_t bytesWritten = serializeJson(jsonDoc, file);
    file.close();

    if (bytesWritten == 0) {
        logger.info("Failed to write to config file");
        return false;
    }

    return true;
}

bool ConfigManager::readConfigFile() {
    if (!LittleFS.exists(configFilePath)) {
        logger.info("Config file doesn't exist");
        return false;
    }

    File file = LittleFS.open(configFilePath, "r");
    if (!file) {
        logger.info("Failed to open config file for reading");
        return false;
    }

    DeserializationError error = deserializeJson(jsonDoc, file);
    file.close();

    if (error) {
        logger.info("Failed to parse config file");
        return false;
    }

    return true;
}

void ConfigManager::createDefaultConfig() {
    jsonDoc.clear();

    jsonDoc["device_name"] = "PekawayShunt";
    jsonDoc["max_capacity"] = 100; // Default 100Ah
    jsonDoc["initial_soc"] = 80;   // Default 80%
    jsonDoc["shunt_micro_ohm"] = 375;
    jsonDoc["maximum_amps"] = 1022;
    jsonDoc["auto_save_interval"] = 30; // seconds
}