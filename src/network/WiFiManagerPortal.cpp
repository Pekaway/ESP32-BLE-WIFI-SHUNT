#include "WiFiManagerPortal.h"
#include <utils/ConfigManager.h>

WiFiManagerPortal& WiFiManagerPortal::getInstance() {
  static WiFiManagerPortal instance;
  return instance;
}

WiFiManagerPortal::WiFiManagerPortal() {
  logger.prependLog = [] { return "WIFI"; };

  wifiManager.setSaveConfigCallback([this] { saveConfigCallback(); });
  wifiManager.setSaveParamsCallback([this] { saveParamsCallback(); });

  ConfigManager& config_manager = ConfigManager::getInstance();

  custom_max_amp_hours = new WiFiManagerParameter(
      "max_amp_hours", "Max Amp Hours", String(config_manager.get<int64_t>(ConfigKey::MAXIMUM_AMPS)).c_str(), 10);
  custom_soc_percent = new WiFiManagerParameter(
      "soc_percent", "Soc Percentage", String(config_manager.get<int64_t>(ConfigKey::INITIAL_SOC)).c_str(), 10);
  custom_charge_efficiency =
      new WiFiManagerParameter("charge_efficiency", "Charge Efficiency in %",
                               String(config_manager.get<uint8_t>(ConfigKey::CHARGE_EFFICIENCY)).c_str(), 10);
  custom_mqtt_server = new WiFiManagerParameter("mqtt_server", "MQTT server", "", 32);
  custom_mqtt_port = new WiFiManagerParameter("mqtt_port", "MQTT port", "", 5);
  custom_mqtt_user = new WiFiManagerParameter(
      "mqtt_user", "MQTT user", config_manager.get<String>(ConfigKey::MQTT_USER).c_str(), 32, "type='email'");
  custom_mqtt_password =
      new WiFiManagerParameter("mqtt_password", "MQTT password",
                               config_manager.get<String>(ConfigKey::MQTT_USER).c_str(), 32, "type='password'");
}

void WiFiManagerPortal::begin() {
  logger.info("Starting WiFiManager portal...");
  setupPortal();
}

void WiFiManagerPortal::reset() {
  logger.info("Resetting WiFiManager portal...");
  wifiManager.resetSettings();
  logger.info("WiFiManager portal reset");
}

void WiFiManagerPortal::handle() { wifiManager.process(); }

void WiFiManagerPortal::setupPortal() {
  wifiManager.addParameter(custom_max_amp_hours);
  wifiManager.addParameter(custom_soc_percent);
  wifiManager.addParameter(custom_mqtt_user);
  wifiManager.addParameter(custom_mqtt_password);
  wifiManager.addParameter(custom_charge_efficiency);
  wifiManager.addParameter(custom_mqtt_server);
  wifiManager.addParameter(custom_mqtt_port);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConnectTimeout(1);

  if (!wifiManager.autoConnect("Pekaway Shunt")) {
    logger.critical("Failed to connect to WiFi");
    return;
  }

  logger.info("Connected to WiFi");
}

void WiFiManagerPortal::saveConfigCallback() { logger.info("Configuration saved"); }

void WiFiManagerPortal::saveParamsCallback() {
  Shunt& shunt = Shunt::getInstance();

  long long const maxAmpHours = strtoll(custom_max_amp_hours->getValue(), nullptr, 10);
  long long const socPercent = strtoll(custom_soc_percent->getValue(), nullptr, 10);
  uint8_t const chargeEfficiency = strtol(custom_charge_efficiency->getValue(), nullptr, 10);

  shunt.setMaxCapacity(maxAmpHours);
  shunt.setCurrentStateOfCharge(socPercent);
  shunt.setChargeEfficiency(chargeEfficiency);
  logger.info("Shunt values updated from portal");

  ConfigManager& config = ConfigManager::getInstance();
  auto const mqttUser = custom_mqtt_user->getValue();
  auto const mqttPassword = custom_mqtt_password->getValue();
  auto const mqttServer = custom_mqtt_server->getValue();
  auto const mqttPort = strtol(custom_mqtt_port->getValue(), nullptr, 10);

  config.set(ConfigKey::MQTT_SERVER, mqttServer);
  config.set(ConfigKey::MQTT_USER, mqttUser);
  config.set(ConfigKey::MQTT_PASSWORD, mqttPassword);
  config.set(ConfigKey::MQTT_PORT, mqttPort);
  config.saveConfig();
  logger.info("MQTT credentials updated from portal");
}