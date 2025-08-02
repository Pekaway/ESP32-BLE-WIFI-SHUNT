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

void BluetoothManager::init(char const* serverName) {
  BLEDevice::init(serverName);
  BLEDevice::setMTU(512);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(this));
  BLEAdvertising* pAdvertising = pServer->getAdvertising();

  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x12);

  auto advData = BLEAdvertisementData();
  advData.setName(serverName);
  advData.setManufacturerData(DEVICE_TYPE);
  pAdvertising->setAdvertisementData(advData);

  service = pServer->createService(BLEUUID(SERVICE_UUID), 30, 0);

  String message = "Server initialized as: ";
  message += serverName;
  logger.info(message.c_str());
}

void BluetoothManager::handle() {
  if (pServer) {
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

BLECharacteristic* BluetoothManager::createBinaryWriteCharacteristic(char const* charUUID,
                                                                     std::function<void(uint8_t*, size_t)> callback,
                                                                     uint8_t const properties) {
  if (service) {
    BLECharacteristic* characteristic = service->createCharacteristic(charUUID, properties);
    characteristic->setCallbacks(new BinaryCharacteristicCallbacks(std::move(callback)));

    String message = "Binary write characteristic created with UUID: ";
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

void BluetoothManager::BinaryCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  uint8_t* value = pCharacteristic->getData();
  size_t const length = pCharacteristic->getLength();
  if (callback && value != nullptr) {
    callback(value, length);
  }
}