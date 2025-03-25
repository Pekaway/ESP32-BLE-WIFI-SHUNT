#ifndef WIFIMANAGERPORTAL_H
#define WIFIMANAGERPORTAL_H

#include <sensors/Shunt.h>
#include <Logger.h>
#include <WiFiManager.h>

class WiFiManagerPortal {
 public:
  WiFiManagerPortal(WiFiManagerPortal const&) = delete;
  WiFiManagerPortal& operator=(WiFiManagerPortal const&) = delete;

  static WiFiManagerPortal& getInstance();

  void begin();
  void handle();

 private:
  WiFiManagerPortal();
  ~WiFiManagerPortal() = default;

  Logger logger = Logger(Serial);

  WiFiManager wifiManager;
  WiFiManagerParameter* custom_max_amp_hours;
  WiFiManagerParameter* custom_soc_percent;
  WiFiManagerParameter* custom_mqtt_server;
  WiFiManagerParameter* custom_mqtt_port;
  WiFiManagerParameter* custom_mqtt_user;
  WiFiManagerParameter* custom_mqtt_password;
  WiFiManagerParameter* custom_charge_efficiency;

  void setupPortal();
  void saveConfigCallback();
  void saveParamsCallback();
};

#endif  // WIFIMANAGERPORTAL_H