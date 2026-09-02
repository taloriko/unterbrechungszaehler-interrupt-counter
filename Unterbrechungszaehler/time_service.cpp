#include "time_service.h"

#include <sys/time.h>

#include "config.h"
#include "rtc_ds3231.h"
#include "serial_log.h"
#include "status_registry.h"
#include "time_utils.h"
#include "wifi_module.h"

namespace TimeService {
namespace {

enum class PendingKind : uint8_t { None, BootCheck, ManualCheck, NtpServerCheck };

State currentState;
int64_t baseEpochMs = 0;
uint64_t baseMonotonicMs = 0;
bool bootCheckPending = false;
PendingKind pendingKind = PendingKind::None;
bool pendingNtpStarted = false;
String pendingCandidateServer;

int64_t elapsedSigned(uint64_t newer, uint64_t older) {
  return newer >= older ? static_cast<int64_t>(newer - older) : -static_cast<int64_t>(older - newer);
}

bool normalizeSample(const TimeTypes::Sample &sample, uint64_t targetMonotonicMs, int64_t &epochMs) {
  if (!sample.valid) return false;
  epochMs = sample.epochMs + elapsedSigned(targetMonotonicMs, sample.monotonicMs);
  return true;
}

void setSystemEpoch(int64_t epochMs, uint64_t sampleMonotonicMs) {
  const uint64_t nowMono = TimeUtils::monotonicMs();
  const int64_t currentEpochMs = epochMs + elapsedSigned(nowMono, sampleMonotonicMs);
  struct timeval tv {};
  tv.tv_sec = static_cast<time_t>(currentEpochMs / 1000LL);
  tv.tv_usec = static_cast<suseconds_t>((currentEpochMs % 1000LL) * 1000LL);
  settimeofday(&tv, nullptr);
  baseEpochMs = currentEpochMs;
  baseMonotonicMs = nowMono;
}

void activate(TimeTypes::Source source, TimeTypes::Quality quality, const TimeTypes::Sample &sample) {
  const TimeTypes::Source previous = currentState.source;
  setSystemEpoch(sample.epochMs, sample.monotonicMs);
  currentState.source = source;
  currentState.quality = quality;
  currentState.absoluteValid = true;
  if (previous != source) {
    SerialLog::infof("TIME", "Time source changed | from=%s | to=%s",
                     TimeTypes::sourceName(previous), TimeTypes::sourceName(source));
  }
}

void activateRelative() {
  const TimeTypes::Source previous = currentState.source;
  currentState.source = TimeTypes::Source::Relative;
  currentState.quality = TimeTypes::Quality::Relative;
  currentState.absoluteValid = false;
  baseEpochMs = 0;
  baseMonotonicMs = TimeUtils::monotonicMs();
  if (previous != TimeTypes::Source::Relative) {
    SerialLog::warningf("TIME", "Time source changed | from=%s | to=relative",
                        TimeTypes::sourceName(previous));
  }
}

String formatUtc(int64_t epochMs) {
  HardwareTypes::DateTimeValue value;
  if (!TimeUtils::epochUtcToDateTime(epochMs / 1000LL, value)) return String("invalid");
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03lldZ",
           value.year, value.month, value.day, value.hour, value.minute, value.second,
           static_cast<long long>(epochMs % 1000LL));
  return String(buffer);
}

void captureRtcSample(bool refreshHardware) {
  if (refreshHardware && RtcDs3231::enabled()) RtcDs3231::probe();

  currentState.rtcReachable = RtcDs3231::enabled() && RtcDs3231::detected();
  currentState.rtcOsf = RtcDs3231::enabled() && RtcDs3231::oscillatorStopFlag();
  currentState.rtc = TimeTypes::Sample{};
  currentState.rtcCalendarPlausible = false;
  currentState.rtcTimeValid = false;

  if (!currentState.rtcReachable) return;
  const HardwareTypes::DateTimeValue &value = RtcDs3231::dateTime();
  int64_t epochSeconds = 0;
  const bool plausible = TimeUtils::isPlausibleDateTime(value, AppConfig::TIME_MIN_VALID_YEAR, AppConfig::TIME_MAX_VALID_YEAR) &&
                         TimeUtils::dateTimeToEpochUtc(value, epochSeconds);
  currentState.rtcCalendarPlausible = plausible;
  currentState.rtcTimeValid = plausible && !currentState.rtcOsf;
  currentState.rtc.available = true;
  currentState.rtc.valid = currentState.rtcTimeValid;
  if (plausible) {
    currentState.rtc.epochMs = epochSeconds * 1000LL;
    const uint64_t rtcMono = RtcDs3231::lastCheckMonotonicMs();
    currentState.rtc.monotonicMs = rtcMono ? rtcMono : TimeUtils::monotonicMs();
  }
}

void captureSystemSample() {
  currentState.system = TimeTypes::Sample{};
  if (!currentState.absoluteValid) return;
  const TimeTypes::Snapshot snapshot = now();
  currentState.system.available = snapshot.valid;
  currentState.system.valid = snapshot.valid;
  currentState.system.epochMs = snapshot.epochMs;
  currentState.system.monotonicMs = snapshot.monotonicMs;
}

bool sampleDelta(const TimeTypes::Sample &sample, const TimeTypes::Sample &reference, int64_t &deltaMs) {
  if (!sample.available || sample.epochMs <= 0 || !reference.valid) return false;
  const int64_t normalized = sample.epochMs + elapsedSigned(reference.monotonicMs, sample.monotonicMs);
  deltaMs = normalized - reference.epochMs;
  return true;
}

void computeDeltas() {
  currentState.ntpDeltaAvailable = false;
  currentState.rtcDeltaAvailable = false;
  currentState.rtcBeforeSyncDeltaAvailable = false;
  currentState.browserDeltaAvailable = false;
  currentState.systemDeltaAvailable = false;

  TimeTypes::Sample reference;
  if (currentState.source == TimeTypes::Source::Ntp && currentState.ntp.valid) reference = currentState.ntp;
  else if (currentState.source == TimeTypes::Source::Rtc && currentState.rtc.valid) reference = currentState.rtc;
  else if (currentState.source == TimeTypes::Source::Browser && currentState.browser.valid) reference = currentState.browser;
  else if (currentState.system.valid) reference = currentState.system;
  if (!reference.valid) return;

  currentState.ntpDeltaAvailable = sampleDelta(currentState.ntp, reference, currentState.ntpDeltaMs);
  currentState.rtcDeltaAvailable = sampleDelta(currentState.rtc, reference, currentState.rtcDeltaMs);
  if (currentState.rtcBeforeSyncAvailable) {
    currentState.rtcBeforeSyncDeltaAvailable = sampleDelta(currentState.rtcBeforeSync, reference, currentState.rtcBeforeSyncDeltaMs);
  }
  currentState.browserDeltaAvailable = sampleDelta(currentState.browser, reference, currentState.browserDeltaMs);
  currentState.systemDeltaAvailable = sampleDelta(currentState.system, reference, currentState.systemDeltaMs);
}

void syncRtcFromNtp() {
  currentState.rtcSyncAttempted = false;
  currentState.rtcSyncOk = false;
  currentState.rtcBeforeSyncAvailable = false;
  if (!AppConfig::RTC_SYNC_FROM_NTP_ENABLED || !currentState.ntp.valid || !RtcDs3231::enabled() || !RtcDs3231::detected()) return;

  currentState.rtcBeforeSync = currentState.rtc;
  currentState.rtcBeforeSyncAvailable = currentState.rtc.available && currentState.rtc.epochMs > 0;
  currentState.rtcSyncAttempted = true;

  int64_t ntpNowMs = 0;
  const uint64_t nowMono = TimeUtils::monotonicMs();
  normalizeSample(currentState.ntp, nowMono, ntpNowMs);
  HardwareTypes::DateTimeValue target;
  if (!TimeUtils::epochUtcToDateTime(ntpNowMs / 1000LL, target)) {
    SerialLog::error("TIME", "RTC sync failed | NTP epoch could not be converted");
    return;
  }

  if (!RtcDs3231::setDateTime(target)) {
    SerialLog::error("TIME", "RTC synchronization from NTP failed");
    return;
  }

  captureRtcSample(false);
  currentState.rtcSyncOk = currentState.rtc.valid;
  if (currentState.rtcSyncOk) SerialLog::success("TIME", "RTC synchronized from NTP and verified");
  else SerialLog::error("TIME", "RTC write completed but verification is not valid");
}

void setTimeStatus() {
  StatusRegistry::State status = StatusRegistry::State::Warning;
  if (currentState.operation.checking) status = StatusRegistry::State::Checking;
  else if (currentState.source == TimeTypes::Source::Ntp || currentState.source == TimeTypes::Source::Rtc) status = StatusRegistry::State::Ok;
  else if (currentState.source == TimeTypes::Source::Browser) status = StatusRegistry::State::Warning;
  else if (currentState.source == TimeTypes::Source::Relative) status = StatusRegistry::State::Warning;
  StatusRegistry::setState("time", status);
}

void finishCheck() {
  currentState.lastCheckMonotonicMs = TimeUtils::monotonicMs();
  captureSystemSample();
  computeDeltas();
  setTimeStatus();

  const TimeTypes::Snapshot snapshot = now();
  if (snapshot.valid) {
    SerialLog::successf("TIME", "Active source=%s | quality=%s | UTC=%s",
                        TimeTypes::sourceName(snapshot.source), TimeTypes::qualityName(snapshot.quality),
                        formatUtc(snapshot.epochMs).c_str());
  } else {
    SerialLog::warning("TIME", "Active source=relative | absolute time unavailable");
  }
}

void evaluateSources() {
  currentState.rtcSyncAttempted = false;
  currentState.rtcSyncOk = false;
  currentState.rtcBeforeSyncAvailable = false;

  if (currentState.ntp.valid) {
    activate(TimeTypes::Source::Ntp, TimeTypes::Quality::Reference, currentState.ntp);
    syncRtcFromNtp();
  } else if (currentState.rtc.valid) {
    activate(TimeTypes::Source::Rtc, TimeTypes::Quality::Valid, currentState.rtc);
  } else if (currentState.browser.valid) {
    activate(TimeTypes::Source::Browser, TimeTypes::Quality::Fallback, currentState.browser);
  } else {
    activateRelative();
  }
  finishCheck();
}

void setNtpResult(const NtpTimeProvider::Result &ntp) {
  currentState.ntp = ntp.sample;
  currentState.ntpError = ntp.error;
  currentState.ntpRttMs = ntp.rttMs;
  currentState.ntpResolvedIp = ntp.resolvedIp;
  if (ntp.ok) {
    SerialLog::successf("TIME", "NTP valid | server=%s | IP=%s | RTT=%lu ms | UTC=%s",
                        NtpTimeProvider::server().c_str(), ntp.resolvedIp.c_str(),
                        static_cast<unsigned long>(ntp.rttMs), formatUtc(ntp.sample.epochMs).c_str());
  } else {
    SerialLog::warningf("TIME", "NTP unavailable | server=%s | reason=%s",
                        NtpTimeProvider::server().c_str(), NtpTimeProvider::errorName(ntp.error));
  }
}

void beginOperation(OperationKind kind) {
  ++currentState.operation.id;
  if (currentState.operation.id == 0) ++currentState.operation.id;
  currentState.operation.checking = true;
  currentState.operation.kind = kind;
  currentState.operation.ok = false;
  currentState.operation.error = "none";
  setTimeStatus();
}

void completeOperation(bool ok, const char *error = "none") {
  currentState.operation.checking = false;
  currentState.operation.ok = ok;
  currentState.operation.error = error ? error : "unknown";
  pendingKind = PendingKind::None;
  pendingNtpStarted = false;
  pendingCandidateServer = "";
  setTimeStatus();
}

void scheduleCheck(bool manual) {
  beginOperation(OperationKind::TimeCheck);
  pendingKind = manual ? PendingKind::ManualCheck : PendingKind::BootCheck;
  pendingNtpStarted = false;
  currentState.lastCheckManual = manual;
  SerialLog::infof("TIME", "%s time check scheduled | NTP server=%s",
                   manual ? "Manual" : "Boot", NtpTimeProvider::server().c_str());
}

void startRegularNtpCheck(bool manual) {
  captureRtcSample(manual);
  if (currentState.rtcReachable) {
    if (currentState.rtc.valid) SerialLog::infof("TIME", "RTC valid | UTC=%s | OSF=no", formatUtc(currentState.rtc.epochMs).c_str());
    else SerialLog::warningf("TIME", "RTC not trusted | OSF=%s | calendar-plausible=%s",
                             currentState.rtcOsf ? "yes" : "no", currentState.rtcCalendarPlausible ? "yes" : "no");
  } else {
    SerialLog::warning("TIME", "RTC unavailable");
  }
  pendingNtpStarted = NtpTimeProvider::startProbe();
  if (!pendingNtpStarted) {
    SerialLog::error("TIME", "NTP provider busy while starting time check");
    completeOperation(false, "busy");
  }
}

void startCandidateNtpCheck() {
  SerialLog::infof("TIME", "NTP candidate check started | server=%s", pendingCandidateServer.c_str());
  pendingNtpStarted = NtpTimeProvider::startProbe(pendingCandidateServer.c_str());
  if (!pendingNtpStarted) {
    SerialLog::error("TIME", "NTP provider busy while starting candidate check");
    completeOperation(false, "busy");
  }
}

void finishRegularCheck(const NtpTimeProvider::Result &ntp) {
  const bool manual = pendingKind == PendingKind::ManualCheck;
  setNtpResult(ntp);
  if (manual && !currentState.ntp.valid && !currentState.rtc.valid && !currentState.browser.valid) {
    currentState.browserFallbackAttempted = false;
  }
  // Mark the operation complete before setting the final status in finishCheck.
  currentState.operation.checking = false;
  currentState.operation.ok = true;
  currentState.operation.error = "none";
  pendingKind = PendingKind::None;
  pendingNtpStarted = false;
  evaluateSources();
}

void finishCandidateCheck(const NtpTimeProvider::Result &candidate) {
  if (!candidate.ok) {
    const char *error = NtpTimeProvider::errorName(candidate.error);
    SerialLog::warningf("TIME", "NTP candidate rejected | server=%s | reason=%s",
                        pendingCandidateServer.c_str(), error);
    completeOperation(false, error);
    return;
  }
  if (!NtpTimeProvider::saveServer(pendingCandidateServer.c_str())) {
    SerialLog::error("TIME", "NTP candidate valid but could not be persisted in NVS");
    completeOperation(false, "persist_failed");
    return;
  }

  SerialLog::successf("TIME", "NTP server validated and stored | server=%s", NtpTimeProvider::server().c_str());
  captureRtcSample(true);
  setNtpResult(candidate);
  currentState.lastCheckManual = true;
  currentState.operation.checking = false;
  currentState.operation.ok = true;
  currentState.operation.error = "none";
  pendingKind = PendingKind::None;
  pendingNtpStarted = false;
  pendingCandidateServer = "";
  evaluateSources();
}

}  // namespace

void begin() {
  StatusRegistry::registerProvider("time", "status.time", "clock", true);
  NtpTimeProvider::begin();
  currentState = State{};
  currentState.source = TimeTypes::Source::Relative;
  currentState.quality = TimeTypes::Quality::Relative;
  baseEpochMs = 0;
  baseMonotonicMs = TimeUtils::monotonicMs();
  bootCheckPending = true;
  pendingKind = PendingKind::None;
  pendingNtpStarted = false;
  pendingCandidateServer = "";

  // RTC was already probed by HardwareRegistry. Use it immediately as a
  // provisional source so startup gets a useful clock without waiting for Wi-Fi.
  captureRtcSample(false);
  if (currentState.rtc.valid) activate(TimeTypes::Source::Rtc, TimeTypes::Quality::Valid, currentState.rtc);
  else activateRelative();
  captureSystemSample();
  computeDeltas();
  StatusRegistry::setState("time", StatusRegistry::State::Checking);
  SerialLog::infof("TIME", "Boot time check deferred until Wi-Fi startup settles | provisional source=%s",
                   TimeTypes::sourceName(currentState.source));
}

void update() {
  if (bootCheckPending && !currentState.operation.checking &&
      (WifiModule::stationConnected() || WifiModule::startupSettled())) {
    bootCheckPending = false;
    scheduleCheck(false);
  }

  if (currentState.operation.checking && !pendingNtpStarted) {
    if (pendingKind == PendingKind::BootCheck || pendingKind == PendingKind::ManualCheck) {
      startRegularNtpCheck(pendingKind == PendingKind::ManualCheck);
    } else if (pendingKind == PendingKind::NtpServerCheck) {
      startCandidateNtpCheck();
    }
  }

  NtpTimeProvider::update();
  if (!currentState.operation.checking) return;

  NtpTimeProvider::Result result;
  if (!NtpTimeProvider::takeResult(result)) return;
  if (pendingKind == PendingKind::NtpServerCheck) finishCandidateCheck(result);
  else finishRegularCheck(result);
}

OperationResult check() {
  bootCheckPending = false;
  if (currentState.operation.checking) return OperationResult{false, "busy"};
  scheduleCheck(true);
  return OperationResult{true, "none"};
}

OperationResult checkAndSaveNtpServer(const char *serverName) {
  bootCheckPending = false;
  if (currentState.operation.checking) return OperationResult{false, "busy"};
  if (!NtpTimeProvider::isValidServerName(serverName)) return OperationResult{false, "invalid_server"};

  pendingCandidateServer = serverName;
  beginOperation(OperationKind::NtpServerCheck);
  pendingKind = PendingKind::NtpServerCheck;
  pendingNtpStarted = false;
  return OperationResult{true, "none"};
}

OperationResult acceptBrowserTime(int64_t epochMs, int16_t timezoneOffsetMinutes) {
  if (currentState.operation.checking || bootCheckPending) return OperationResult{false, "busy"};
  if (!AppConfig::BROWSER_TIME_FALLBACK_ENABLED) return OperationResult{false, "browser_disabled"};
  if (!browserFallbackAllowed()) return OperationResult{false, "higher_priority_source"};

  currentState.browserFallbackAttempted = true;
  if (!TimeUtils::isPlausibleEpochMs(epochMs, AppConfig::TIME_MIN_VALID_YEAR, AppConfig::TIME_MAX_VALID_YEAR)) {
    SerialLog::warning("TIME", "Browser fallback rejected | implausible timestamp");
    return OperationResult{false, "browser_time_invalid"};
  }

  currentState.browser.available = true;
  currentState.browser.valid = true;
  currentState.browser.epochMs = epochMs;
  currentState.browser.monotonicMs = TimeUtils::monotonicMs();
  currentState.browserTimezoneOffsetMinutes = timezoneOffsetMinutes;
  activate(TimeTypes::Source::Browser, TimeTypes::Quality::Fallback, currentState.browser);
  finishCheck();
  SerialLog::successf("TIME", "Browser fallback accepted | UTC=%s | timezone-offset=%d min",
                      formatUtc(currentState.browser.epochMs).c_str(), static_cast<int>(timezoneOffsetMinutes));
  return OperationResult{true, "none"};
}

TimeTypes::Snapshot now() {
  TimeTypes::Snapshot snapshot;
  snapshot.monotonicMs = TimeUtils::monotonicMs();
  snapshot.source = currentState.source;
  snapshot.quality = currentState.quality;
  snapshot.valid = currentState.absoluteValid;
  if (snapshot.valid) snapshot.epochMs = baseEpochMs + elapsedSigned(snapshot.monotonicMs, baseMonotonicMs);
  return snapshot;
}

const State &state() {
  return currentState;
}

const String &ntpServer() {
  return NtpTimeProvider::server();
}

bool checking() {
  return bootCheckPending || currentState.operation.checking;
}

bool browserFallbackAllowed() {
  if (bootCheckPending || currentState.operation.checking) return false;
  if (!AppConfig::BROWSER_TIME_FALLBACK_ENABLED || currentState.browserFallbackAttempted) return false;
  return !currentState.ntp.valid && !currentState.rtc.valid && !currentState.absoluteValid;
}

const char *operationKindName(OperationKind kind) {
  switch (kind) {
    case OperationKind::TimeCheck: return "time_check";
    case OperationKind::NtpServerCheck: return "ntp_server_check";
    case OperationKind::None:
    default: return "none";
  }
}

TimeTypes::Snapshot eventTimestamp() {
  TimeTypes::Snapshot snapshot = now();
  if (!snapshot.valid) {
    snapshot.source = TimeTypes::Source::Relative;
    snapshot.quality = TimeTypes::Quality::Relative;
  }
  return snapshot;
}

}  // namespace TimeService
