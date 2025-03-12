#include <network/BluetoothManager.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <Arduino.h>
#include <Logger.h>
#include "Constants.h"

BluetoothManager& btManager = BluetoothManager::getInstance();
Logger logger(Serial);

BLECharacteristic* voltageChar;
BLECharacteristic* currentChar;
BLECharacteristic* socChar;

bool setupAllowed = true;
unsigned long startUpTime = millis();

void receivedMaxAmpCallback(std::string value) {
  logger.info(("Received Max Amp: " + value).c_str());
  if (setupAllowed) {
    Shunt& shunt = Shunt::getInstance();
    long long const maxAmpHours = std::stoll(value);
    shunt.setMaxCapacity(maxAmpHours);
  } else {
    logger.warning("Setup already closed, ignoring received max amp hours");
  }
}

void receivedSOCPercent(std::string value) {
  logger.info(("Received SOC percent: " + value).c_str());
  if (setupAllowed) {
    Shunt& shunt = Shunt::getInstance();
    long long const socPercent = std::stoll(value);
    shunt.setCurrentStateOfCharge(socPercent);
  } else {
    logger.warning("Setup already closed, ignoring received SOC percent");
  }
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  logger.info("Starting setup...");

  Shunt& shunt = Shunt::getInstance();
  if (!shunt.init(SHUNT_MICRO_OHM, MAXIMUM_AMPS)) {
    logger.critical("Failed to initialize Shunt");
    return;
  }

  btManager.init(BLE_SERVER_NAME);
  BLEService* shuntService = btManager.createService(SERVICE_UUID);

  // Initialize BLE characteristics
  voltageChar = btManager.createReadCharacteristic(shuntService, VOLTAGE_CHAR_UUID);
  currentChar = btManager.createReadCharacteristic(shuntService, CURRENT_CHAR_UUID);
  socChar = btManager.createReadCharacteristic(shuntService, SOC_CHAR_UUID);
  btManager.createWriteCharacteristic(shuntService, MAX_AMP_HOURS_CHAR_UUID, receivedMaxAmpCallback);
  btManager.createWriteCharacteristic(shuntService, SOC_PERCENT_CHAR_UUID, receivedSOCPercent);

  btManager.startAdvertising();

  logger.info("Setup complete");
}

void loop() {
  if (startUpTime + 60.000 * 2 < millis()) {
    setupAllowed = false;
    logger.info("Setup closed");
  }

  Shunt& shunt = Shunt::getInstance();
  shunt.update();

  // Update BLE characteristics
  btManager.updateCharacteristicValue(voltageChar, String(shunt.getBusVoltage()).c_str());
  btManager.updateCharacteristicValue(currentChar, String(shunt.getBusCurrent()).c_str());
  btManager.updateCharacteristicValue(socChar, String(shunt.getStateOfCharge()).c_str());
}
