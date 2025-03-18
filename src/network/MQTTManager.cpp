#include "MQTTManager.h"
#include <utils/ConfigManager.h>
#include <utils/Utils.h>
#include <WiFi.h>

MQTTManager::MQTTManager() : client(256), prefix() {}

void MQTTManager::begin() {
  net.setInsecure();

  prefix = const_cast<char*>(
      ConfigManager::getInstance().get<String>(ConfigKey::MQTT_USER).c_str());

  client.begin(MQTT_BROKER, MQTT_PORT, net);
  client.onMessage([this](String const& topic, String const& payload) {
    logger.info(
        ("Received message on topic: " + topic + " - " + payload).c_str());
  });

  client.setWill(concatenate(prefix, "shunt/status"), "offline", true, 1);

  connect();

  if (client.connected()) {
    client.publish(concatenate(prefix, "shunt/status"), "online", true, 1);
    logger.info("Connected to MQTT broker");
  } else {
    logger.critical("Failed to connect to MQTT broker");
  }
}

void MQTTManager::handle() {
  if (!client.connected()) {
    connect();
  } else {
    client.loop();
  }
}

void MQTTManager::publishShuntValues() {
  Shunt& shunt = Shunt::getInstance();
  auto const voltage = String(shunt.getBusVoltage());
  auto const current = String(shunt.getBusCurrent());
  auto const power = String(shunt.getPower());
  auto const soc = String(shunt.getStateOfCharge());

  client.publish(concatenate(prefix, "shunt/voltage"), voltage.c_str());
  client.publish(concatenate(prefix, "shunt/current"), current.c_str());
  client.publish(concatenate(prefix, "shunt/power"), power.c_str());
  client.publish(concatenate(prefix, "shunt/soc"), soc.c_str());

  logger.info("Published shunt values to MQTT");
}

boolean MQTTManager::connect() {
  if (!WiFi.isConnected()) {
    logger.warning("WiFi is not connected, cannot connect to MQTT broker");
    return false;
  }

  logger.info("Connecting to MQTT broker...");

  ConfigManager& config = ConfigManager::getInstance();
  auto const username = config.get<String>(ConfigKey::MQTT_USER);
  auto const password = config.get<String>(ConfigKey::MQTT_PASSWORD);
  prefix = const_cast<char*>(username.c_str());

  auto const connected =
      client.connect("shuntClient", username.c_str(), password.c_str());

  if (connected) {
    client.publish(concatenate(prefix, "shunt/status"), "online", true, 1);
    logger.info("Connected to MQTT broker");
    return true;
  }
  logger.critical("Failed to connect to MQTT broker!");
  return false;
}