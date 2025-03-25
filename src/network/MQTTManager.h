#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <sensors/Shunt.h>
#include <Arduino.h>
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

 private:
  MQTTManager();
  ~MQTTManager() = default;

  String const HASS_BASE_TOPIC = "homeassistant/sensor/pekaway_shunt/";
  String const STATUS_TOPIC = "shunt/status";
  String const DEVICE_ID = "shuntClient";

  NetworkClient net = NetworkClient();
  MQTTClient client;
  Logger logger = Logger(Serial);

  boolean connect();
};

#endif  // MQTTMANAGER_H