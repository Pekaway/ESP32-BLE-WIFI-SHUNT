#ifndef UARTMANAGER_H
#define UARTMANAGER_H

#include <sensors/Shunt.h>
#include <Arduino.h>
#include <Logger.h>

class UartManager {
 public:
  UartManager(UartManager const&) = delete;
  UartManager& operator=(UartManager const&) = delete;

  static UartManager& getInstance();

  void begin();

  void writeShuntValues();

 private:
  UartManager();
  ~UartManager() = default;

  template <class T>
  static String formatMessage(String const& label, T const& value);

  Logger logger = Logger(Serial);
  Shunt& shunt = Shunt::getInstance();
  HardwareSerial Serial1 = HardwareSerial(1);
};

#endif  // UARTMANAGER_H