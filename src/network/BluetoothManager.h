#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <functional>
#include <string>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Logger.h>

class BluetoothManager {
 public:
  static BluetoothManager& getInstance();

  void init(char const* serverName);

  void startAdvertising();

  BLEService* createService(char const* serviceUUID);

  BLECharacteristic* createReadCharacteristic(BLEService* service,
                                              char const* charUUID);

  // Added for notify capabilities
  BLECharacteristic* createNotifyCharacteristic(BLEService* service,
                                                char const* charUUID);

  BLECharacteristic* createWriteCharacteristic(
      BLEService* service, char const* charUUID,
      std::function<void(String const&)> callback);

  void updateCharacteristicValue(BLECharacteristic* characteristic,
                                 char const* value);

  bool isConnected() const;

 private:
  Logger logger = Logger(Serial);

  BluetoothManager() = default;
  ~BluetoothManager() = default;
  BluetoothManager(BluetoothManager const&) = delete;
  BluetoothManager& operator=(BluetoothManager const&) = delete;

  BLEServer* pServer = nullptr;
  bool deviceConnected = false;

  class ServerCallbacks final : public BLEServerCallbacks {
    BluetoothManager* manager;

   public:
    ServerCallbacks(BluetoothManager* mgr) : manager(mgr) {}
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
  };

  class CharacteristicCallbacks final : public BLECharacteristicCallbacks {
    std::function<void(String const&)> callback;

   public:
    CharacteristicCallbacks(std::function<void(String const&)> cb)
        : callback(cb) {}
    void onWrite(BLECharacteristic* pCharacteristic) override;
  };
};

#endif  // BLUETOOTHMANAGER_H