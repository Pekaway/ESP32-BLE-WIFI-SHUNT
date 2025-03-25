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

  BluetoothManager(BluetoothManager const&) = delete;
  BluetoothManager& operator=(BluetoothManager const&) = delete;

  void init(char const* serverName, char const* serviceUUID);
  void startAdvertising();
  void handle();
  [[nodiscard]] bool isConnected() const;

  BLECharacteristic* createReadCharacteristic(char const* charUUID);
  BLECharacteristic* createNotifyCharacteristic(char const* charUUID);
  BLECharacteristic* createWriteCharacteristic(char const* charUUID, std::function<void(String const&)> callback);

 private:
  BluetoothManager();
  ~BluetoothManager() = default;

  Logger logger = Logger(Serial);

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
    explicit CharacteristicCallbacks(std::function<void(String const&)> cb) : callback(std::move(cb)) {}
    void onWrite(BLECharacteristic* pCharacteristic) override;
  };
};

#endif  // BLUETOOTHMANAGER_H