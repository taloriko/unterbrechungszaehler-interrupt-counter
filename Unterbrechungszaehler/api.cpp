#include "api.h"

#include <esp_timer.h>
#include <time.h>

#include "config.h"
#include "json_utils.h"
#include "ota_module.h"
#include "project_config.h"
#include "wifi_module.h"
#include "hardware_registry.h"
#include "interruption_api.h"
#include "hardware_types.h"
#include "status_registry.h"
#include "time_service.h"

namespace Api {
namespace {

void appendStringField(String &out, const char *key, const char *value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendEscapedString(out, value);
  if (comma) out += ',';
}

void appendStringField(String &out, const char *key, const String &value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendEscapedString(out, value);
  if (comma) out += ',';
}

void appendBoolField(String &out, const char *key, bool value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendBool(out, value);
  if (comma) out += ',';
}

void appendUIntField(String &out, const char *key, uint32_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendUInt(out, value);
  if (comma) out += ',';
}

void appendUInt64Field(String &out, const char *key, uint64_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendUInt64(out, value);
  if (comma) out += ',';
}

void appendIntField(String &out, const char *key, int32_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendInt(out, value);
  if (comma) out += ',';
}

void appendInt64Field(String &out, const char *key, int64_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendInt64(out, value);
  if (comma) out += ',';
}

void appendSample(String &out, const TimeTypes::Sample &sample) {
  out += '{';
  appendBoolField(out, "available", sample.available);
  appendBoolField(out, "valid", sample.valid);
  if (sample.available && sample.epochMs > 0) {
    appendInt64Field(out, "epochMs", sample.epochMs);
    appendUInt64Field(out, "monotonicMs", sample.monotonicMs, false);
  } else if (out.endsWith(",")) {
    out.remove(out.length() - 1);
  }
  out += '}';
}

void appendTimeManagement(String &out) {
  const TimeService::State &ts = TimeService::state();
  const TimeTypes::Snapshot current = TimeService::now();
  out += '{';
  appendStringField(out, "activeSource", TimeTypes::sourceName(ts.source));
  appendStringField(out, "quality", TimeTypes::qualityName(ts.quality));
  appendBoolField(out, "valid", current.valid);
  appendBoolField(out, "checking", TimeService::checking());
  JsonUtils::appendKey(out, "operation");
  out += '{';
  appendUIntField(out, "id", ts.operation.id);
  appendStringField(out, "kind", TimeService::operationKindName(ts.operation.kind));
  appendBoolField(out, "checking", ts.operation.checking);
  appendBoolField(out, "ok", ts.operation.ok);
  appendStringField(out, "error", ts.operation.error, false);
  out += "},";
  if (current.valid) appendInt64Field(out, "epochMs", current.epochMs);
  appendUInt64Field(out, "monotonicMs", current.monotonicMs);
  appendUInt64Field(out, "lastCheckMonotonicMs", ts.lastCheckMonotonicMs);
  appendBoolField(out, "lastCheckManual", ts.lastCheckManual);
  appendStringField(out, "ntpServer", TimeService::ntpServer());
  appendBoolField(out, "browserFallbackAllowed", TimeService::browserFallbackAllowed());

  JsonUtils::appendKey(out, "ntp");
  out += '{';
  appendStringField(out, "server", TimeService::ntpServer());
  JsonUtils::appendKey(out, "sample"); appendSample(out, ts.ntp); out += ',';
  appendStringField(out, "error", NtpTimeProvider::errorName(ts.ntpError));
  appendUIntField(out, "rttMs", ts.ntpRttMs);
  if (ts.ntpResolvedIp.length()) appendStringField(out, "resolvedIp", ts.ntpResolvedIp);
  if (ts.ntpDeltaAvailable) appendInt64Field(out, "deltaMs", ts.ntpDeltaMs);
  if (out.endsWith(",")) out.remove(out.length() - 1);
  out += "},";

  JsonUtils::appendKey(out, "rtc");
  out += '{';
  appendBoolField(out, "reachable", ts.rtcReachable);
  appendBoolField(out, "calendarPlausible", ts.rtcCalendarPlausible);
  appendBoolField(out, "timeValid", ts.rtcTimeValid);
  appendBoolField(out, "osf", ts.rtcOsf);
  JsonUtils::appendKey(out, "sample"); appendSample(out, ts.rtc); out += ',';
  appendBoolField(out, "syncAttempted", ts.rtcSyncAttempted);
  appendBoolField(out, "syncOk", ts.rtcSyncOk);
  if (ts.rtcDeltaAvailable) appendInt64Field(out, "deltaMs", ts.rtcDeltaMs);
  if (ts.rtcBeforeSyncAvailable) {
    JsonUtils::appendKey(out, "beforeSync"); appendSample(out, ts.rtcBeforeSync); out += ',';
    if (ts.rtcBeforeSyncDeltaAvailable) appendInt64Field(out, "beforeSyncDeltaMs", ts.rtcBeforeSyncDeltaMs);
  }
  if (out.endsWith(",")) out.remove(out.length() - 1);
  out += "},";

  JsonUtils::appendKey(out, "browser");
  out += '{';
  appendBoolField(out, "attempted", ts.browserFallbackAttempted);
  appendBoolField(out, "allowed", TimeService::browserFallbackAllowed());
  JsonUtils::appendKey(out, "sample"); appendSample(out, ts.browser); out += ',';
  appendIntField(out, "timezoneOffsetMinutes", ts.browserTimezoneOffsetMinutes);
  if (ts.browserDeltaAvailable) appendInt64Field(out, "deltaMs", ts.browserDeltaMs);
  if (out.endsWith(",")) out.remove(out.length() - 1);
  out += "},";

  JsonUtils::appendKey(out, "system");
  out += '{';
  appendBoolField(out, "valid", current.valid);
  if (current.valid) appendInt64Field(out, "epochMs", current.epochMs);
  appendUInt64Field(out, "monotonicMs", current.monotonicMs);
  if (ts.systemDeltaAvailable) appendInt64Field(out, "deltaMs", ts.systemDeltaMs);
  if (out.endsWith(",")) out.remove(out.length() - 1);
  out += '}';

  out += '}';
}

}  // namespace

String buildBootstrapJson() {
  String out;
  out.reserve(4000);
  out += '{';

  JsonUtils::appendKey(out, "project");
  out += '{';
  appendStringField(out, "name", AppConfig::PROJECT_NAME);
  appendStringField(out, "icon", AppConfig::PROJECT_ICON);
  appendStringField(out, "version", AppConfig::SOFTWARE_VERSION);
  appendStringField(out, "footerComment", AppConfig::FOOTER_COMMENT);
  appendStringField(out, "githubUser", AppConfig::GITHUB_USER);
  appendStringField(out, "githubUserUrl", AppConfig::GITHUB_USER_URL);
  appendStringField(out, "timeZone", ProjectConfig::TIMEZONE_NAME);
  appendUIntField(out, "livePollMs", ProjectConfig::LIVE_POLL_INTERVAL_MS);
  appendStringField(out, "projectUrl", AppConfig::GITHUB_PROJECT_URL, false);
  out += "},";

  JsonUtils::appendKey(out, "firmware");
  out += '{';
  appendStringField(out, "name", AppConfig::FIRMWARE_NAME);
  appendStringField(out, "version", AppConfig::SOFTWARE_VERSION, false);
  out += "},";

  JsonUtils::appendKey(out, "preferences");
  out += '{';
  appendStringField(out, "fallbackLanguage", AppConfig::FALLBACK_LANGUAGE);
  appendStringField(out, "defaultTheme", AppConfig::DEFAULT_THEME);
  JsonUtils::appendKey(out, "languages");
  out += AppConfig::AVAILABLE_LANGUAGES_JSON;
  out += "},";

  const TimeTypes::Snapshot currentTime = TimeService::now();
  JsonUtils::appendKey(out, "time");
  out += '{';
  appendBoolField(out, "valid", currentTime.valid);
  appendStringField(out, "source", TimeTypes::sourceName(currentTime.source));
  appendStringField(out, "quality", TimeTypes::qualityName(currentTime.quality));
  if (currentTime.valid) appendInt64Field(out, "epochMs", currentTime.epochMs);
  appendBoolField(out, "browserFallbackAllowed", TimeService::browserFallbackAllowed(), false);
  out += "},";

  JsonUtils::appendKey(out, "timeManagement");
  appendTimeManagement(out);
  out += ',';

  JsonUtils::appendKey(out, "interruptions");
  InterruptionApi::appendSummaryObject(out);
  out += ',';

  JsonUtils::appendKey(out, "projectSettings");
  InterruptionApi::appendProjectPreferencesObject(out);
  out += ',';

  JsonUtils::appendKey(out, "status");
  StatusRegistry::appendStatusObject(out);
  out += ',';
  JsonUtils::appendKey(out, "statusProviders");
  StatusRegistry::appendProviderArray(out);

  out += '}';
  return out;
}

String buildDeviceJson() {
  String out;
  out.reserve(4300);
  out += '{';

  const uint64_t uptimeMs = static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
  const uint32_t heapTotal = ESP.getHeapSize();
  const uint32_t heapFree = ESP.getFreeHeap();
  const uint32_t heapMin = ESP.getMinFreeHeap();
  const uint32_t heapUsed = heapTotal > heapFree ? heapTotal - heapFree : 0;
  const uint32_t usedPercent = heapTotal == 0 ? 0 : static_cast<uint32_t>((static_cast<uint64_t>(heapUsed) * 100ULL) / heapTotal);
  const uint32_t psramSize = ESP.getPsramSize();

  JsonUtils::appendKey(out, "device");
  out += '{';
  appendStringField(out, "firmware", AppConfig::FIRMWARE_NAME);
  appendStringField(out, "version", AppConfig::SOFTWARE_VERSION);
  appendUInt64Field(out, "uptimeMs", uptimeMs);
  appendStringField(out, "board", AppConfig::BOARD_NAME);
  appendStringField(out, "chip", ESP.getChipModel());
  appendUIntField(out, "cores", ESP.getChipCores());
  appendUIntField(out, "flashBytes", ESP.getFlashChipSize());
  if (psramSize > 0) {
    appendUIntField(out, "psramBytes", psramSize, false);
  } else if (out.endsWith(",")) {
    out.remove(out.length() - 1);
  }
  out += "},";

  const bool connected = WifiModule::stationConnected();
  const bool apActive = WifiModule::accessPointActive();
  JsonUtils::appendKey(out, "wifi");
  out += '{';
  appendBoolField(out, "connected", connected);
  appendBoolField(out, "apActive", apActive);
  appendStringField(out, "state", WifiModule::stateName());
  appendStringField(out, "mode", WifiModule::modeName());

  const String currentSsid = WifiModule::ssid();
  const String currentIp = WifiModule::ip();
  if (currentSsid.length()) {
    JsonUtils::appendKey(out, "ssid");
    JsonUtils::appendEscapedString(out, currentSsid);
    out += ',';
  }
  if (currentIp.length()) {
    JsonUtils::appendKey(out, "ip");
    JsonUtils::appendEscapedString(out, currentIp);
    out += ',';
  }
  if (connected) appendIntField(out, "rssi", WifiModule::rssi());
  if (apActive) appendUIntField(out, "clients", WifiModule::accessPointClients());
  if (out.endsWith(",")) out.remove(out.length() - 1);
  out += "},";

  JsonUtils::appendKey(out, "memory");
  out += '{';
  appendUIntField(out, "heapTotal", heapTotal);
  appendUIntField(out, "heapFree", heapFree);
  appendUIntField(out, "heapMin", heapMin);
  appendUIntField(out, "heapUsedPercent", usedPercent, false);
  out += "},";

  JsonUtils::appendKey(out, "hardware");
  HardwareRegistry::appendJson(out);
  out += ',';

  JsonUtils::appendKey(out, "ota");
  out += '{';
  appendBoolField(out, "supported", OtaModule::supported());
  appendUIntField(out, "currentBytes", OtaModule::currentFirmwareBytes());
  appendUIntField(out, "maxBytes", OtaModule::maxFirmwareBytes());
  appendUIntField(out, "headroomBytes", OtaModule::projectHeadroomBytes(), false);
  out += '}';

  out += '}';
  return out;
}

String buildHardwareJson() {
  String out;
  out.reserve(3000);
  out += '{';
  JsonUtils::appendKey(out, "hardware");
  HardwareRegistry::appendJson(out);
  out += ',';
  JsonUtils::appendKey(out, "status");
  StatusRegistry::appendStatusObject(out);
  out += '}';
  return out;
}

String buildTimeJson() {
  String out;
  out.reserve(2600);
  out += '{';
  appendBoolField(out, "ok", true);
  JsonUtils::appendKey(out, "time");
  appendTimeManagement(out);
  out += ',';
  JsonUtils::appendKey(out, "status");
  StatusRegistry::appendStatusObject(out);
  out += '}';
  return out;
}

}  // namespace Api
