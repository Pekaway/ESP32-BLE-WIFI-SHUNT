#ifndef RESETMANAGER_H
#define RESETMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Logger.h>

class ResetManager {
 public:
  ResetManager(ResetManager const&) = delete;
  ResetManager& operator=(ResetManager const&) = delete;

  static ResetManager& getInstance();

  bool init();
  void handle();

 private:
  ResetManager();
  ~ResetManager() = default;

  static constexpr auto RESET_FILE_PATH = "/reset_data.json";

  JsonDocument jsonDoc;
  bool didReset = false;
  Logger logger = Logger(Serial);

  bool loadResetData();
  bool saveResetData();
  void createDefaultResetData();
  void checkResetCondition();
  void resetCounters();
};

#endif  // RESETMANAGER_H
