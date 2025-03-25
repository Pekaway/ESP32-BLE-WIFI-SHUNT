#ifndef CALLBACKHANDLER_H
#define CALLBACKHANDLER_H

#include <Arduino.h>
#include <Logger.h>

class CallbackHandler {
 public:
  CallbackHandler(CallbackHandler const&) = delete;
  CallbackHandler& operator=(CallbackHandler const&) = delete;

  static CallbackHandler& getInstance();

  void init();
  void closeSetup();
  [[nodiscard]] bool isSetupAllowed() const;

  void handleMaxAmpCallback(String const& value);
  void handleSOCPercent(String const& value);
  void handleChargeEfficiency(String const& value);

 private:
  CallbackHandler();
  ~CallbackHandler() = default;

  Logger logger = Logger(Serial);
  bool setupAllowed = true;

  bool isAllowed();
};

#endif  // CALLBACKHANDLER_H