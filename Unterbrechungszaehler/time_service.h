#pragma once

#include <Arduino.h>

#include "ntp_time_provider.h"
#include "time_types.h"

namespace TimeService {

enum class OperationKind : uint8_t {
  None,
  TimeCheck,
  NtpServerCheck
};

struct OperationState {
  bool checking = false;
  uint32_t id = 0;
  OperationKind kind = OperationKind::None;
  bool ok = true;
  const char *error = "none";
};

struct State {
  TimeTypes::Source source = TimeTypes::Source::Relative;
  TimeTypes::Quality quality = TimeTypes::Quality::Relative;
  bool absoluteValid = false;
  uint64_t lastCheckMonotonicMs = 0;
  bool lastCheckManual = false;
  OperationState operation;

  TimeTypes::Sample ntp;
  NtpTimeProvider::Error ntpError = NtpTimeProvider::Error::None;
  uint32_t ntpRttMs = 0;
  String ntpResolvedIp;

  TimeTypes::Sample rtc;
  TimeTypes::Sample rtcBeforeSync;
  bool rtcBeforeSyncAvailable = false;
  bool rtcReachable = false;
  bool rtcCalendarPlausible = false;
  bool rtcTimeValid = false;
  bool rtcOsf = false;
  bool rtcSyncAttempted = false;
  bool rtcSyncOk = false;

  TimeTypes::Sample browser;
  bool browserFallbackAttempted = false;
  int16_t browserTimezoneOffsetMinutes = 0;

  TimeTypes::Sample system;

  bool ntpDeltaAvailable = false;
  int64_t ntpDeltaMs = 0;
  bool rtcDeltaAvailable = false;
  int64_t rtcDeltaMs = 0;
  bool rtcBeforeSyncDeltaAvailable = false;
  int64_t rtcBeforeSyncDeltaMs = 0;
  bool browserDeltaAvailable = false;
  int64_t browserDeltaMs = 0;
  bool systemDeltaAvailable = false;
  int64_t systemDeltaMs = 0;
};

struct OperationResult {
  bool ok = false;
  const char *error = "unknown";
};

void begin();
// Runs deferred boot/manual NTP transactions cooperatively. The only remaining
// synchronous network step is the Arduino-ESP32 DNS resolver used by the NTP
// provider; the UDP response wait never blocks loop().
void update();
OperationResult check();
OperationResult checkAndSaveNtpServer(const char *serverName);
OperationResult acceptBrowserTime(int64_t epochMs, int16_t timezoneOffsetMinutes);

TimeTypes::Snapshot now();
const State &state();
const String &ntpServer();
// True while the deferred boot check or an explicit time operation is active.
// This lets the UI perform a bounded follow-up without introducing permanent polling.
bool checking();
bool browserFallbackAllowed();
const char *operationKindName(OperationKind kind);

// This is the single timestamp entry point intended for future event logging.
// The returned snapshot atomically carries time, source and monotonic order.
TimeTypes::Snapshot eventTimestamp();

}  // namespace TimeService
