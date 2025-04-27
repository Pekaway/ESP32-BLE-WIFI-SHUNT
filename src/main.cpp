#include "CallbackHandler.h"
#include "constants.h"
#include "network/MQTTManager.h"
#include <network/BluetoothManager.h>
#include <network/UartManager.h>
#include <network/WiFiManagerPortal.h>
#include <sensors/ExternalBattery.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <utils/NeoPixel.h>
#include <utils/ResetManager.h>
#include <Arduino.h>
#include <Logger.h>

Logger logger(Serial);

ConfigManager& config = ConfigManager::getInstance();
BluetoothManager& btManager = BluetoothManager::getInstance();
MQTTManager& mqttManager = MQTTManager::getInstance();
WiFiManagerPortal& wifiPortal = WiFiManagerPortal::getInstance();
Shunt& shunt = Shunt::getInstance();
ResetManager& resetManager = ResetManager::getInstance();
NeoPixel& pixel = NeoPixel::getInstance();
CallbackHandler& callbackHandler = CallbackHandler::getInstance();
ExternalBattery& externalBattery = ExternalBattery::getInstance();

BLECharacteristic* shuntStatusChar;
BLECharacteristic* batteryConfigChar;
BLECharacteristic* wifiConfigChar;
BLECharacteristic* mqttConfigChar;

unsigned long startUpTime = millis();

void setup() {
  Serial.begin(SERIAL_SPEED);
  logger.info("Starting setup...");

  config.init();
  resetManager.init();
  callbackHandler.init();
  externalBattery.init();

  if (Shunt& shunt = Shunt::getInstance(); !shunt.init()) {
    logger.critical("Failed to initialize Shunt");
    return;
  }

  btManager.init(BLE_SERVER_NAME);

  shuntStatusChar = btManager.createNotifyCharacteristic(SHUNT_STATUS_CHAR_UUID);

  batteryConfigChar = btManager.createWriteCharacteristic(
      BATTERY_CONFIG_CHAR_UUID, [](String const& value) { callbackHandler.handleBatteryConfig(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  mqttConfigChar = btManager.createWriteCharacteristic(
      MQTT_CONFIG_CHAR_UUID, [](String const& value) { callbackHandler.handleMQTTConfig(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  wifiConfigChar = btManager.createWriteCharacteristic(
      WIFI_CHAR_UUID, [](String const& value) { callbackHandler.handleWiFi(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  btManager.startAdvertising();

  wifiPortal.begin();
  mqttManager.begin();

  logger.info("Setup complete");
}

unsigned long lastShuntUpdateTime = 0;

void loop() {
  unsigned long const currentTime = millis();

  resetManager.handle();

  if (callbackHandler.isSetupAllowed() && startUpTime + SETUP_TIME < currentTime) {
    callbackHandler.closeSetup();
    pixel.closeSetup();
  }

  if (currentTime - lastShuntUpdateTime >= SHUNT_UPDATE_INTERVAL) {
    lastShuntUpdateTime = currentTime;

    callbackHandler.updateBatteryConfigCharacteristic(batteryConfigChar);
    callbackHandler.updateShuntStatus(shuntStatusChar);
    CallbackHandler::updateWifiConfigCharacteristic(wifiConfigChar);
    callbackHandler.updateMqttConfigCharacteristic(mqttConfigChar);

    mqttManager.publishShuntValues();
  }

  shunt.update();

  wifiPortal.handle();
  mqttManager.handle();
  btManager.handle();
  pixel.handle();

  delay(1000);
}