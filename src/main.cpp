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

BLECharacteristic* voltageChar;
BLECharacteristic* currentChar;
BLECharacteristic* socChar;
BLECharacteristic* chargeChar;

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

  voltageChar = btManager.createReadCharacteristic(VOLTAGE_CHAR_UUID);
  currentChar = btManager.createReadCharacteristic(CURRENT_CHAR_UUID);
  socChar = btManager.createReadCharacteristic(SOC_CHAR_UUID);
  chargeChar = btManager.createReadCharacteristic(CHARGE_EFFICIENCY_CHAR_UUID);

  btManager.createWriteCharacteristic(MAX_AMP_HOURS_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMaxAmpCallback(value); });
  btManager.createWriteCharacteristic(SOC_PERCENT_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleSOCPercent(value); });
  btManager.createWriteCharacteristic(CHARGE_EFFICIENCY_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleChargeEfficiency(value); });
  btManager.createWriteCharacteristic(MQTT_USER_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMQTTUser(value); });
  btManager.createWriteCharacteristic(MQTT_PASSWORD_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMQTTPassword(value); });
  btManager.createWriteCharacteristic(MQTT_SERVER_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMQTTServer(value); });
  btManager.createWriteCharacteristic(MQTT_PORT_CHAR_UUID,
                                      [](String const& value) { callbackHandler.handleMQTTPort(value); });
  btManager.createWriteCharacteristic(WIFI_CHAR_UUID, [](String const& value) { callbackHandler.handleWiFi(value); });

  btManager.startAdvertising();

  wifiPortal.begin();
  mqttManager.begin();

  logger.info("Setup complete");
}

void loop() {
  if (callbackHandler.isSetupAllowed() && startUpTime + 60000 * 2 < millis()) {
    callbackHandler.closeSetup();
  }

  Shunt& shunt = Shunt::getInstance();
  shunt.update();

  voltageChar->setValue(String(shunt.getBusVoltage()).c_str());
  currentChar->setValue(String(shunt.getBusCurrent()).c_str());
  socChar->setValue(String(shunt.getStateOfCharge()).c_str());
  chargeChar->setValue(String(shunt.getChargeEfficiency()).c_str());

  wifiPortal.handle();
  mqttManager.handle();
  btManager.handle();

  mqttManager.publishShuntValues();

  delay(1000);
}