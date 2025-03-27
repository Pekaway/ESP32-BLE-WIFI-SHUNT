#include "MQTTManager.h"
#include <utils/ConfigManager.h>
#include <WiFi.h>

MQTTManager::MQTTManager() : client(256) {
  logger.prependLog = [] { return "SHUNT"; };
}

MQTTManager& MQTTManager::getInstance() {
  static MQTTManager instance;
  return instance;
}

void MQTTManager::begin() {
  ConfigManager& config = ConfigManager::getInstance();

  auto const server = config.get<String>(ConfigKey::MQTT_SERVER);
  auto const port = config.get<uint16_t>(ConfigKey::MQTT_PORT);

  client.begin(server.c_str(), port, net);
  client.onMessage([this](String const& topic, String const& payload) {
    logger.info(("Received message on topic: " + topic + " - " + payload).c_str());
  });

  client.setWill("shunt/status", "offline", true, 1);

  connect();
}

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
  auto const chargeEfficiency = shunt.getChargeEfficiency();
  auto const maxCapacity = shunt.getMaxCapacity();

  client.publish((HASS_BASE_TOPIC + "voltage/state").c_str(), String(voltage).c_str(), true);
  client.publish((HASS_BASE_TOPIC + "current/state").c_str(), String(current).c_str(), true);
  client.publish((HASS_BASE_TOPIC + "power/state").c_str(), String(power).c_str(), true);
  client.publish((HASS_BASE_TOPIC + "soc/state").c_str(), String(soc).c_str(), true);
  client.publish((HASS_BASE_TOPIC + "charge_efficiency/state").c_str(), String(chargeEfficiency).c_str(), true);
  client.publish((HASS_BASE_TOPIC + "max_capacity/state").c_str(), String(maxCapacity).c_str(), true);

  logger.info("Shunt values published to MQTT");
}

void MQTTManager::registerHomeAssistantSensors() {
  struct SensorConfig {
    char const* name;
    char const* unit;
    char const* deviceClass;
    char const* icon;
  };

  SensorConfig const sensors[] = {{"Voltage", "V", "voltage", "mdi:lightning-bolt"},
                                  {"Current", "A", "current", "mdi:current-ac"},
                                  {"Power", "W", "power", "mdi:flash"},
                                  {"State of Charge", "%", "battery", "mdi:battery"},
                                  {"Charge Efficiency", "%", "", "mdi:battery-charging"},
                                  {"Maximum Capacity", "Ah", "", "mdi:battery-high"}};

  char const* sensorIds[] = {"voltage", "current", "power", "soc", "charge_efficiency", "max_capacity"};

  for (int i = 0; i < 6; i++) {
    JsonDocument doc;

    doc["name"] = String("Pekaway Shunt ") + sensors[i].name;
    doc["state_topic"] = HASS_BASE_TOPIC + sensorIds[i] + "/state";
    doc["unit_of_measurement"] = sensors[i].unit;

    if (strlen(sensors[i].deviceClass) > 0) {
      doc["device_class"] = sensors[i].deviceClass;
    }

    doc["icon"] = sensors[i].icon;
    doc["unique_id"] = String("pekaway_shunt_") + sensorIds[i];

    auto device = doc["device"].to<JsonObject>();
    auto identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add("pekaway_shunt");
    device["name"] = "Pekaway Battery Shunt";
    device["manufacturer"] = "Pekaway";
    device["model"] = "Battery Shunt";

    String configPayload;
    serializeJson(doc, configPayload);

    client.publish((HASS_BASE_TOPIC + sensorIds[i] + "/config").c_str(), configPayload.c_str());
  }

  logger.info("Home Assistant sensor configurations published");
}

boolean MQTTManager::connect() {
  if (!WiFi.isConnected()) {
    logger.warning("WiFi is not connected, cannot connect to MQTT broker");
    return false;
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

  if (client.connect("shuntClient", username.c_str(), password.c_str())) {
    client.publish("shunt/status", "online", true, 1);
    logger.info("Connected to MQTT broker");

    registerHomeAssistantSensors();

    return true;
  }

  logger.critical("Failed to connect to MQTT broker!");
  return false;
}