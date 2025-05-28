#ifndef CONSTANTS_H
#define CONSTANTS_H

// BLE UUIDs
auto constexpr SERVICE_UUID = "91bad492-b950-4226-aa2b-4ede9fa42f59";
auto constexpr SHUNT_STATUS_CHAR_UUID = "ff100f8c-1266-4309-b472-76bc25e4a62f";

// BLE Write UUIDs
auto constexpr BATTERY_CONFIG_CHAR_UUID = "63d58a25-c22b-4586-b297-f1e310b7b0bc";
auto constexpr MQTT_CONFIG_CHAR_UUID = "2f681ce5-f2b0-4034-b714-d87156af4b5a";
auto constexpr WIFI_CHAR_UUID = "785840ec-f07e-495c-a9e6-e268398ebdd6";
auto constexpr SHUNT_CONFIG_CHAR_UUID = "e030f283-f7f3-4df8-a0f5-f2af18d1cea6";

// Shunt constants
constexpr uint32_t SHUNT_MICRO_OHM = 375;
constexpr uint16_t SHUNT_MAXIMUM_AMPS = 500;

// Battery constants
constexpr uint32_t SHUNT_MAX_CAPACITY = 100;      //
constexpr uint32_t SHUNT_INITIAL_SOC = 100;       // %
constexpr auto AUTO_SAVE_INTERVAL = 30;           // s
constexpr uint8_t SHUNT_CHARGE_EFFICIENCY = 100;  // %

// MQTT constants
String const HASS_BASE_TOPIC = "homeassistant/sensor/pekaway_shunt/";

// Reset constants
constexpr uint16_t RESET_WINDOW_MS = 30 * 1000;
constexpr uint8_t RESET_THRESHOLD = 3;

// NeoPixel constants
constexpr uint8_t PIXEL_COUNT = 1;
constexpr uint8_t PIXEL_PIN = 4;

// Uart
constexpr uint8_t UART_RX_PIN = 27;
constexpr uint8_t UART_TX_PIN = 28;
constexpr uint32_t UART_BAUD_RATE = 19200;
constexpr uint8_t UART_PID = 255;

// Other constants
constexpr uint32_t SERIAL_SPEED = 115200;
auto const BLE_SERVER_NAME = "Pekaway Shunt";
auto constexpr SETUP_TIME = 5 * 60 * 1000;
auto constexpr SHUNT_UPDATE_INTERVAL = 3 * 1000;

#endif  // CONSTANTS_H
