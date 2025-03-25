#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <functional>
#include <string>
#include <utility>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Logger.h>

class BluetoothManager {
 public:
  static BluetoothManager& getInstance();

  void init(char const* serverName, char const* serviceUUID);

  void startAdvertising();

  void handle();

  BLECharacteristic* createReadCharacteristic(char const* charUUID);

  BLECharacteristic* createNotifyCharacteristic(char const* charUUID);

  BLECharacteristic* createWriteCharacteristic(
      char const* charUUID, std::function<void(String const&)> callback);

  [[nodiscard]] bool isConnected() const;

 private:
  Logger logger = Logger(Serial);

  BluetoothManager() = default;
  ~BluetoothManager() = default;
  BluetoothManager(BluetoothManager const&) = delete;
  BluetoothManager& operator=(BluetoothManager const&) = delete;

  BLEServer* pServer = nullptr;
  BLEService* service = nullptr;
  bool deviceConnected = false;

  class ServerCallbacks final : public BLEServerCallbacks {
    BluetoothManager* manager;

   public:
    explicit ServerCallbacks(BluetoothManager* mgr) : manager(mgr) {}
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
  };

  class CharacteristicCallbacks final : public BLECharacteristicCallbacks {
    std::function<void(String const&)> callback;

   public:
    explicit CharacteristicCallbacks(std::function<void(String const&)> cb)
        : callback(std::move(cb)) {}
    void onWrite(BLECharacteristic* pCharacteristic) override;
  };
};

#endif  // BLUETOOTHMANAGER_H