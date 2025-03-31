#include "CallbackHandler.h"
#include "sensors/Shunt.h"
#include <utils/ConfigManager.h>
#include <WiFi.h>
#include <WiFiManager.h>

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

void CallbackHandler::handleMQTTConfig(String const& value) {
  logger.info("Received MQTT Config JSON");
  if (isAllowed()) {
    JsonDocument doc;
    DeserializationError const error = deserializeJson(doc, value);

    if (error) {
      logger.critical(("JSON parsing failed: " + String(error.c_str())).c_str());
      return;
    }

    ConfigManager& config = ConfigManager::getInstance();

    if (doc["user"].is<String>()) {
      String const user = doc["user"];
      logger.info(("MQTT User: " + user).c_str());
      config.set<String>(ConfigKey::MQTT_USER, user);
    }

    if (doc["password"].is<String>()) {
      String const password = doc["password"];
      logger.info("MQTT Password updated");
      config.set<String>(ConfigKey::MQTT_PASSWORD, password);
    }

    if (doc["server"].is<String>()) {
      String const server = doc["server"];
      logger.info(("MQTT Server: " + server).c_str());
      config.set<String>(ConfigKey::MQTT_SERVER, server);
    }

    if (doc["port"].is<u16_t>()) {
      uint16_t const port = doc["port"];
      logger.info(("MQTT Port: " + String(port)).c_str());
      config.set<uint16_t>(ConfigKey::MQTT_PORT, port);
    }

    logger.info("MQTT Configuration updated");
  }
}

void CallbackHandler::handleWiFi(String const& value) {
  logger.info(("Received WiFi: " + value).c_str());
  if (isAllowed()) {
    JsonDocument doc;
    DeserializationError const error = deserializeJson(doc, value);

    if (error) {
      logger.critical(("JSON parsing failed: " + String(error.c_str())).c_str());
      return;
    }

    String const ssid = doc["ssid"];
    String const password = doc["password"];

    WiFi.disconnect();
    WiFi.persistent(true);
    WiFi.begin(ssid, password);
    WiFi.persistent(false);

    logger.info("WiFi SSID updated");
  }
}

void CallbackHandler::handleBatteryConfig(String const& value) {
  logger.info("Received battery configuration");
  if (!isAllowed()) {
    logger.warning("Setup no longer allowed - ignoring battery configuration");
    return;
  }

  // Validate JSON
  JsonDocument doc;
  DeserializationError const error = deserializeJson(doc, value);

  if (error) {
    logger.critical(("Battery JSON parsing failed: " + String(error.c_str())).c_str());
    return;
  }

  ConfigManager& config = ConfigManager::getInstance();
  Shunt& shunt = Shunt::getInstance();
  bool configChanged = false;

  // Handle max capacity
  if (doc["maxCapacity"].is<uint32_t>()) {
    if (uint32_t const maxCapacity = doc["maxCapacity"]; maxCapacity > 0 && maxCapacity <= 10000) {
      logger.info(("Setting max capacity: " + String(maxCapacity)).c_str());
      shunt.setMaxCapacity(maxCapacity);
      configChanged = true;
    } else {
      logger.warning("Invalid max capacity value (must be uint32_t)");
    }
  }

  // Handle SOC percent
  if (doc["socPercent"].is<uint8_t>()) {
    if (uint8_t const socPercent = doc["socPercent"]; socPercent > 0 && socPercent <= 100) {
      logger.info(("Setting SOC percent: " + String(socPercent)).c_str());

      // TODO

      configChanged = true;
    } else {
      logger.warning("Invalid SOC percent value (must be 0-100)");
    }
  }

  // Handle charge efficiency
  if (doc["chargeEfficiency"].is<uint8_t>()) {
    if (uint8_t const efficiency = doc["chargeEfficiency"]; efficiency > 0 && efficiency <= 100) {
      logger.info(("Setting charge efficiency: " + String(efficiency)).c_str());
      shunt.setChargeEfficiency(efficiency);
      configChanged = true;
    } else {
      logger.warning("Invalid charge efficiency value (must be 0-100)");
    }
  }

  if (doc["maxAmps"].is<float>()) {
    if (float const maxAmps = doc["maxAmps"]; maxAmps > 0 && maxAmps <= 1022) {
      logger.info(("Setting maximum amps: " + String(maxAmps)).c_str());
      config.set<float>(ConfigKey::MAXIMUM_AMPS, maxAmps);
      configChanged = true;
    } else {
      logger.warning("Invalid maximum amps value (must be 0-1022)");
    }
  }

  if (configChanged) {
    logger.info("Battery configuration updated");
  } else {
    logger.warning("No valid battery configuration parameters provided");
  }
}