#pragma once

#include <Arduino.h>

// Small shared serial logger for all firmware modules.
// Modules use the same output format instead of writing directly to Serial.
namespace SerialLog {

enum class Level : uint8_t {
  Info,
  Success,
  Warning,
  Error
};

void begin(uint32_t baudRate);
void write(Level level, const char *module, const char *message);
void info(const char *module, const char *message);
void success(const char *module, const char *message);
void warning(const char *module, const char *message);
void error(const char *module, const char *message);

void infof(const char *module, const char *format, ...);
void successf(const char *module, const char *format, ...);
void warningf(const char *module, const char *format, ...);
void errorf(const char *module, const char *format, ...);

}  // namespace SerialLog
