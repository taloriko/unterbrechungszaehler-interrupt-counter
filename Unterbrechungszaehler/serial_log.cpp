#include "serial_log.h"

#include <stdarg.h>
#include <stdio.h>

namespace SerialLog {
namespace {

constexpr size_t LOG_BUFFER_SIZE = 192;

const char *levelName(Level level) {
  switch (level) {
    case Level::Success: return "OK";
    case Level::Warning: return "WARN";
    case Level::Error: return "ERROR";
    case Level::Info:
    default: return "INFO";
  }
}

void writeFormatted(Level level, const char *module, const char *format, va_list args) {
  char buffer[LOG_BUFFER_SIZE];
  vsnprintf(buffer, sizeof(buffer), format, args);
  write(level, module, buffer);
}

}  // namespace

void begin(uint32_t baudRate) {
  Serial.begin(baudRate);
  delay(50);
  Serial.println();
}

void write(Level level, const char *module, const char *message) {
  const uint32_t now = millis();
  Serial.printf("[%10lu ms] [%-5s] [%-8s] %s\n",
                static_cast<unsigned long>(now),
                levelName(level),
                module ? module : "SYSTEM",
                message ? message : "");
}

void info(const char *module, const char *message) {
  write(Level::Info, module, message);
}

void success(const char *module, const char *message) {
  write(Level::Success, module, message);
}

void warning(const char *module, const char *message) {
  write(Level::Warning, module, message);
}

void error(const char *module, const char *message) {
  write(Level::Error, module, message);
}

void infof(const char *module, const char *format, ...) {
  va_list args;
  va_start(args, format);
  writeFormatted(Level::Info, module, format, args);
  va_end(args);
}

void successf(const char *module, const char *format, ...) {
  va_list args;
  va_start(args, format);
  writeFormatted(Level::Success, module, format, args);
  va_end(args);
}

void warningf(const char *module, const char *format, ...) {
  va_list args;
  va_start(args, format);
  writeFormatted(Level::Warning, module, format, args);
  va_end(args);
}

void errorf(const char *module, const char *format, ...) {
  va_list args;
  va_start(args, format);
  writeFormatted(Level::Error, module, format, args);
  va_end(args);
}

}  // namespace SerialLog
