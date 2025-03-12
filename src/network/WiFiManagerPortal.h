#ifndef WIFIMANAGERPORTAL_H
#define WIFIMANAGERPORTAL_H

#include <WiFiManager.h>
#include <sensors/Shunt.h>
#include <Logger.h>

class WiFiManagerPortal {
public:
  WiFiManagerPortal();
  void begin();
  void handle();

private:
  WiFiManager wifiManager;
  WiFiManagerParameter custom_max_amp_hours;
  WiFiManagerParameter custom_soc_percent;

  Logger logger = Logger(Serial);

  void setupPortal();
  void saveConfigCallback();
  void saveParamsCallback();
};

#endif // WIFIMANAGERPORTAL_H