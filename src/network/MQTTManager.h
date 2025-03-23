#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <sensors/Shunt.h>
#include <Arduino.h>
#include <Logger.h>
#include <MQTT.h>
#include <NetworkClientSecure.h>

class MQTTManager {
 public:
  MQTTManager();
  void begin();
  void handle();
  void publishShuntValues();
  void registerHomeAssistantSensors();

 private:
  NetworkClient net = NetworkClient();
  MQTTClient client;
  Logger logger = Logger(Serial);

  boolean connect();
};

#endif  // MQTTMANAGER_H