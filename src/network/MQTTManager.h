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

 private:
  NetworkClientSecure net = NetworkClientSecure();
  MQTTClient client;
  Logger logger = Logger(Serial);

  char* prefix;

  boolean connect();
};

#endif  // MQTTMANAGER_H