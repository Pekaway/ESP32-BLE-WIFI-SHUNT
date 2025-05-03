#ifndef CONFIGKEYS_H
#define CONFIGKEYS_H

#include <unordered_map>
#include <Arduino.h>

enum class ConfigKey {
  DEVICE_NAME,
  MQTT_USER,
  MQTT_PASSWORD,
  MQTT_SERVER,
  MQTT_PORT,
  CURRENT_CAPACITY_MAMS,
  MAXIMUM_CAPACITY_MAMS,
  CHARGE_EFFICIENCY,
  FULL_CHARGE_VOLTAGE,
  FULL_CHARGE_CURRENT,
  FULL_CHARGE_DURATION,
  WIFI_SSID,
  WIFI_PASSWORD,
  LED_ENABLED,
};

class ConfigKeys {
 public:
  static char const* toString(ConfigKey const key) {
    static std::unordered_map<ConfigKey, char const*> const keyToStringMap = {
        {ConfigKey::DEVICE_NAME, "device_name"},
        {ConfigKey::MQTT_USER, "mqtt_user"},
        {ConfigKey::MQTT_PASSWORD, "mqtt_password"},
        {ConfigKey::MQTT_SERVER, "mqtt_server"},
        {ConfigKey::MQTT_PORT, "mqtt_port"},
        {ConfigKey::CURRENT_CAPACITY_MAMS, "current_capacity_mams"},
        {ConfigKey::CHARGE_EFFICIENCY, "charge_efficiency"},
        {ConfigKey::FULL_CHARGE_VOLTAGE, "full_charge_voltage"},
        {ConfigKey::FULL_CHARGE_CURRENT, "full_charge_current"},
        {ConfigKey::FULL_CHARGE_DURATION, "full_charge_duration"},
        {ConfigKey::WIFI_SSID, "wifi_ssid"},
        {ConfigKey::WIFI_PASSWORD, "wifi_password"},
        {ConfigKey::LED_ENABLED, "led_enabled"},
        {ConfigKey::MAXIMUM_CAPACITY_MAMS, "maximum_capacity_mams"}};

    if (auto const it = keyToStringMap.find(key); it != keyToStringMap.end()) {
      return it->second;
    }
    return nullptr;
  }
};

#endif  // CONFIGKEYS_H