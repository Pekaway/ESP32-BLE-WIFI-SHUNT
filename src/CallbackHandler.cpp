#include "CallbackHandler.h"
#include "sensors/Shunt.h"
#include <network/MQTTManager.h>
#include <utils/ConfigManager.h>
#include <utils/NeoPixel.h>
#include <WiFi.h>

CallbackHandler& CallbackHandler::getInstance() {
  static CallbackHandler instance;
  return instance;
}

CallbackHandler::CallbackHandler() {
  logger.prependLog = [] { return "CALLBACK"; };
}

void CallbackHandler::init() { setupAllowed = !config.get(ConfigKey::SCHEDULED_RESTART, false); }

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

    config.saveConfig();

    MQTTManager& mqtt = MQTTManager::getInstance();
    mqtt.connect(true);
    logger.info("MQTT Configuration updated");
  }
}

void CallbackHandler::handleWiFi(String const& value) {
  logger.info("Received WiFi");
  if (isAllowed()) {
    JsonDocument doc;
    DeserializationError const error = deserializeJson(doc, value);

    if (error) {
      logger.critical(("JSON parsing failed: " + String(error.c_str())).c_str());
      return;
    }

    String const ssid = doc["ssid"];
    String const password = doc["password"];

    config.set(ConfigKey::WIFI_SSID, ssid);
    config.set(ConfigKey::WIFI_PASSWORD, password);
    config.saveConfig();

    WiFi.disconnect();
    WiFi.persistent(false);
    WiFi.begin(ssid, password);

    logger.info("WiFi SSID updated");
  }
}

void CallbackHandler::handleBatteryConfig(String const& value) {
  logger.info("Received battery configuration");
  if (!isAllowed()) {
    logger.warning("Setup no longer allowed - ignoring battery configuration");
    return;
  }

  JsonDocument doc;
  DeserializationError const error = deserializeJson(doc, value);

  if (error) {
    logger.critical(("Battery JSON parsing failed: " + String(error.c_str())).c_str());
    return;
  }

  Shunt& shunt = Shunt::getInstance();

  if (doc["capacityAmpHours"].is<uint16_t>()) {
    if (uint16_t const maxCapacity = doc["capacityAmpHours"]; maxCapacity > 0 && maxCapacity <= 10000) {
      logger.info(("Setting max capacity: " + String(maxCapacity)).c_str());

      shunt.setMaxCapacity(maxCapacity);
    } else {
      logger.warning("Invalid max capacity value (must be uint32_t)");
    }
  }

  if (doc["socPercentage"].is<uint8_t>()) {
    if (uint8_t const socPercent = doc["socPercentage"]; socPercent > 0 && socPercent <= 100) {
      logger.info(("Setting SOC percent: " + String(socPercent)).c_str());

      shunt.setCurrentStateOfCharge(socPercent);
    } else {
      logger.warning("Invalid SOC percent value (must be 0-100)");
    }
  }

  if (doc["chargeEfficiency"].is<float>()) {
    if (uint16_t const efficiency = doc["chargeEfficiency"]; efficiency > 0 && efficiency <= 100) {
      logger.info(("Setting charge efficiency: " + String(efficiency)).c_str());

      shunt.setChargeEfficiency(efficiency);
    } else {
      logger.warning("Invalid charge efficiency value (must be 0-100)");
    }
  }

  if (doc["fullChargeVoltage"].is<float>()) {
    if (float const voltage = doc["fullChargeVoltage"]; voltage > 0 && voltage <= 100) {
      logger.info(("Setting full charge voltage: " + String(voltage)).c_str());

      shunt.setFullChargeVoltage(voltage);
    } else {
      logger.warning("Invalid full charge voltage value (must be 0-100)");
    }
  }

  if (doc["fullChargeCurrent"].is<float>()) {
    if (float const current = doc["fullChargeCurrent"]; current > 0 && current <= 100) {
      logger.info(("Setting full charge current: " + String(current)).c_str());

      shunt.setFullChargeCurrent(current);
    } else {
      logger.warning("Invalid full charge current value (must be 0-100)");
    }
  }

  if (doc["fullChargeDuration"].is<uint32_t>()) {
    if (uint32_t const duration = doc["fullChargeDuration"]; duration > 0 && duration <= 60) {
      logger.info(("Setting full charge duration: " + String(duration)).c_str());

      shunt.setFullChargeDuration(duration);
    } else {
      logger.warning("Invalid full charge duration value (must be 1-60 minutes)");
    }
  }

  logger.info("Battery configuration updated");
}

void CallbackHandler::handleShuntConfig(String const& value) {
  logger.info("Received shunt configuration");
  if (!isAllowed()) {
    logger.warning("Setup no longer allowed - ignoring shunt configuration");
    return;
  }

  JsonDocument doc;
  DeserializationError const error = deserializeJson(doc, value);

  if (error) {
    logger.critical(("Shunt JSON parsing failed: " + String(error.c_str())).c_str());
    return;
  }

  if (doc["ledEnabled"].is<bool>()) {
    bool const ledEnabled = doc["ledEnabled"];
    config.set(ConfigKey::LED_ENABLED, ledEnabled);
  }

  config.saveConfig();

  NeoPixel::getInstance().handle();

  logger.info("Shunt configuration updated");
}

void CallbackHandler::updateShuntStatus(BLECharacteristic* shuntStatusChar) const {
  JsonDocument doc;
  doc["voltage"] = shunt.getBusVoltage();
  doc["current"] = shunt.getBusCurrent();
  doc["soc"] = shunt.getStateOfCharge();
  doc["capacityAmpHours"] = shunt.getMaxCapacity();
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();
  doc["time"] = millis();
  doc["externalBattery"] = externalBattery.readVoltage();
  doc["ttgo"] = shunt.getTTGO();
  doc["power"] = shunt.getPower();

  String statusJson;
  serializeJson(doc, statusJson);

  shuntStatusChar->setValue(statusJson.c_str());
  shuntStatusChar->notify();
}

void CallbackHandler::updateBatteryConfigCharacteristic(BLECharacteristic* batteryConfigChar) const {
  JsonDocument doc;
  doc["maxCapacity"] = shunt.getMaxCapacity();
  doc["socPercentage"] = static_cast<uint8_t>(shunt.calculateStateOfCharge());
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();
  doc["capacityAmpHours"] = shunt.getMaxCapacity();
  doc["fullChargeVoltage"] = shunt.getFullChargeVoltage();
  doc["fullChargeCurrent"] = shunt.getFullChargeCurrent();
  doc["fullChargeDuration"] = shunt.getFullChargeDuration();
  doc["setupAllowed"] = setupAllowed;

  String jsonString;
  serializeJson(doc, jsonString);

  batteryConfigChar->setValue(jsonString.c_str());
}

void CallbackHandler::updateWifiConfigCharacteristic(BLECharacteristic* wifiConfigChar) const {
  JsonDocument doc;
  doc["ip"] = WiFi.localIP();
  doc["ssid"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();
  doc["setupAllowed"] = setupAllowed;

  String jsonString;
  serializeJson(doc, jsonString);

  wifiConfigChar->setValue(jsonString.c_str());
}

void CallbackHandler::updateMqttConfigCharacteristic(BLECharacteristic* mqttConfigChar) const {
  JsonDocument doc;
  doc["server"] = config.get<String>(ConfigKey::MQTT_SERVER);
  doc["port"] = config.get<uint16_t>(ConfigKey::MQTT_PORT);
  doc["user"] = config.get<String>(ConfigKey::MQTT_USER);
  doc["connected"] = MQTTManager::getInstance().isConnected();
  doc["setupAllowed"] = setupAllowed;

  String jsonString;
  serializeJson(doc, jsonString);

  mqttConfigChar->setValue(jsonString.c_str());
}

void CallbackHandler::updateShuntConfigCharacteristic(BLECharacteristic* shuntConfigChar) const {
  JsonDocument doc;
  doc["ledEnabled"] = config.get<bool>(ConfigKey::LED_ENABLED);
  doc["uptime"] = millis();
  doc["setupAllowed"] = setupAllowed;
  doc["softwareVersion"] = SOFTWARE_VERSION;
  doc["version"] = VERSION;

  String json_string;
  serializeJson(doc, json_string);

  shuntConfigChar->setValue(json_string.c_str());
}

void CallbackHandler::handleResetAndDefaultConfig(String const& value) {
  logger.info("Received request to reset shunt and apply default config");
  config.resetToDefaults();
  logger.info("Shunt reset and default config applied, now restarting...");
  delay(1000);
  ESP.restart();
}

void CallbackHandler::handleOTAUpdate(uint8_t* data, size_t length) {
  if (length < 4) {
    logger.warning("OTA packet too short");
    return;
  }

  UpdateManager& updateManager = UpdateManager::getInstance();

  uint8_t const cmdType = data[0];
  uint16_t const payloadSize = (data[2] << 8) | data[1];
  uint8_t const seqNum = data[3];
  uint8_t* payload = data + 4;

  if (length != payloadSize + 4) {
    logger.warning("OTA packet size mismatch");
    return;
  }

  switch (cmdType) {
    case 0x01:  // BEGIN
      if (payloadSize == 4) {
        if (uint32_t const expectedSize = (payload[3] << 24) | (payload[2] << 16) | (payload[1] << 8) | payload[0];
            expectedSize > 0) {
          logger.info(("Starting binary OTA update with size: " + String(expectedSize)).c_str());
          expectedSeqNum = 0;

          if (!updateManager.beginOTAUpdate(expectedSize)) {
            logger.critical("Failed to begin OTA update");
          }
        } else {
          logger.critical("Invalid OTA size received");
        }
      } else {
        logger.critical("Invalid BEGIN payload size");
      }
      break;

    case 0x02:  // DATA
      if (updateManager.isUpdateInProgress()) {
        if (seqNum != expectedSeqNum) {
          logger.warning(
              ("Sequence number mismatch: expected " + String(expectedSeqNum) + ", got " + String(seqNum)).c_str());
          return;
        }

        if (!updateManager.writeOTAData(payload, payloadSize)) {
          logger.critical("Failed to write OTA data");
        } else {
          expectedSeqNum++;
        }
      } else {
        logger.warning("Received OTA data but no update in progress");
      }
      break;

    case 0x03:  // END
      if (updateManager.isUpdateInProgress()) {
        logger.info("Ending binary OTA update");

        if (updateManager.endOTAUpdate()) {
          logger.info("OTA update completed successfully, restarting...");
          updateManager.switchToNewFirmware();
        } else {
          logger.critical("Failed to complete OTA update");
        }
      } else {
        logger.warning("Received OTA END but no update in progress");
      }
      break;

    case 0x04:  // ABORT
      logger.info("Aborting binary OTA update");
      updateManager.abortOTAUpdate();
      expectedSeqNum = 0;
      break;

    default:
      logger.warning(("Unknown binary OTA command: 0x" + String(cmdType, HEX)).c_str());
      break;
  }
}
