#ifndef CALLBACKHANDLER_H
#define CALLBACKHANDLER_H

#include <sensors/ExternalBattery.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <Arduino.h>
#include <BLECharacteristic.h>
#include <Logger.h>

class CallbackHandler {
 public:
  CallbackHandler(CallbackHandler const&) = delete;
  CallbackHandler& operator=(CallbackHandler const&) = delete;

  static CallbackHandler& getInstance();

  void init();
  void closeSetup();
  [[nodiscard]] bool isSetupAllowed() const;

  void handleMQTTConfig(String const& value);
  void handleWiFi(String const& value);
  void handleBatteryConfig(String const& value);

  void updateShuntStatus(BLECharacteristic* shuntStatusChar) const;
  void updateBatteryConfigCharacteristic(BLECharacteristic* batteryConfigChar) const;
  static void updateWifiConfigCharacteristic(BLECharacteristic* wifiConfigChar);
  void updateMqttConfigCharacteristic(BLECharacteristic* mqttConfigChar) const;

 private:
  CallbackHandler();
  ~CallbackHandler() = default;

  Logger logger = Logger(Serial);
  Shunt& shunt = Shunt::getInstance();
  ConfigManager& config = ConfigManager::getInstance();
  ExternalBattery& externalBattery = ExternalBattery::getInstance();
  bool setupAllowed = true;

  bool isAllowed();
};

#endif  // CALLBACKHANDLER_H
