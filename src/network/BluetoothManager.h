//
// Created by willipreuk on 08/03/25.
//

#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>
#include <Logger.h>
#include <string>

class BluetoothManager {
public:
    static BluetoothManager& getInstance();

    void init(const char* serverName);

    void startAdvertising();

    BLEService* createService(const char* serviceUUID);

    BLECharacteristic* createReadCharacteristic(BLEService* service, const char* charUUID);

    // Added for notify capabilities
    BLECharacteristic* createNotifyCharacteristic(BLEService* service, const char* charUUID);

    BLECharacteristic* createWriteCharacteristic(BLEService* service, const char* charUUID,
                                                 std::function<void(const std::string&)> callback);

    void updateCharacteristicValue(BLECharacteristic* characteristic, const char* value);

    bool isConnected() const;

private:
    Logger logger;

    BluetoothManager() = default;
    ~BluetoothManager() = default;
    BluetoothManager(const BluetoothManager&) = delete;
    BluetoothManager& operator=(const BluetoothManager&) = delete;

    BLEServer* pServer = nullptr;
    bool deviceConnected = false;

    class ServerCallbacks : public BLEServerCallbacks {
    private:
        BluetoothManager* manager;
    public:
        ServerCallbacks(BluetoothManager* mgr) : manager(mgr) {}
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };

    class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    private:
        std::function<void(const std::string&)> callback;
    public:
        CharacteristicCallbacks(std::function<void(const std::string&)> cb) : callback(cb) {}
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };
};

#endif //BLUETOOTHMANAGER_H