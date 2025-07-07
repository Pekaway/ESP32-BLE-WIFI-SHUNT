#include "CallbackHandler.h"
#include "constants.h"
#include "network/MQTTManager.h"
#include <network/BluetoothManager.h>
#include <sensors/ExternalBattery.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <utils/NeoPixel.h>
#include <utils/ResetManager.h>
#include <Arduino.h>
#include <Logger.h>
#include <WiFi.h>

Logger logger(Serial);

ConfigManager& config = ConfigManager::getInstance();
BluetoothManager& btManager = BluetoothManager::getInstance();
MQTTManager& mqttManager = MQTTManager::getInstance();
Shunt& shunt = Shunt::getInstance();
ResetManager& resetManager = ResetManager::getInstance();
NeoPixel& pixel = NeoPixel::getInstance();
CallbackHandler& callbackHandler = CallbackHandler::getInstance();
ExternalBattery& externalBattery = ExternalBattery::getInstance();

BLECharacteristic* shuntStatusChar;
BLECharacteristic* batteryConfigChar;
BLECharacteristic* wifiConfigChar;
BLECharacteristic* mqttConfigChar;
BLECharacteristic* shuntConfigChar;

unsigned long startUpTime = millis();

void setup() {
  Serial.begin(SERIAL_SPEED);
  logger.info("Starting setup...");

  config.init();
  resetManager.init();
  callbackHandler.init();
  externalBattery.init();

  WiFiClass::mode(WIFI_STA);
  auto const ssid = config.get<String>(ConfigKey::WIFI_SSID);
  auto const password = config.get<String>(ConfigKey::WIFI_PASSWORD);
  WiFi.begin(ssid, password);

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
  shuntConfigChar = btManager.createWriteCharacteristic(
      SHUNT_CONFIG_CHAR_UUID, [](String const& value) { callbackHandler.handleShuntConfig(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  btManager.startAdvertising();
  callbackHandler.updateBatteryConfigCharacteristic(batteryConfigChar);
  callbackHandler.updateShuntStatus(shuntStatusChar);
  callbackHandler.updateWifiConfigCharacteristic(wifiConfigChar);
  callbackHandler.updateMqttConfigCharacteristic(mqttConfigChar);
  callbackHandler.updateShuntConfigCharacteristic(shuntConfigChar);

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
    callbackHandler.updateWifiConfigCharacteristic(wifiConfigChar);
    callbackHandler.updateMqttConfigCharacteristic(mqttConfigChar);
    callbackHandler.updateShuntConfigCharacteristic(shuntConfigChar);

    mqttManager.publishShuntValues();
  }

  shunt.update();

  mqttManager.handle();
  btManager.handle();
  pixel.handle();

  delay(1000);
}