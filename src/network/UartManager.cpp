#include "UartManager.h"
#include <constants.h>

UartManager& UartManager::getInstance() {
  static UartManager instance;
  return instance;
}

UartManager::UartManager() {
  logger.prependLog = [] { return "UART"; };
}

void UartManager::begin() {
  // Serial1.begin(UART_BAUD_RATE);
  logger.info(("UART initialized at " + String(UART_BAUD_RATE) + " baud").c_str());
}

template <typename T>
String UartManager::formatMessage(String const& label, T const& value) {
  return "\r\n" + label + "\t" + String(value);
}

void UartManager::writeShuntValues() {
  if (!Serial1.available()) {
    logger.critical("UART not available!");
    return;
  }

  auto const voltage = shunt.getBusVoltage();
  auto const current = shunt.getBusCurrent();
  auto const power = shunt.getPower();
  auto const soc = shunt.getStateOfCharge();

  String message = formatMessage("V", voltage) + formatMessage("I", current) + formatMessage("P", power) +
                   formatMessage("SOC", soc) + formatMessage("PID", UART_PID);

  int checksum = 0;
  for (int i = 0; i < message.length(); i++) {
    checksum += message.charAt(i);
  }
  checksum = checksum % 256;
  message += formatMessage("Checksum", checksum);

  // Serial1.print(message);
  logger.info(("Shunt values sent to UART: " + message).c_str());
}
