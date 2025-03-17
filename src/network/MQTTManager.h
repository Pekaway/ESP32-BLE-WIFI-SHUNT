#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <sensors/Shunt.h>
#include <Logger.h>
#include <MQTT.h>
#include <WiFi.h>

class MQTTManager {
 public:
  MQTTManager();
  void begin();
  void handle();
  void publishShuntValues();

 private:
  WiFiClient net;
  MQTTClient client;
  Logger logger = Logger(Serial);

  void connect();
};

#endif  // MQTTMANAGER_H