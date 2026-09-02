#include "web_server.h"

#include <Arduino.h>
#include <WebServer.h>
#include <cstdlib>

#include "api.h"
#include "config.h"
#include "serial_log.h"
#include "ota_module.h"
#include "web_assets.h"
#include "wifi_module.h"
#include "hardware_registry.h"
#include "time_service.h"
#include "interruption_api.h"
#include "interruption_service.h"
#include "interruption_store.h"
#include "interruption_aggregates.h"
#include "project_preferences.h"
#include "project_config.h"
#include "display_views.h"

namespace {

WebServer server(80);
bool routesRegistered = false;
bool serverStarted = false;
const char *COLLECTED_HEADERS[] = {"If-None-Match", "X-Firmware-Size"};

void sendJson(const String &json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", json);
}

void handleIndex() {
  const String requestTag = server.header("If-None-Match");
  if (requestTag == WEB_ASSET_ETAG) {
    server.sendHeader("Cache-Control", AppConfig::WEB_CACHE_CONTROL);
    server.sendHeader("ETag", WEB_ASSET_ETAG);
    server.send(304, "text/plain", "");
    return;
  }

  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", AppConfig::WEB_CACHE_CONTROL);
  server.sendHeader("ETag", WEB_ASSET_ETAG);
  server.send_P(200, PSTR("text/html; charset=utf-8"), reinterpret_cast<PGM_P>(INDEX_HTML_GZ), INDEX_HTML_GZ_LEN);
}

void handleBootstrap() {
  sendJson(Api::buildBootstrapJson());
}

void handleDevice() {
  sendJson(Api::buildDeviceJson());
}

void handleHardware() {
  sendJson(Api::buildHardwareJson());
}

void handleHardwareCheck() {
  const String id = server.arg("id");
  if (id.length()) {
    if (!HardwareRegistry::hasModule(id.c_str())) {
      server.send(404, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"unknown_module\"}");
      return;
    }
    if (!HardwareRegistry::probe(id.c_str())) {
      server.send(409, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"check_rejected\"}");
      return;
    }
  } else {
    HardwareRegistry::probeAll();
  }
  sendJson(Api::buildHardwareJson());
}

void handleHardwareAction() {
  const String id = server.arg("id");
  const String action = server.arg("action");
  if (!id.length() || !action.length()) {
    server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"missing_argument\"}");
    return;
  }
  if (!HardwareRegistry::action(id.c_str(), action.c_str())) {
    server.send(409, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"action_rejected\"}");
    return;
  }
  sendJson(Api::buildHardwareJson());
}

void handleTime() {
  sendJson(Api::buildTimeJson());
}

void sendTimeOperationError(int httpStatus, const char *error);

void handleTimeCheck() {
  const TimeService::OperationResult result = TimeService::check();
  if (!result.ok) {
    sendTimeOperationError(409, result.error);
    return;
  }
  sendJson(Api::buildTimeJson());
}

void sendTimeOperationError(int httpStatus, const char *error) {
  String json;
  json.reserve(96);
  json += F("{\"ok\":false,\"error\":\"");
  json += error ? error : "unknown";
  json += F("\"}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(httpStatus, "application/json; charset=utf-8", json);
}

void handleNtpServer() {
  const String candidate = server.arg("server");
  if (!candidate.length()) {
    sendTimeOperationError(400, "invalid_server");
    return;
  }
  const TimeService::OperationResult result = TimeService::checkAndSaveNtpServer(candidate.c_str());
  if (!result.ok) {
    sendTimeOperationError(409, result.error);
    return;
  }
  sendJson(Api::buildTimeJson());
}

void handleBrowserTime() {
  const String epochText = server.arg("epochMs");
  const String offsetText = server.arg("tzOffset");
  if (!epochText.length()) {
    sendTimeOperationError(400, "browser_time_invalid");
    return;
  }
  char *end = nullptr;
  const int64_t epochMs = static_cast<int64_t>(strtoll(epochText.c_str(), &end, 10));
  if (!end || *end != '\0') {
    sendTimeOperationError(400, "browser_time_invalid");
    return;
  }
  int16_t timezoneOffsetMinutes = 0;
  if (offsetText.length()) {
    char *offsetEnd = nullptr;
    const long parsedOffset = std::strtol(offsetText.c_str(), &offsetEnd, 10);
    if (!offsetEnd || *offsetEnd != '\0' || parsedOffset < -1440L || parsedOffset > 1440L) {
      sendTimeOperationError(400, "browser_time_invalid");
      return;
    }
    timezoneOffsetMinutes = static_cast<int16_t>(parsedOffset);
  }
  const TimeService::OperationResult result = TimeService::acceptBrowserTime(epochMs, timezoneOffsetMinutes);
  if (!result.ok) {
    sendTimeOperationError(409, result.error);
    return;
  }
  sendJson(Api::buildTimeJson());
}


void handleInterruptionEvent() {
  if (!InterruptionService::captureWeb()) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(503, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"capture_queue_full\"}");
    return;
  }
  sendJson(InterruptionApi::buildSummaryJson(true));
}

void handleInterruptionLive() {
  const String sinceText = server.arg("since");
  uint64_t since = 0;
  if (sinceText.length()) {
    char *end = nullptr;
    since = static_cast<uint64_t>(strtoull(sinceText.c_str(), &end, 10));
    if (!end || *end != '\0') since = 0;
  }
  if (since == InterruptionService::summary().revision) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(204, "text/plain", "");
    return;
  }
  sendJson(InterruptionApi::buildSummaryJson(true));
}

void handleInterruptionSound() {
  const String enabledText = server.arg("enabled");
  if (enabledText != "1" && enabledText != "0" && enabledText != "true" && enabledText != "false") {
    server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_sound_value\"}");
    return;
  }
  const bool enabled = enabledText == "1" || enabledText == "true";
  if (!InterruptionService::setSoundEnabled(enabled)) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"preference_write_failed\"}");
    return;
  }
  sendJson(InterruptionApi::buildSummaryJson(true));
}

bool parseBoolArg(const String &value, bool &parsed) {
  if (value == "1" || value == "true") { parsed = true; return true; }
  if (value == "0" || value == "false") { parsed = false; return true; }
  return false;
}

bool parseUnsignedArg(const String &value, uint32_t minValue, uint32_t maxValue, uint32_t &parsed) {
  if (!value.length()) return false;
  char *end = nullptr;
  const unsigned long raw = strtoul(value.c_str(), &end, 10);
  if (!end || *end != '\0' || raw < minValue || raw > maxValue) return false;
  parsed = static_cast<uint32_t>(raw);
  return true;
}

void handleProjectPreferences() {
  const bool hasSound = server.hasArg("soundEnabled");
  const bool hasTrack = server.hasArg("soundTrack");
  const bool hasSoundMode = server.hasArg("soundMode");
  const bool hasFlash = server.hasArg("displayFlashEnabled");
  const bool hasMode = server.hasArg("displayMode");
  const bool hasBrightness = server.hasArg("displayBrightness");
  const bool hasDimAfter = server.hasArg("displayDimAfterMinutes");
  const bool hasDimBrightness = server.hasArg("displayDimBrightness");
  const uint8_t fields = static_cast<uint8_t>(hasSound) + static_cast<uint8_t>(hasTrack) +
                         static_cast<uint8_t>(hasSoundMode) + static_cast<uint8_t>(hasFlash) + static_cast<uint8_t>(hasMode) +
                         static_cast<uint8_t>(hasBrightness) + static_cast<uint8_t>(hasDimAfter) +
                         static_cast<uint8_t>(hasDimBrightness);
  if (fields != 1U) {
    server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"exactly_one_preference_required\"}");
    return;
  }

  bool ok = false;
  bool displayChanged = false;
  if (hasSound) {
    bool value = false;
    if (!parseBoolArg(server.arg("soundEnabled"), value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_sound_value\"}");
      return;
    }
    ok = InterruptionService::setSoundEnabled(value);
  } else if (hasTrack) {
    uint32_t value = 0;
    if (!parseUnsignedArg(server.arg("soundTrack"), 2, 65535, value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_track\"}");
      return;
    }
    ok = ProjectPreferences::setSoundTrack(static_cast<uint16_t>(value));
  } else if (hasSoundMode) {
    ProjectPreferences::SoundMode value = ProjectPreferences::SoundMode::Fixed;
    if (!ProjectPreferences::parseSoundMode(server.arg("soundMode").c_str(), value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_sound_mode\"}");
      return;
    }
    ok = ProjectPreferences::setSoundMode(value);
  } else if (hasFlash) {
    bool value = false;
    if (!parseBoolArg(server.arg("displayFlashEnabled"), value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_display_flash\"}");
      return;
    }
    ok = ProjectPreferences::setDisplayFlashEnabled(value);
    displayChanged = ok;
  } else if (hasMode) {
    ProjectPreferences::DisplayMode value = ProjectPreferences::DisplayMode::Standard;
    if (!ProjectPreferences::parseDisplayMode(server.arg("displayMode").c_str(), value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_display_mode\"}");
      return;
    }
    ok = ProjectPreferences::setDisplayMode(value);
    displayChanged = ok;
  } else if (hasBrightness) {
    uint32_t value = 0;
    if (!parseUnsignedArg(server.arg("displayBrightness"), 1, 100, value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_display_brightness\"}");
      return;
    }
    ok = ProjectPreferences::setDisplayBrightnessPercent(static_cast<uint8_t>(value));
    displayChanged = ok;
  } else if (hasDimAfter) {
    uint32_t value = 0;
    if (!parseUnsignedArg(server.arg("displayDimAfterMinutes"), 0, ProjectConfig::DISPLAY_DIM_AFTER_MAX_MINUTES, value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_display_dim_timeout\"}");
      return;
    }
    ok = ProjectPreferences::setDisplayDimAfterMinutes(static_cast<uint16_t>(value));
    displayChanged = ok;
  } else if (hasDimBrightness) {
    uint32_t value = 0;
    if (!parseUnsignedArg(server.arg("displayDimBrightness"), 0, 100, value)) {
      server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_display_dim_brightness\"}");
      return;
    }
    ok = ProjectPreferences::setDisplayDimBrightnessPercent(static_cast<uint8_t>(value));
    displayChanged = ok;
  }

  if (!ok) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"preference_write_failed\"}");
    return;
  }
  if (displayChanged) DisplayViews::settingsChanged();
  sendJson(InterruptionApi::buildProjectPreferencesJson(true));
}

void handleInterruptionStorage() {
  sendJson(InterruptionApi::buildStorageJson());
}

void sendProjectDataUnavailable(const char *reason) {
  String json;
  json.reserve(96);
  json += F("{\"ok\":false,\"error\":\"");
  json += reason ? reason : "data_unavailable";
  json += F("\"}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(503, "application/json; charset=utf-8", json);
}

void handleAnalyticsBundle() {
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }

  const String mode = server.arg("hourlyMode");
  const uint16_t hourlyYear = static_cast<uint16_t>(server.arg("hourlyYear").toInt());
  const uint8_t hourlyWeek = static_cast<uint8_t>(server.arg("hourlyWeek").toInt());
  const String from = server.arg("from");
  const String to = server.arg("to");
  const uint16_t monthWeekYear = static_cast<uint16_t>(server.arg("monthWeekYear").toInt());
  bool valid = false;
  const String json = InterruptionApi::buildAnalyticsBundleJson(
      mode.c_str(), hourlyYear, hourlyWeek, from.c_str(), to.c_str(), monthWeekYear, valid);

  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  if (!valid) {
    server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_filter\"}");
    return;
  }
  sendJson(json);
}

void handleHeatmapHourly() {
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  const String mode = server.arg("mode");
  const String yearText = server.arg("year");
  const String weekText = server.arg("week");
  const String from = server.arg("from");
  const String to = server.arg("to");
  const uint16_t year = static_cast<uint16_t>(yearText.toInt());
  const uint8_t week = static_cast<uint8_t>(weekText.toInt());
  bool valid = false;
  const String json = InterruptionApi::buildHourlyHeatmapJson(mode.c_str(), year, week, from.c_str(), to.c_str(), valid);
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  if (!valid) {
    server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_filter\"}");
    return;
  }
  sendJson(json);
}

void handleHeatmapMonthWeek() {
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  const uint16_t year = static_cast<uint16_t>(server.arg("year").toInt());
  bool valid = false;
  const String json = InterruptionApi::buildMonthWeekHeatmapJson(year, valid);
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  if (!valid) {
    server.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_year\"}");
    return;
  }
  sendJson(json);
}

void handleHeatmapYearMonth() {
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  const String json = InterruptionApi::buildYearMonthHeatmapJson();
  if (!InterruptionAggregates::ready()) {
    sendProjectDataUnavailable("statistics_rebuilding");
    return;
  }
  sendJson(json);
}

void handleInterruptionCsv() {
  if (!InterruptionStore::ready()) {
    sendProjectDataUnavailable("raw_store_recovering");
    return;
  }
  InterruptionApi::streamCsv(server);
}

void handleNotFound() {
  if (server.method() == HTTP_GET) {
    server.send(404, "text/plain; charset=utf-8", "Not found");
    return;
  }
  server.send(405, "text/plain; charset=utf-8", "Method not allowed");
}

void registerRoutes() {
  if (routesRegistered) return;

  server.collectHeaders(COLLECTED_HEADERS, 2);
  server.on("/", HTTP_GET, handleIndex);
  server.on("/index.html", HTTP_GET, handleIndex);
  server.on("/api/bootstrap", HTTP_GET, handleBootstrap);
  server.on("/api/device", HTTP_GET, handleDevice);
  server.on("/api/hardware", HTTP_GET, handleHardware);
  server.on("/api/hardware/check", HTTP_POST, handleHardwareCheck);
  server.on("/api/hardware/action", HTTP_POST, handleHardwareAction);
  server.on("/api/time", HTTP_GET, handleTime);
  server.on("/api/time/check", HTTP_POST, handleTimeCheck);
  server.on("/api/time/ntp", HTTP_POST, handleNtpServer);
  server.on("/api/time/browser", HTTP_POST, handleBrowserTime);
  server.on("/api/interruptions/event", HTTP_POST, handleInterruptionEvent);
  server.on("/api/interruptions/live", HTTP_GET, handleInterruptionLive);
  server.on("/api/interruptions/sound", HTTP_POST, handleInterruptionSound);
  server.on("/api/interruptions/preferences", HTTP_POST, handleProjectPreferences);
  server.on("/api/interruptions/storage", HTTP_GET, handleInterruptionStorage);
  server.on("/api/interruptions/analytics", HTTP_GET, handleAnalyticsBundle);
  server.on("/api/interruptions/heatmap/hourly", HTTP_GET, handleHeatmapHourly);
  server.on("/api/interruptions/heatmap/month-week", HTTP_GET, handleHeatmapMonthWeek);
  server.on("/api/interruptions/heatmap/year-month", HTTP_GET, handleHeatmapYearMonth);
  server.on("/api/interruptions/export.csv", HTTP_GET, handleInterruptionCsv);
  OtaModule::registerRoutes(server);
  server.onNotFound(handleNotFound);
  routesRegistered = true;
}

bool startServerIfNetworkReady() {
  if (serverStarted) return true;
  if (!WifiModule::stationConnected() && !WifiModule::accessPointActive()) return false;

  server.begin();
  serverStarted = true;
  SerialLog::success("WEB", "HTTP server started on port 80");
  const String address = WifiModule::ip();
  if (address.length()) SerialLog::infof("WEB", "Open http://%s/", address.c_str());
  return true;
}

}  // namespace

void beginWebServer() {
  registerRoutes();
  if (!startServerIfNetworkReady()) {
    SerialLog::info("WEB", "HTTP routes ready; server waits for a usable Wi-Fi interface");
  }
}

void handleWebServer() {
  if (!serverStarted && !startServerIfNetworkReady()) return;
  server.handleClient();
}
