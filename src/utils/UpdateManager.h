#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <Arduino.h>
#include <Logger.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

class UpdateManager {
 public:
  static UpdateManager& getInstance();

  UpdateManager(UpdateManager const&) = delete;
  UpdateManager& operator=(UpdateManager const&) = delete;

  bool beginOTAUpdate(size_t expectedSize);
  bool writeOTAData(uint8_t* data, size_t len);
  bool endOTAUpdate();
  void abortOTAUpdate();

  bool validatePartition();
  bool switchToNewFirmware();

  size_t getBytesWritten() const { return bytesWritten; }
  size_t getTotalSize() const { return totalSize; }
  uint8_t getProgress() const;
  bool isUpdateInProgress() const { return updateInProgress; }

 private:
  UpdateManager();
  ~UpdateManager() = default;

  Logger logger = Logger(Serial);

  esp_partition_t const* otaPartition;
  esp_ota_handle_t otaHandle;

  bool updateInProgress;
  size_t bytesWritten;
  size_t totalSize;

  bool findNextOTAPartition();
};

#endif  // UPDATEMANAGER_H
