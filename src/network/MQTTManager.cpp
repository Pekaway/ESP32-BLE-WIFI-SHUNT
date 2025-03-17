#include "MQTTManager.h"
#include <utils/ConfigManager.h>

MQTTManager::MQTTManager() : client(256) {}

void MQTTManager::begin() {
  client.begin(MQTT_BROKER, MQTT_PORT, net);
  client.onMessage([this](String const& topic, String const& payload) {
    logger.info(
        ("Received message on topic: " + topic + " - " + payload).c_str());
  });

  client.setWill("shunt/status", "offline", true, 1);

  ConfigManager& config = ConfigManager::getInstance();
  auto const username = config.get<String>(ConfigKey::MQTT_USER);
  auto const password = config.get<String>(ConfigKey::MQTT_PASSWORD);
  client.connect("shuntClient", username.c_str(), password.c_str());

  if (client.connected()) {
    client.publish("shunt/status", "online", true, 1);
    logger.info("Connected to MQTT broker");
  } else {
    logger.critical("Failed to connect to MQTT broker");
  }
}

void MQTTManager::handle() {
  client.loop();
  if (!client.connected()) {
    connect();
  }
}

void MQTTManager::publishShuntValues() {
  Shunt& shunt = Shunt::getInstance();
  auto const voltage = String(shunt.getBusVoltage());
  auto const current = String(shunt.getBusCurrent());
  auto const power = String(shunt.getPower());
  auto const soc = String(shunt.getStateOfCharge());

  client.publish("shunt/voltage", voltage.c_str());
  client.publish("shunt/current", current.c_str());
  client.publish("shunt/power", power.c_str());
  client.publish("shunt/soc", soc.c_str());

  logger.info("Published shunt values to MQTT");
}

void MQTTManager::connect() {
  while (!client.connected()) {
    logger.info("Connecting to MQTT broker...");
    if (client.connect("shuntClient")) {
      client.publish("shunt/status", "online", true, 1);
      logger.info("Connected to MQTT broker");
    } else {
      logger.critical(
          "Failed to connect to MQTT broker, retrying in 5 seconds...");
      delay(5000);
    }
  }
}