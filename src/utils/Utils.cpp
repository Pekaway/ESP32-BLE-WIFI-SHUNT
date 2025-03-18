#include "Utils.h"
#include <Arduino.h>

char const* concatenate(char const* str1, char const* str2) {
  String const result = String(str1) + String(str2);
  return result.c_str();
}