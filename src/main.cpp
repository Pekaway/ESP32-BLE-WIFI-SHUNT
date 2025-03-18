#include "constants.h"
#include "network/MQTTManager.h"
#include <network/BluetoothManager.h>
#include <network/WiFiManagerPortal.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <Arduino.h>
#include <Logger.h>

BluetoothManager& btManager = BluetoothManager::getInstance();
Logger logger(Serial);
WiFiManagerPortal wifiPortal;
MQTTManager mqttManager;

BLECharacteristic* voltageChar;
BLECharacteristic* currentChar;
BLECharacteristic* socChar;

bool setupAllowed = true;
unsigned long startUpTime = millis();

void receivedMaxAmpCallback(String value) {
  logger.info(("Received Max Amp: " + value).c_str());
  if (setupAllowed) {
    Shunt& shunt = Shunt::getInstance();
    long long const maxAmpHours = strtoll(value.c_str(), nullptr, 10);
    shunt.setMaxCapacity(maxAmpHours);
  } else {
    logger.warning("Setup already closed, ignoring received max amp hours");
  }
}

void receivedSOCPercent(String value) {
  logger.info(("Received SOC percent: " + value).c_str());
  if (setupAllowed) {
    Shunt& shunt = Shunt::getInstance();
    long long const socPercent = strtoll(value.c_str(), nullptr, 10);
    shunt.setCurrentStateOfCharge(socPercent);
  } else {
    logger.warning("Setup already closed, ignoring received SOC percent");
  }
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  logger.info("Starting setup...");

  ConfigManager& config = ConfigManager::getInstance();
  config.init();

  Shunt& shunt = Shunt::getInstance();
  if (!shunt.init(SHUNT_MICRO_OHM, MAXIMUM_AMPS)) {
    logger.critical("Failed to initialize Shunt");
    return;
  }

  btManager.init(BLE_SERVER_NAME);
  BLEService* shuntService = btManager.createService(SERVICE_UUID);

  // Initialize BLE characteristics
  voltageChar =
      btManager.createReadCharacteristic(shuntService, VOLTAGE_CHAR_UUID);
  currentChar =
      btManager.createReadCharacteristic(shuntService, CURRENT_CHAR_UUID);
  socChar = btManager.createReadCharacteristic(shuntService, SOC_CHAR_UUID);
  btManager.createWriteCharacteristic(shuntService, MAX_AMP_HOURS_CHAR_UUID,
                                      receivedMaxAmpCallback);
  btManager.createWriteCharacteristic(shuntService, SOC_PERCENT_CHAR_UUID,
                                      receivedSOCPercent);

  btManager.startAdvertising();

  // Start WiFiManager portal
  wifiPortal.begin();

  // Start MQTTManager
  mqttManager.begin();

  logger.info("Setup complete");
}

void loop() {
  if (setupAllowed && startUpTime + 60000 * 2 < millis()) {
    setupAllowed = false;
    logger.info("Setup closed");
  }

  Shunt& shunt = Shunt::getInstance();
  shunt.update();

  // Update BLE characteristics
  btManager.updateCharacteristicValue(voltageChar,
                                      String(shunt.getBusVoltage()).c_str());
  btManager.updateCharacteristicValue(currentChar,
                                      String(shunt.getBusCurrent()).c_str());
  btManager.updateCharacteristicValue(socChar,
                                      String(shunt.getStateOfCharge()).c_str());

  // Handle WiFiManager client requests
  wifiPortal.handle();

  // Handle MQTT client
  mqttManager.handle();

  // Publish shunt values to MQTT
  // mqttManager.publishShuntValues();

  delay(100);
}