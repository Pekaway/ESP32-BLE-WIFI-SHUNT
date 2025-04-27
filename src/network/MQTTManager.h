#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <sensors/Shunt.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Logger.h>
#include <MQTT.h>
#include <NetworkClientSecure.h>

class MQTTManager {
 public:
  MQTTManager(MQTTManager const&) = delete;
  MQTTManager& operator=(MQTTManager const&) = delete;

  static MQTTManager& getInstance();

  void begin();
  void handle();
  void publishShuntValues();
  void registerHomeAssistantSensors();
  boolean connect(bool forceReconnect = false);
  bool isConnected() { return client.connected(); }

 private:
  MQTTManager();
  ~MQTTManager() = default;

  String const HASS_SENSOR_BASE_TOPIC = "homeassistant/sensor/pekaway_shunt/";
  String const HASS_NUMER_BASE_TOPIC = "homeassistant/number/pekaway_shunt/";
  String const STATUS_TOPIC = "shunt/status";
  String const DEVICE_ID = "shuntClient";
  String const CHARGE_EFFICIENCY_TOPIC = "charge_efficiency";
  String const FULL_CHARGE_VOLTAGE_TOPIC = "full_charge_voltage";
  String const FULL_CHARGE_CURRENT_TOPIC = "full_charge_current";
  String const FULL_CHARGE_DURATION_TOPIC = "full_charge_duration";

  NetworkClient net = NetworkClient();
  MQTTClient client;
  Logger logger = Logger(Serial);

  void handleCommandMessage(String const& topic, String const& message);
  void addDeviceInfo(JsonDocument& json);

  struct HomeAssistantNumberConfig {
    char const* id{};
    char const* name{};
    char const* topic{};
    char const* unit{};
    char const* deviceClass{};
    float min{};
    float max{};
    float step{};
    char const* icon = "mdi:battery-charging";
  };
  void publishNumberConfig(HomeAssistantNumberConfig const& config);

  struct HomeAssistantSensorConfig {
    char const* id{};
    char const* name{};
    char const* unit{};
    char const* deviceClass = "";
    char const* icon = "mdi:battery";
  };
  void publishSensorConfig(HomeAssistantSensorConfig const& config);
};

#endif  // MQTTMANAGER_H