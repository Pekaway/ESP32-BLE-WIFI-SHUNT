#include "UpdateManager.h"
#include <esp_bt.h>

UpdateManager& UpdateManager::getInstance() {
  static UpdateManager instance;
  return instance;
}

UpdateManager::UpdateManager()
    : otaPartition(nullptr), otaHandle(0), updateInProgress(false), bytesWritten(0), totalSize(0) {
  logger.prependLog = [] { return "OTA"; };
}

bool UpdateManager::findNextOTAPartition() {
  esp_partition_t const* currentPartition = esp_ota_get_running_partition();
  esp_partition_t const* nextPartition = esp_ota_get_next_update_partition(currentPartition);

  if (nextPartition == nullptr) {
    logger.critical("No OTA partition available for update");
    return false;
  }

  logger.info(("Current partition: " + String(currentPartition->label)).c_str());
  logger.info(("Next partition: " + String(nextPartition->label)).c_str());
  logger.info(("Next partition size: " + String(nextPartition->size)).c_str());

  otaPartition = nextPartition;
  return true;
}

bool UpdateManager::beginOTAUpdate(size_t expectedSize) {
  if (updateInProgress) {
    logger.warning("OTA update already in progress");
    return false;
  }

  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

  if (!findNextOTAPartition()) {
    return false;
  }

  if (expectedSize > otaPartition->size) {
    logger.critical(
        ("Firmware size (" + String(expectedSize) + ") exceeds partition size (" + String(otaPartition->size) + ")")
            .c_str());
    return false;
  }

  esp_err_t err = esp_ota_begin(otaPartition, expectedSize, &otaHandle);
  if (err != ESP_OK) {
    logger.critical(("Failed to begin OTA update: " + String(esp_err_to_name(err))).c_str());
    return false;
  }

  updateInProgress = true;
  bytesWritten = 0;
  totalSize = expectedSize;

  logger.info(("OTA update started, expected size: " + String(expectedSize)).c_str());
  return true;
}

bool UpdateManager::writeOTAData(uint8_t* data, size_t len) {
  if (!updateInProgress) {
    logger.critical("No OTA update in progress");
    return false;
  }

  if (bytesWritten + len > totalSize) {
    logger.critical(("Data exceeds expected size: " + String(bytesWritten + len) + " > " + String(totalSize)).c_str());
    abortOTAUpdate();
    return false;
  }

  esp_err_t err = esp_ota_write(otaHandle, data, len);
  if (err != ESP_OK) {
    logger.critical(("Failed to write OTA data: " + String(esp_err_to_name(err))).c_str());
    abortOTAUpdate();
    return false;
  }

  bytesWritten += len;

  if (bytesWritten % 256 == 0 || bytesWritten == totalSize) {
    logger.info(
        ("OTA progress: " + String(getProgress()) + "% (" + String(bytesWritten) + "/" + String(totalSize) + ")")
            .c_str());
  }

  return true;
}

bool UpdateManager::endOTAUpdate() {
  if (!updateInProgress) {
    logger.critical("No OTA update in progress");
    return false;
  }

  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N0);

  if (bytesWritten != totalSize) {
    logger.critical(("Incomplete update: " + String(bytesWritten) + "/" + String(totalSize)).c_str());
    abortOTAUpdate();
    return false;
  }

  esp_err_t err = esp_ota_end(otaHandle);
  if (err != ESP_OK) {
    logger.critical(("Failed to end OTA update: " + String(esp_err_to_name(err))).c_str());
    updateInProgress = false;
    return false;
  }

  updateInProgress = false;
  logger.info("OTA update completed successfully");

  return validatePartition();
}

void UpdateManager::abortOTAUpdate() {
  if (updateInProgress) {
    esp_ota_abort(otaHandle);
    updateInProgress = false;
    bytesWritten = 0;
    totalSize = 0;
    logger.warning("OTA update aborted");
  }
}

bool UpdateManager::validatePartition() {
  if (otaPartition == nullptr) {
    logger.critical("No OTA partition to validate");
    return false;
  }

  esp_err_t err = esp_ota_set_boot_partition(otaPartition);
  if (err != ESP_OK) {
    logger.critical(("Failed to set boot partition: " + String(esp_err_to_name(err))).c_str());
    return false;
  }

  logger.info("New firmware partition validated and set as boot partition");
  return true;
}

bool UpdateManager::switchToNewFirmware() {
  if (!validatePartition()) {
    return false;
  }

  logger.info("Restarting to switch to new firmware...");
  delay(1000);
  ESP.restart();
  return true;
}

uint8_t UpdateManager::getProgress() const {
  if (totalSize == 0) return 0;
  return (bytesWritten * 100) / totalSize;
}
