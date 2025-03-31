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

BLECharacteristic* shuntStatusChar;

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

  shuntStatusChar = btManager.createReadCharacteristic(SHUNT_STATUS_CHAR_UUID);

  btManager.createWriteCharacteristic(BATTERY_CONFIG_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleBatteryConfig(value); });
  btManager.createWriteCharacteristic(MQTT_CONFIG_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMQTTConfig(value); });
  btManager.createWriteCharacteristic(WIFI_CHAR_UUID, [](String const& value) { callbackHandler.handleWiFi(value); });

  btManager.startAdvertising();

  wifiPortal.begin();
  mqttManager.begin();

  logger.info("Setup complete");
}

String buildShuntStatusJson() {
  Shunt& shunt = Shunt::getInstance();

  JsonDocument doc;
  doc["voltage"] = shunt.getBusVoltage();
  doc["current"] = shunt.getBusCurrent();
  doc["soc"] = shunt.getStateOfCharge();
  doc["capacity"] = shunt.getMaxCapacity();
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();

  String statusJson;
  serializeJson(doc, statusJson);
  return statusJson;
}

void loop() {
  if (callbackHandler.isSetupAllowed() && startUpTime + 60000 * 2 < millis()) {
    callbackHandler.closeSetup();
  }

  Shunt& shunt = Shunt::getInstance();
  shunt.update();

  String const statusJson = buildShuntStatusJson();
  shuntStatusChar->setValue(statusJson.c_str());

  wifiPortal.handle();
  mqttManager.handle();
  btManager.handle();

  mqttManager.publishShuntValues();

  delay(1000);
}