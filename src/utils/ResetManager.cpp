#include "./ResetManager.h"
#include "../constants.h"
#include "ConfigManager.h"
#include "NeoPixel.h"

ResetManager& ResetManager::getInstance() {
  static ResetManager instance;
  return instance;
}

ResetManager::ResetManager() {
  logger.prependLog = [] { return "RESET"; };
}

bool ResetManager::init() {
  if (!loadResetData()) {
    logger.info("Creating default reset data");
    createDefaultResetData();
    saveResetData();
  }

  String m;
  serializeJson(jsonDoc, m);
  logger.info(("Reset data loaded: " + m).c_str());

  uint8_t const bootCount = jsonDoc["bootCount"].as<uint8_t>() + 1;
  jsonDoc["bootCount"] = bootCount;
  logger.info(("Quick reboot detected. Count: " + String(bootCount)).c_str());
  saveResetData();

  checkResetCondition();
  return true;
}

void ResetManager::handle() {
  if (!didReset && millis() > RESET_WINDOW_MS) {
    resetCounters();
    logger.info("Reset window expired, resetting counters");
    didReset = true;
  }
}

void ResetManager::checkResetCondition() {
  if (auto const bootCount = jsonDoc["bootCount"].as<uint8_t>(); bootCount >= RESET_THRESHOLD) {
    logger.info("Reboot threshold reached! Resetting configuration...");
    NeoPixel& pixel = NeoPixel::getInstance();
    pixel.red();

    ConfigManager& config = ConfigManager::getInstance();
    config.resetToDefaults();
    resetCounters();

    delay(5000);
    ESP.restart();
  }
}

void ResetManager::resetCounters() {
  createDefaultResetData();
  logger.info("Reboot counters reset");
}

bool ResetManager::loadResetData() {
  if (!LittleFS.begin(true)) {
    logger.info("Failed to mount LittleFS");
    return false;
  }

  if (!LittleFS.exists(RESET_FILE_PATH)) {
    logger.info("Reset data file doesn't exist");
    return false;
  }

  File file = LittleFS.open(RESET_FILE_PATH, "r");
  if (!file) {
    logger.info("Failed to open reset data file for reading");
    return false;
  }

  DeserializationError const error = deserializeJson(jsonDoc, file);
  file.close();

  if (error) {
    logger.info("Failed to parse reset data file");
    return false;
  }

  logger.info("Reset data loaded successfully");
  return true;
}

bool ResetManager::saveResetData() {
  File file = LittleFS.open(RESET_FILE_PATH, "w");
  if (!file) {
    logger.info("Failed to open reset data file for writing");
    return false;
  }

  size_t const bytesWritten = serializeJson(jsonDoc, file);
  file.close();

  if (bytesWritten == 0) {
    logger.info("Failed to write reset data");
    return false;
  }

  logger.info("Reset data saved successfully");
  return true;
}

void ResetManager::createDefaultResetData() {
  jsonDoc.clear();
  jsonDoc["bootCount"] = 0;
  saveResetData();
}
