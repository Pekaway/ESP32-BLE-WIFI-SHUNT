#include "CallbackHandler.h"
#include "constants.h"
#include "network/MQTTManager.h"
#include <network/BluetoothManager.h>
#include <network/WiFiManagerPortal.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <Arduino.h>
#include <Logger.h>

Logger logger(Serial);

BluetoothManager& btManager = BluetoothManager::getInstance();
MQTTManager& mqttManager = MQTTManager::getInstance();
WiFiManagerPortal& wifiPortal = WiFiManagerPortal::getInstance();
CallbackHandler& callbackHandler = CallbackHandler::getInstance();
Shunt& shunt = Shunt::getInstance();

BLECharacteristic* shuntStatusChar;
BLECharacteristic* batteryConfigChar;

unsigned long startUpTime = millis();

void setup() {
  Serial.begin(SERIAL_SPEED);
  logger.info("Starting setup...");

  ConfigManager& config = ConfigManager::getInstance();
  config.init();

  callbackHandler.init();

  if (Shunt& shunt = Shunt::getInstance(); !shunt.init(MAXIMUM_AMPS)) {
    logger.critical("Failed to initialize Shunt");
    return;
  }

  btManager.init(BLE_SERVER_NAME, SERVICE_UUID);

  shuntStatusChar = btManager.createNotifyCharacteristic(SHUNT_STATUS_CHAR_UUID);

  batteryConfigChar = btManager.createWriteCharacteristic(
      BATTERY_CONFIG_CHAR_UUID, [](String const& value) { callbackHandler.handleBatteryConfig(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  btManager.createWriteCharacteristic(MQTT_CONFIG_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMQTTConfig(value); });
  btManager.createWriteCharacteristic(WIFI_CHAR_UUID, [](String const& value) { callbackHandler.handleWiFi(value); });

  btManager.startAdvertising();

  wifiPortal.begin();
  mqttManager.begin();

  logger.info("Setup complete");
}

void updateShuntStatus() {
  JsonDocument doc;
  doc["voltage"] = shunt.getBusVoltage();
  doc["current"] = shunt.getBusCurrent();
  doc["soc"] = shunt.getStateOfCharge();
  doc["capacity"] = shunt.getMaxCapacity();
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();
  doc["time"] = millis();

  String statusJson;
  serializeJson(doc, statusJson);
  shuntStatusChar->setValue(statusJson.c_str());
  shuntStatusChar->notify();
}

void updateBatteryConfigCharacteristic() {
  ConfigManager& config = ConfigManager::getInstance();

  JsonDocument doc;
  doc["maxCapacity"] = shunt.getMaxCapacity();
  doc["socPercent"] = static_cast<uint8_t>(shunt.calculateStateOfCharge());
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();
  doc["maxAmps"] = config.get<float>(ConfigKey::MAXIMUM_AMPS, 0);
  doc["fullChargeVoltage"] = shunt.getFullChargeVoltage();
  doc["fullChargeCurrent"] = shunt.getFullChargeCurrent();
  doc["fullChargeDuration"] = shunt.getFullChargeDuration();

  String jsonString;
  serializeJson(doc, jsonString);

  batteryConfigChar->setValue(jsonString.c_str());
}

unsigned long lastShuntUpdateTime = 0;
constexpr unsigned long SHUNT_UPDATE_INTERVAL = 10000;

void loop() {
  unsigned long const currentTime = millis();

  if (callbackHandler.isSetupAllowed() && startUpTime + SETUP_TIME < currentTime) {
    callbackHandler.closeSetup();
  }

  if (currentTime - lastShuntUpdateTime >= SHUNT_UPDATE_INTERVAL) {
    lastShuntUpdateTime = currentTime;

    updateBatteryConfigCharacteristic();
    updateShuntStatus();
    mqttManager.publishShuntValues();
  }

  shunt.update();

  // Handle network services on every iteration
  wifiPortal.handle();
  mqttManager.handle();
  btManager.handle();

  delay(1000);
}