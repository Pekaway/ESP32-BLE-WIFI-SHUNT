#include "BluetoothManager.h"
#include <utility>
#include <Arduino.h>
#include <constants.h>

BluetoothManager::BluetoothManager() {
  logger.prependLog = [] { return "BLE"; };
}

BluetoothManager& BluetoothManager::getInstance() {
  static BluetoothManager instance;
  return instance;
}

void BluetoothManager::init(char const* serverName, char const* serviceUUID) {
  BLEDevice::init(serverName);

  auto advData = BLEAdvertisementData();
  advData.setName(serverName);
  advData.setManufacturerData(serverName);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(this));
  pServer->getAdvertising()->setAdvertisementData(advData);

  service = pServer->createService(BLEUUID(SERVICE_UUID), 30, 0);

  String message = "Server initialized as: ";
  message += serverName;
  logger.info(message.c_str());
}

void BluetoothManager::handle() {
  if (pServer) {
    if (auto const connectedCount = pServer->getConnectedCount(); connectedCount > 0) {
      String message = "Connected devices: ";
      message += connectedCount;
      logger.info(message.c_str());
    }
  } else {
    logger.critical("Server not initialized");
  }
}

void BluetoothManager::startAdvertising() {
  if (pServer) {
    pServer->getAdvertising()->start();
    service->start();

    logger.info("Advertising started.");
  } else {
    logger.critical("Server not initialized");
  }
}

BLECharacteristic* BluetoothManager::createReadCharacteristic(char const* charUUID) {
  if (service) {
    BLECharacteristic* characteristic = service->createCharacteristic(charUUID, BLECharacteristic::PROPERTY_READ);

    String message = "Read characteristic created with UUID: ";
    message += charUUID;
    logger.info(message.c_str());

    return characteristic;
  }
  logger.critical("Service is null");
  return nullptr;
}

BLECharacteristic* BluetoothManager::createNotifyCharacteristic(char const* charUUID) {
  if (service) {
    BLECharacteristic* characteristic =
        service->createCharacteristic(charUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    characteristic->addDescriptor(new BLE2902());

    String message = "Notify characteristic created with UUID: ";
    message += charUUID;
    logger.info(message.c_str());

    return characteristic;
  }
  logger.critical("Service is null");
  return nullptr;
}

BLECharacteristic* BluetoothManager::createWriteCharacteristic(char const* charUUID,
                                                               std::function<void(String const&)> callback,
                                                               uint8_t const properties) {
  if (service) {
    BLECharacteristic* characteristic = service->createCharacteristic(charUUID, properties);
    characteristic->setCallbacks(new CharacteristicCallbacks(std::move(callback)));

    String message = "Write characteristic created with UUID: ";
    message += charUUID;
    logger.info(message.c_str());

    return characteristic;
  }
  logger.critical("Service is null");
  return nullptr;
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

void BluetoothManager::CharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  String const value = pCharacteristic->getValue();
  if (callback) {
    callback(value);
  }
}