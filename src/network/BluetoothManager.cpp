#include "BluetoothManager.h"
#include <Arduino.h>

BluetoothManager& BluetoothManager::getInstance() {
  static BluetoothManager instance;
  return instance;
}

void BluetoothManager::init(char const* serverName) {
  BLEDevice::init(serverName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(this));

  logger.prependLog = [] { return "BLE"; };

  String message = "Server initialized as: ";
  message += serverName;
  logger.info(message.c_str());
}

void BluetoothManager::startAdvertising() {
  if (pServer) {
    pServer->getAdvertising()->start();
    logger.info("Advertising started.");
  } else {
    logger.critical("Server not initialized");
  }
}

BLEService* BluetoothManager::createService(char const* serviceUUID) {
  if (pServer) {
    BLEService* service = pServer->createService(serviceUUID);

    String message = "Service created with UUID: ";
    message += serviceUUID;
    logger.info(message.c_str());

    return service;
  }
  logger.critical("Server not initialized");
  return nullptr;
}

BLECharacteristic* BluetoothManager::createReadCharacteristic(
    BLEService* service, char const* charUUID) {
  if (service) {
    BLECharacteristic* characteristic = service->createCharacteristic(
        charUUID, BLECharacteristic::PROPERTY_READ);

    String message = "Read characteristic created with UUID: ";
    message += charUUID;
    logger.info(message.c_str());

    return characteristic;
  }
  logger.critical("Service is null");
  return nullptr;
}

BLECharacteristic* BluetoothManager::createNotifyCharacteristic(
    BLEService* service, char const* charUUID) {
  if (service) {
    BLECharacteristic* characteristic = service->createCharacteristic(
        charUUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    characteristic->addDescriptor(new BLE2902());

    String message = "Notify characteristic created with UUID: ";
    message += charUUID;
    logger.info(message.c_str());

    return characteristic;
  }
  logger.critical("Service is null");
  return nullptr;
}

BLECharacteristic* BluetoothManager::createWriteCharacteristic(
    BLEService* service, char const* charUUID,
    std::function<void(String const&)> callback) {
  if (service) {
    BLECharacteristic* characteristic = service->createCharacteristic(
        charUUID, BLECharacteristic::PROPERTY_WRITE);
    characteristic->setCallbacks(new CharacteristicCallbacks(callback));

    String message = "Write characteristic created with UUID: ";
    message += charUUID;
    logger.info(message.c_str());

    return characteristic;
  }
  logger.critical("Service is null");
  return nullptr;
}

void BluetoothManager::updateCharacteristicValue(
    BLECharacteristic* characteristic, char const* value) {
  if (characteristic) {
    characteristic->setValue(value);
  } else {
    logger.critical("Characteristic is null");
  }
}

bool BluetoothManager::isConnected() const { return deviceConnected; }

void BluetoothManager::ServerCallbacks::onConnect(BLEServer* pServer) {
  manager->deviceConnected = true;
  manager->logger.info("Client connected");
}

void BluetoothManager::ServerCallbacks::onDisconnect(BLEServer* pServer) {
  manager->deviceConnected = false;
  manager->logger.info("Client disconnected");
  pServer->startAdvertising();
}

void BluetoothManager::CharacteristicCallbacks::onWrite(
    BLECharacteristic* pCharacteristic) {
  String const value = pCharacteristic->getValue();
  if (callback) {
    callback(value);
  }
}