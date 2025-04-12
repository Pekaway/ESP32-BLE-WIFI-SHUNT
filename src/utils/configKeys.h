#ifndef CONFIGKEYS_H
#define CONFIGKEYS_H

#include <unordered_map>
#include <Arduino.h>

enum class ConfigKey {
  DEVICE_NAME,
  INITIAL_SOC,
  SHUNT_MICRO_OHM,
  MAXIMUM_AMPS,
  AUTO_SAVE_INTERVAL,
  MQTT_USER,
  MQTT_PASSWORD,
  MQTT_SERVER,
  MQTT_PORT,
  CURRENT_SOC,
  CURRENT_CAPACITY_MAMS,
  CHARGE_EFFICIENCY,
  FULL_CHARGE_VOLTAGE,
  FULL_CHARGE_CURRENT,
  FULL_CHARGE_DURATION,
};

class ConfigKeys {
 public:
  static char const* toString(ConfigKey const key) {
    static std::unordered_map<ConfigKey, char const*> const keyToStringMap = {
        {ConfigKey::DEVICE_NAME, "device_name"},
        {ConfigKey::INITIAL_SOC, "initial_soc"},
        {ConfigKey::SHUNT_MICRO_OHM, "shunt_micro_ohm"},
        {ConfigKey::MAXIMUM_AMPS, "maximum_amps"},
        {ConfigKey::AUTO_SAVE_INTERVAL, "auto_save_interval"},
        {ConfigKey::MQTT_USER, "mqtt_user"},
        {ConfigKey::MQTT_PASSWORD, "mqtt_password"},
        {ConfigKey::MQTT_SERVER, "mqtt_server"},
        {ConfigKey::MQTT_PORT, "mqtt_port"},
        {ConfigKey::CURRENT_SOC, "current_soc"},
        {ConfigKey::CURRENT_CAPACITY_MAMS, "current_capacity_mams"},
        {ConfigKey::CHARGE_EFFICIENCY, "charge_efficiency"},
        {ConfigKey::FULL_CHARGE_VOLTAGE, "full_charge_voltage"},
        {ConfigKey::FULL_CHARGE_CURRENT, "full_charge_current"},
        {ConfigKey::FULL_CHARGE_DURATION, "full_charge_duration"},
    };

    if (auto const it = keyToStringMap.find(key); it != keyToStringMap.end()) {
      return it->second;
    }
    return nullptr;
  }
};

#endif  // CONFIGKEYS_H