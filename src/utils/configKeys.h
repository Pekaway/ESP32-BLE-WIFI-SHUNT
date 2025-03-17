#ifndef CONFIGKEYS_H
#define CONFIGKEYS_H

#include <unordered_map>
#include <Arduino.h>

enum class ConfigKey {
  DEVICE_NAME,
  MAX_CAPACITY,
  INITIAL_SOC,
  SHUNT_MICRO_OHM,
  MAXIMUM_AMPS,
  AUTO_SAVE_INTERVAL,
  MQTT_USER,
  MQTT_PASSWORD,
  CURRENT_SOC,
  CURRENT_CAPACITY_MAMS,
};

class ConfigKeys {
 public:
  static char const* toString(ConfigKey key) {
    static std::unordered_map<ConfigKey, char const*> const keyToStringMap = {
        {ConfigKey::DEVICE_NAME, "device_name"},
        {ConfigKey::MAX_CAPACITY, "max_capacity"},
        {ConfigKey::INITIAL_SOC, "initial_soc"},
        {ConfigKey::SHUNT_MICRO_OHM, "shunt_micro_ohm"},
        {ConfigKey::MAXIMUM_AMPS, "maximum_amps"},
        {ConfigKey::AUTO_SAVE_INTERVAL, "auto_save_interval"},
        {ConfigKey::MQTT_USER, "mqtt_user"},
        {ConfigKey::MQTT_PASSWORD, "mqtt_password"},
        {ConfigKey::CURRENT_SOC, "current_soc"},
        {ConfigKey::CURRENT_CAPACITY_MAMS, "current_capacity_mams"},
    };

    auto const it = keyToStringMap.find(key);
    if (it != keyToStringMap.end()) {
      return it->second;
    }
    return nullptr;
  }
};

#endif  // CONFIGKEYS_H