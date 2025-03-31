#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

// BLE UUIDs
auto constexpr SERVICE_UUID = "91bad492-b950-4226-aa2b-4ede9fa42f59";
auto constexpr SHUNT_STATUS_CHAR_UUID = "ff100f8c-1266-4309-b472-76bc25e4a62f";

// BLE Write UUIDs
auto constexpr BATTERY_CONFIG_CHAR_UUID = "63d58a25-c22b-4586-b297-f1e310b7b0bc";
auto constexpr MQTT_CONFIG_CHAR_UUID = "2f681ce5-f2b0-4034-b714-d87156af4b5a";
auto constexpr WIFI_CHAR_UUID = "785840ec-f07e-495c-a9e6-e268398ebdd6";

// Shunt constants
constexpr uint32_t SHUNT_MICRO_OHM = 375;
constexpr uint16_t MAXIMUM_AMPS = 1022;

// MQTT constants
auto const MQTT_BROKER = "mqttapi-emqx-vanpi.pekaway.de";
constexpr auto MQTT_PORT = 8883;
String const HASS_BASE_TOPIC = "homeassistant/sensor/pekaway_shunt/";

// Other constants
constexpr uint32_t SERIAL_SPEED = 115200;
auto const BLE_SERVER_NAME = "Pekaway Shunt";

#endif  // CONSTANTS_H
