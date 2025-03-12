#include "BluetoothManager.h"
#include <Arduino.h>

BluetoothManager& BluetoothManager::getInstance() {
    static BluetoothManager instance;
    return instance;
}

void BluetoothManager::init(const char* serverName) {
    BLEDevice::init(serverName);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    Logger l(Serial);
    l.prependLog = [] {
        return "BLE";
    };
    logger = l;

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

BLEService* BluetoothManager::createService(const char* serviceUUID) {
    if (pServer) {
        BLEService* service = pServer->createService(serviceUUID);

        String message = "Service created with UUID: ";
        message += serviceUUID;
        logger.info(message.c_str());

        return service;
    }
    logger.info("Server not initialized");
    return nullptr;
}

BLECharacteristic* BluetoothManager::createReadCharacteristic(BLEService* service, const char* charUUID) {
    if (service) {
        BLECharacteristic* characteristic = service->createCharacteristic(
            charUUID,
            BLECharacteristic::PROPERTY_READ
        );

        String message = "Read characteristic created with UUID: ";
        message += charUUID;
        logger.info(message.c_str());

        return characteristic;
    }
    logger.info("Service is null");
    return nullptr;
}

BLECharacteristic* BluetoothManager::createNotifyCharacteristic(BLEService* service, const char* charUUID) {
    if (service) {
        BLECharacteristic* characteristic = service->createCharacteristic(
            charUUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
        );
        characteristic->addDescriptor(new BLE2902());

        String message = "Notify characteristic created with UUID: ";
        message += charUUID;
        logger.info(message.c_str());

        return characteristic;
    }
    logger.info("Service is null");
    return nullptr;
}

BLECharacteristic* BluetoothManager::createWriteCharacteristic(BLEService* service, const char* charUUID,
                                                         std::function<void(const std::string&)> callback) {
    if (service) {
        BLECharacteristic* characteristic = service->createCharacteristic(
            charUUID,
            BLECharacteristic::PROPERTY_WRITE
        );
        characteristic->setCallbacks(new CharacteristicCallbacks(callback));

        String message = "Write characteristic created with UUID: ";
        message += charUUID;
        logger.info(message.c_str());

        return characteristic;
    }
    logger.info("Service is null");
    return nullptr;
}

void BluetoothManager::updateCharacteristicValue(BLECharacteristic* characteristic, const char* value) {
    if (characteristic) {
        characteristic->setValue(value);
        // If the characteristic has notify property, notify connected clients
        if ((characteristic->getProperties() & BLECharacteristic::PROPERTY_NOTIFY) != 0) {
            characteristic->notify();
        }
    }
}

bool BluetoothManager::isConnected() const {
    return deviceConnected;
}

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
    const std::string value = pCharacteristic->getValue();
    if (callback) {
        callback(value);
    }
}