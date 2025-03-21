#ifndef WIFIMANAGERPORTAL_H
#define WIFIMANAGERPORTAL_H

#include <sensors/Shunt.h>
#include <Logger.h>
#include <WiFiManager.h>

class WiFiManagerPortal {
 public:
  WiFiManagerPortal();
  void begin();
  void handle();

 private:
  WiFiManager wifiManager;
  WiFiManagerParameter* custom_max_amp_hours;
  WiFiManagerParameter* custom_soc_percent;
  WiFiManagerParameter* custom_mqtt_user;
  WiFiManagerParameter* custom_mqtt_password;
  WiFiManagerParameter* custom_charge_efficiency;

  Logger logger = Logger(Serial);

  void setupPortal();
  void saveConfigCallback();
  void saveParamsCallback();
};

#endif  // WIFIMANAGERPORTAL_H