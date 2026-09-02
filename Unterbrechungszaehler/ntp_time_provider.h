#pragma once

#include <Arduino.h>

#include "time_types.h"

namespace NtpTimeProvider {

enum class Error : uint8_t {
  None,
  NoNetwork,
  EmptyServer,
  Dns,
  Socket,
  Send,
  Timeout,
  InvalidResponse,
  ServerUnsynchronized,
  ImplausibleTime
};

struct Result {
  bool ok = false;
  TimeTypes::Sample sample;
  Error error = Error::None;
  uint32_t rttMs = 0;
  String resolvedIp;
};

void begin();
const String &server();
bool isValidServerName(const char *serverName);

// Start exactly one NTP transaction. DNS resolution is supplied by the
// Arduino-ESP32 core and is the only remaining synchronous step. The UDP
// response wait itself is cooperative and completed from update().
bool startProbe();
bool startProbe(const char *serverName);
void update();
bool busy();
bool resultReady();
bool takeResult(Result &result);

bool saveServer(const char *serverName);
const char *errorName(Error error);

}  // namespace NtpTimeProvider
