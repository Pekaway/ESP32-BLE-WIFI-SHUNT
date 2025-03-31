#include "MQTTManager.h"
#include <utils/ConfigManager.h>
#include <WiFi.h>

MQTTManager::MQTTManager() : client(256) {
  logger.prependLog = [] { return "MQTT"; };
}

MQTTManager& MQTTManager::getInstance() {
  static MQTTManager instance;
  return instance;
}

void MQTTManager::begin() { connect(true); }

void MQTTManager::handle() {
  if (!client.connected()) {
    connect();
  } else {
    client.loop();
  }
}

void MQTTManager::publishShuntValues() {
  if (!client.connected()) {
    return;
  }

  Shunt& shunt = Shunt::getInstance();
  auto const voltage = shunt.getBusVoltage();
  auto const current = shunt.getBusCurrent();
  auto const power = shunt.getPower();
  auto const soc = shunt.getStateOfCharge();
  auto const maxCapacity = shunt.getMaxCapacity();

  client.publish((HASS_SENSOR_BASE_TOPIC + "voltage/state").c_str(), String(voltage).c_str());
  client.publish((HASS_SENSOR_BASE_TOPIC + "current/state").c_str(), String(current).c_str());
  client.publish((HASS_SENSOR_BASE_TOPIC + "power/state").c_str(), String(power).c_str());
  client.publish((HASS_SENSOR_BASE_TOPIC + "soc/state").c_str(), String(soc).c_str());
  client.publish((HASS_SENSOR_BASE_TOPIC + "max_capacity/state").c_str(), String(maxCapacity).c_str());

  client.publish((HASS_NUMER_BASE_TOPIC + CHARGE_EFFICIENCY_TOPIC + "/state").c_str(),
                 String(shunt.getChargeEfficiency()).c_str());
  client.publish((HASS_NUMER_BASE_TOPIC + FULL_CHARGE_VOLTAGE_TOPIC + "/state").c_str(),
                 String(shunt.getFullChargeVoltage()).c_str());
  client.publish((HASS_NUMER_BASE_TOPIC + FULL_CHARGE_CURRENT_TOPIC + "/state").c_str(),
                 String(shunt.getFullChargeCurrent()).c_str());
  client.publish((HASS_NUMER_BASE_TOPIC + FULL_CHARGE_DURATION_TOPIC + "/state").c_str(),
                 String(shunt.getFullChargeDuration()).c_str());

  logger.info("Shunt values published to MQTT");
}

void MQTTManager::registerHomeAssistantSensors() {
  publishSensorConfig(
      {.id = "voltage", .name = "Voltage", .unit = "V", .deviceClass = "voltage", .icon = "mdi:current-ac"});

  publishSensorConfig(
      {.id = "current", .name = "Current", .unit = "A", .deviceClass = "current", .icon = "mdi:lightning-bolt"});

  publishSensorConfig({.id = "power", .name = "Power", .unit = "W", .deviceClass = "power", .icon = "mdi:flash"});

  publishSensorConfig({.id = "soc", .name = "State of Charge", .unit = "%", .deviceClass = "", .icon = "mdi:battery"});

  publishSensorConfig(
      {.id = "max_capacity", .name = "Maximum Capacity", .unit = "Ah", .deviceClass = "", .icon = "mdi:battery-high"});

  publishNumberConfig({.id = "charge_efficiency",
                       .name = "Charge Efficiency",
                       .topic = CHARGE_EFFICIENCY_TOPIC.c_str(),
                       .unit = "%",
                       .min = 1,
                       .max = 100,
                       .step = 1,
                       .icon = "mdi:percent"});

  publishNumberConfig({.id = "full_charge_voltage",
                       .name = "Full Charge Voltage",
                       .topic = FULL_CHARGE_VOLTAGE_TOPIC.c_str(),
                       .unit = "V",
                       .deviceClass = "voltage",
                       .min = 0,
                       .max = 25,
                       .step = 0.1,
                       .icon = "mdi:current-ac"});

  publishNumberConfig({.id = "full_charge_current",
                       .name = "Full Charge Current",
                       .topic = FULL_CHARGE_CURRENT_TOPIC.c_str(),
                       .unit = "A",
                       .deviceClass = "current",
                       .min = 0,
                       .max = 25,
                       .step = 0.1,
                       .icon = "mdi:lightning-bolt"});

  publishNumberConfig({.id = "full_charge_duration",
                       .name = "Full Charge Duration",
                       .topic = FULL_CHARGE_DURATION_TOPIC.c_str(),
                       .unit = "min",
                       .deviceClass = "duration",
                       .min = 1,
                       .max = 60,
                       .step = 1,
                       .icon = "mdi:clock-time-eight"});

  logger.info("Home Assistant sensor configurations published");
}

boolean MQTTManager::connect(bool const forceReconnect) {
  if (!WiFi.isConnected()) {
    logger.warning("WiFi is not connected, cannot connect to MQTT broker");
    return false;
  }

  if (client.connected()) {
    logger.info("Already connected to MQTT broker");
    if (!forceReconnect) {
      return true;
    }
    logger.info("Reconnecting to MQTT broker");
    client.disconnect();
  }

  ConfigManager& config = ConfigManager::getInstance();
  auto const server = config.get<String>(ConfigKey::MQTT_SERVER);
  auto const port = config.get<uint16_t>(ConfigKey::MQTT_PORT);
  auto const username = config.get<String>(ConfigKey::MQTT_USER);
  auto const password = config.get<String>(ConfigKey::MQTT_PASSWORD);

  if (server.isEmpty()) {
    return false;
  }

  char logMessage[128];
  snprintf(logMessage, sizeof(logMessage), "Connecting to MQTT broker at %s:%d with username '%s' and password '%s'",
           server.c_str(), port, username.c_str(), password.c_str());
  logger.info(logMessage);

  client.begin(server.c_str(), port, net);

  client.onMessage([this](String const& topic, String const& payload) {
    logger.info(("Received message on topic: " + topic + " - " + payload).c_str());
    if (topic.indexOf("command") > 0) {
      handleCommandMessage(topic, payload);
    }
  });
  client.setWill("shunt/status", "offline", true, 1);

  if (client.connect("shuntClient", username.c_str(), password.c_str())) {
    client.publish("shunt/status", "online", true, 1);
    logger.info("Connected to MQTT broker");

    registerHomeAssistantSensors();

    client.subscribe((HASS_NUMER_BASE_TOPIC + CHARGE_EFFICIENCY_TOPIC + "/command").c_str());
    client.subscribe((HASS_NUMER_BASE_TOPIC + FULL_CHARGE_VOLTAGE_TOPIC + "/command").c_str());
    client.subscribe((HASS_NUMER_BASE_TOPIC + FULL_CHARGE_CURRENT_TOPIC + "/command").c_str());
    client.subscribe((HASS_NUMER_BASE_TOPIC + FULL_CHARGE_DURATION_TOPIC + "/command").c_str());

    return true;
  }

  logger.critical("Failed to connect to MQTT broker!");
  return false;
}

void MQTTManager::handleCommandMessage(String const& topic, String const& message) {
  Shunt& shunt = Shunt::getInstance();

  if (topic.endsWith(CHARGE_EFFICIENCY_TOPIC + "/command")) {
    if (uint8_t const value = message.toInt(); value > 0 && value <= 100) {
      shunt.setChargeEfficiency(value);
      logger.info(("MQTT set charge efficiency: " + String(value)).c_str());
    }
  } else if (topic.endsWith(FULL_CHARGE_VOLTAGE_TOPIC + "/command")) {
    if (float const value = message.toFloat(); value > 0 && value <= 100) {
      shunt.setFullChargeVoltage(value);
      logger.info(("MQTT set full charge voltage: " + String(value)).c_str());
    }
  } else if (topic.endsWith(FULL_CHARGE_CURRENT_TOPIC + "/command")) {
    if (float const value = message.toFloat(); value > 0 && value <= 100) {
      shunt.setFullChargeCurrent(value);
      logger.info(("MQTT set full charge current: " + String(value)).c_str());
    }
  } else if (topic.endsWith(FULL_CHARGE_DURATION_TOPIC + "/command")) {
    if (uint32_t const value = message.toInt(); value > 0 && value <= 60) {
      shunt.setFullChargeDuration(value);
      logger.info(("MQTT set full charge duration: " + String(value)).c_str());
    }
  }

  publishShuntValues();
}

void MQTTManager::addDeviceInfo(JsonDocument& json) {
  auto const device = json["device"].to<JsonObject>();
  auto const identifiers = device["identifiers"].to<JsonArray>();
  if (bool const added = identifiers.add(DEVICE_ID); !added) {
    logger.critical("Failed to add device id");
  }
  device["name"] = "Pekaway Battery Shunt";
  device["manufacturer"] = "Pekaway";
  device["model"] = "Battery Shunt";
}

void MQTTManager::publishNumberConfig(HomeAssistantNumberConfig const& config) {
  String const configTopic = HASS_NUMER_BASE_TOPIC + config.id + "/config";

  JsonDocument doc;
  doc["name"] = config.name;
  doc["platform"] = "number";
  doc["unique_id"] = "pekaway_shunt_" + String(config.id);
  doc["object_id"] = "pekaway_shunt_" + String(config.id);
  doc["state_topic"] = HASS_NUMER_BASE_TOPIC + config.topic + "/state";
  doc["command_topic"] = HASS_NUMER_BASE_TOPIC + config.topic + "/command";
  doc["unit_of_measurement"] = config.unit;
  doc["min"] = config.min;
  doc["max"] = config.max;
  doc["step"] = config.step;
  doc["icon"] = config.icon;
  if (config.deviceClass != nullptr) {
    doc["device_class"] = config.deviceClass;
  }

  addDeviceInfo(doc);

  String payload;
  serializeJson(doc, payload);

  client.publish(configTopic.c_str(), payload.c_str());
}

void MQTTManager::publishSensorConfig(HomeAssistantSensorConfig const& config) {
  String const configTopic = HASS_SENSOR_BASE_TOPIC + config.id + "/config";

  JsonDocument doc;
  doc["name"] = config.name;
  doc["state_topic"] = HASS_SENSOR_BASE_TOPIC + config.id + "/state";
  doc["unit_of_measurement"] = config.unit;

  if (config.deviceClass && strlen(config.deviceClass) > 0) {
    doc["device_class"] = config.deviceClass;
  }

  doc["icon"] = config.icon;
  doc["unique_id"] = String("pekaway_shunt_") + config.id;

  addDeviceInfo(doc);

  String payload;
  serializeJson(doc, payload);

  client.publish(configTopic.c_str(), payload.c_str());
}