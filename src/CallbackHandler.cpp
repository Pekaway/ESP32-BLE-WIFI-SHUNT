#include "CallbackHandler.h"
#include "sensors/Shunt.h"

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