#include "CallbackHandler.h"
#include "sensors/Shunt.h"
#include <utils/ConfigManager.h>

CallbackHandler& CallbackHandler::getInstance() {
  static CallbackHandler instance;
  return instance;
}

CallbackHandler::CallbackHandler() {
  logger.prependLog = [] { return "CALLBACK"; };
}

void CallbackHandler::init() { setupAllowed = true; }

void CallbackHandler::closeSetup() {
  setupAllowed = false;
  logger.info("Setup closed");
}

bool CallbackHandler::isSetupAllowed() const { return setupAllowed; }

bool CallbackHandler::isAllowed() {
  if (setupAllowed) {
    return true;
  }
  logger.warning("Setup already closed, ignoring received value");
  return false;
}

void CallbackHandler::handleMaxAmpCallback(String const& value) {
  logger.info(("Received Max Amp: " + value).c_str());
  if (isAllowed()) {
    Shunt& shunt = Shunt::getInstance();
    long long const maxAmpHours = strtoll(value.c_str(), nullptr, 10);
    shunt.setMaxCapacity(maxAmpHours);
  }
}

void CallbackHandler::handleSOCPercent(String const& value) {
  logger.info(("Received SOC percent: " + value).c_str());
  if (isAllowed()) {
    Shunt& shunt = Shunt::getInstance();
    long long const socPercent = strtoll(value.c_str(), nullptr, 10);
    shunt.setCurrentStateOfCharge(socPercent);
  }
}

void CallbackHandler::handleChargeEfficiency(String const& value) {
  logger.info(("Received Charge Efficiency: " + value).c_str());
  if (isAllowed()) {
    Shunt& shunt = Shunt::getInstance();
    uint8_t const chargeEfficiency = strtol(value.c_str(), nullptr, 10);
    shunt.setChargeEfficiency(chargeEfficiency);
  }
}

void CallbackHandler::handleMQTTUser(String const& value) {
  logger.info(("Received MQTT User: " + value).c_str());
  if (isAllowed()) {
    ConfigManager& config = ConfigManager::getInstance();
    config.set<String>(ConfigKey::MQTT_USER, value);
    logger.info("MQTT User updated");
  }
}

void CallbackHandler::handleMQTTPassword(String const& value) {
  logger.info("Received MQTT Password");
  if (isAllowed()) {
    ConfigManager& config = ConfigManager::getInstance();
    config.set<String>(ConfigKey::MQTT_PASSWORD, value);
    logger.info("MQTT Password updated");
  }
}

void CallbackHandler::handleMQTTServer(String const& value) {
  logger.info(("Received MQTT Server: " + value).c_str());
  if (isAllowed()) {
    ConfigManager& config = ConfigManager::getInstance();
    config.set<String>(ConfigKey::MQTT_SERVER, value);
    logger.info("MQTT Server updated");
  }
}

void CallbackHandler::handleMQTTPort(String const& value) {
  logger.info(("Received MQTT Port: " + value).c_str());
  if (isAllowed()) {
    ConfigManager& config = ConfigManager::getInstance();
    uint16_t const port = strtol(value.c_str(), nullptr, 10);
    config.set<uint16_t>(ConfigKey::MQTT_PORT, port);
    logger.info("MQTT Port updated");
  }
}