#include "CallbackHandler.h"
#include "constants.h"
#include "network/MQTTManager.h"
#include <network/BluetoothManager.h>
#include <network/WiFiManagerPortal.h>
#include <sensors/Shunt.h>
#include <utils/ConfigManager.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Logger.h>

Logger logger(Serial);

ConfigManager& config = ConfigManager::getInstance();
BluetoothManager& btManager = BluetoothManager::getInstance();
MQTTManager& mqttManager = MQTTManager::getInstance();
WiFiManagerPortal& wifiPortal = WiFiManagerPortal::getInstance();
CallbackHandler& callbackHandler = CallbackHandler::getInstance();
Shunt& shunt = Shunt::getInstance();
Adafruit_NeoPixel pixels(1, 4, NEO_GRB + NEO_KHZ800);

BLECharacteristic* shuntStatusChar;
BLECharacteristic* batteryConfigChar;
BLECharacteristic* wifiConfigChar;
BLECharacteristic* mqttConfigChar;

unsigned long startUpTime = millis();

void setup() {
  Serial.begin(SERIAL_SPEED);
  logger.info("Starting setup...");

  config.init();

  callbackHandler.init();

  if (Shunt& shunt = Shunt::getInstance(); !shunt.init(MAXIMUM_AMPS)) {
    logger.critical("Failed to initialize Shunt");
    return;
  }

  btManager.init(BLE_SERVER_NAME, SERVICE_UUID);

  shuntStatusChar = btManager.createNotifyCharacteristic(SHUNT_STATUS_CHAR_UUID);

  batteryConfigChar = btManager.createWriteCharacteristic(
      BATTERY_CONFIG_CHAR_UUID, [](String const& value) { callbackHandler.handleBatteryConfig(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  mqttConfigChar = btManager.createWriteCharacteristic(
      MQTT_CONFIG_CHAR_UUID, [](String const& value) { callbackHandler.handleMQTTConfig(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  wifiConfigChar = btManager.createWriteCharacteristic(
      WIFI_CHAR_UUID, [](String const& value) { callbackHandler.handleWiFi(value); },
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  btManager.startAdvertising();

  wifiPortal.begin();
  mqttManager.begin();

  pixels.setPixelColor(0, Adafruit_NeoPixel::Color(100, 100, 100));
  pixels.show();

  logger.info("Setup complete");
}

void updateShuntStatus() {
  JsonDocument doc;
  doc["voltage"] = shunt.getBusVoltage();
  doc["current"] = shunt.getBusCurrent();
  doc["soc"] = shunt.getStateOfCharge();
  doc["capacity"] = shunt.getMaxCapacity();
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();
  doc["time"] = millis();

  String statusJson;
  serializeJson(doc, statusJson);
  shuntStatusChar->setValue(statusJson.c_str());
  shuntStatusChar->notify();
}

void updateBatteryConfigCharacteristic() {
  JsonDocument doc;
  doc["maxCapacity"] = shunt.getMaxCapacity();
  doc["socPercentage"] = static_cast<uint8_t>(shunt.calculateStateOfCharge());
  doc["chargeEfficiency"] = shunt.getChargeEfficiency();
  doc["maxAmps"] = config.get<float>(ConfigKey::MAXIMUM_AMPS, 0);
  doc["fullChargeVoltage"] = shunt.getFullChargeVoltage();
  doc["fullChargeCurrent"] = shunt.getFullChargeCurrent();
  doc["fullChargeDuration"] = shunt.getFullChargeDuration();

  String jsonString;
  serializeJson(doc, jsonString);

  batteryConfigChar->setValue(jsonString.c_str());
}

void updateWifiConfigCharacteristic() {
  JsonDocument doc;
  doc["ip"] = WiFiManagerPortal::getIp();
  doc["ssid"] = WiFiManagerPortal::getSSID();

  String jsonString;
  serializeJson(doc, jsonString);

  wifiConfigChar->setValue(jsonString.c_str());
}

void updateMqttConfigCharacteristic() {
  ConfigManager& config = ConfigManager::getInstance();

  JsonDocument doc;
  doc["server"] = config.get<String>(ConfigKey::MQTT_SERVER);
  doc["port"] = config.get<uint16_t>(ConfigKey::MQTT_PORT);
  doc["user"] = config.get<String>(ConfigKey::MQTT_USER);
  doc["password"] = config.get<String>(ConfigKey::MQTT_PASSWORD);

  String jsonString;
  serializeJson(doc, jsonString);

  mqttConfigChar->setValue(jsonString.c_str());
}

void updatePixel() {
  auto const currentMicroAmps = shunt.getBusCurrent();

  uint8_t const brightness = map(abs(currentMicroAmps / 1000000.0), 0, 500, 0, 510);

  if (currentMicroAmps < 0) {
    pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, brightness, 0));
  } else if (currentMicroAmps > 0) {
    pixels.setPixelColor(0, Adafruit_NeoPixel::Color(brightness, 0, 0));
  } else {
    pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
  }

  pixels.show();
}

unsigned long lastShuntUpdateTime = 0;
constexpr unsigned long SHUNT_UPDATE_INTERVAL = 10000;
bool setupAllowed = true;

void loop() {
  unsigned long const currentTime = millis();

  if (callbackHandler.isSetupAllowed() && startUpTime + SETUP_TIME < currentTime) {
    callbackHandler.closeSetup();
    setupAllowed = false;
  }

  if (currentTime - lastShuntUpdateTime >= SHUNT_UPDATE_INTERVAL) {
    lastShuntUpdateTime = currentTime;

    updateBatteryConfigCharacteristic();
    updateShuntStatus();
    updateWifiConfigCharacteristic();
    updateMqttConfigCharacteristic();
    mqttManager.publishShuntValues();
  }

  shunt.update();

  // Handle network services on every iteration
  wifiPortal.handle();
  mqttManager.handle();
  btManager.handle();

  if (!setupAllowed) {
    updatePixel();
  }

  delay(1000);
}