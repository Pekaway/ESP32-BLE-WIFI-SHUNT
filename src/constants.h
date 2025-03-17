#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

// BLE UUIDs
auto const SERVICE_UUID = "91bad492-b950-4226-aa2b-4ede9fa42f59";
auto const VOLTAGE_CHAR_UUID = "ff100f8c-1266-4309-b472-76bc25e4a62f";
auto const CURRENT_CHAR_UUID = "d2e8ede8-9b31-4478-9fd6-75845cb68b1d";
auto const SOC_CHAR_UUID = "459e9ea4-a335-4a3f-b838-46788fd6bbe4";
auto const MAX_AMP_HOURS_CHAR_UUID = "38f2bd70-d659-4970-86c1-061e24700a6e";
auto const SOC_PERCENT_CHAR_UUID = "63d58a25-c22b-4586-b297-f1e310b7b0bc";

// Shunt constants
constexpr uint32_t SHUNT_MICRO_OHM = 375;
constexpr uint16_t MAXIMUM_AMPS = 1022;

// MQTT constants
auto const MQTT_BROKER = "mqttapi-emqx-vanpi.pekaway.de";
constexpr auto MQTT_PORT = 8883;

// Other constants
constexpr uint32_t SERIAL_SPEED = 115200;
auto const BLE_SERVER_NAME = "Pekaway Shunt";

#endif  // CONSTANTS_H