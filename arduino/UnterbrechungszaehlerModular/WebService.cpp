#include "WebService.h"

#include <LittleFS.h>
#include <WiFiUdp.h>
#include <time.h>

#include "AnalyticsService.h"
#include "AutarkService.h"
#include "Config.h"
#include "CounterService.h"
#include "DisplayService.h"
#include "NetworkService.h"
#include "RtcService.h"
#include "StorageService.h"
#include "TimeService.h"
#include "WebUi.h"
#include "WebUiPatch.h"
#include "WebUiFixes.h"
#include "WebUiNetwork.h"
#include "WebUiDisplay.h"

namespace {
String exportDownloadName(const char* baseName, TimeService* timeService) {
  String filename(baseName);

  if (timeService && timeService->valid()) {
    time_t raw = static_cast<time_t>(timeService->epoch());
    struct tm value = {};
    localtime_r(&raw, &value);

    char timestamp[32];
    snprintf(timestamp,
             sizeof(timestamp),
             "_%04d-%02d-%02d_%02d-%02d-%02d.csv",
             value.tm_year + 1900,
             value.tm_mon + 1,
             value.tm_mday,
             value.tm_hour,
             value.tm_min,
             value.tm_sec);
    filename += timestamp;
  } else {
    filename += "_zeit-unbekannt.csv";
  }

  return filename;
}

String attachmentHeader(const String& filename) {
  return String("attachment; filename=\"") + filename + "\"";
}

bool boolArg(const String& value) {
  return value == "1" || value == "true" || value == "on" || value == "yes";
}
}

WebService::WebService() : server_(80) {}

void WebService::begin(StorageService* storage,
                       TimeService* time,
                       NetworkService* network,
                       CounterService* counter,
                       AutarkService* autark,
                       RtcService* rtc,
                       DisplayService* display,
                       AnalyticsService* analytics) {
  storage_ = storage;
  time_ = time;
  network_ = network;
  counter_ = counter;
  autark_ = autark;
  rtc_ = rtc;
  display_ = display;
  analytics_ = analytics;
  registerRoutes();
}

void WebService::tick(bool enabled) {
  if (!enabled) {
    stop();
    return;
  }
  startIfNeeded();
  if (started_) server_.handleClient();
}

void WebService::stop() {
  if (!started_) return;
  server_.stop();
  started_ = false;
}

void WebService::startIfNeeded() {
  if (started_) return;
  server_.begin();
  started_ = true;
}

void WebService::registerRoutes() {
  if (routesRegistered_) return;
  routesRegistered_ = true;

  server_.on("/", HTTP_GET, [this]() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html; charset=utf-8", "");
    server_.sendContent_P(WEB_UI);
    server_.sendContent_P(WEB_UI_PATCH);
    server_.sendContent_P(WEB_UI_FIXES);
    server_.sendContent_P(WEB_UI_NETWORK);
    server_.sendContent_P(WEB_UI_DISPLAY);
    server_.sendContent("");
  });

  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/events", HTTP_GET, [this]() { sendEvents(); });
  server_.on("/api/autark", HTTP_GET, [this]() { sendAutark(); });
  server_.on("/api/aggregate", HTTP_GET, [this]() { sendAggregate(); });
  server_.on("/api/display-preview", HTTP_GET, [this]() { sendDisplayPreview(); });

  server_.on("/api/add", HTTP_POST, [this]() {
    if (autark_ && autark_->active()) {
      sendJsonError(409, "autark_active");
      return;
    }
    if (!counter_ || !counter_->addNormalEvent(false)) {
      sendJsonError(503, "event_not_stored");
      return;
    }
    if (display_) display_->notifyEvent(false);
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.on("/api/delete-last", HTTP_POST, [this]() {
    if (autark_ && autark_->active()) {
      sendJsonError(409, "autark_active");
      return;
    }
    if (!counter_ || !counter_->deleteNormalEvent()) {
      sendJsonError(404, "no_event");
      return;
    }
    if (display_) display_->notifyActivity(false);
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.on("/api/time", HTTP_POST, [this]() {
    if (!time_) {
      sendJsonError(500, "time_unavailable");
      return;
    }
    const uint32_t epoch = strtoul(server_.arg("epoch").c_str(), nullptr, 10);
    if (time_->valid()) {
      server_.send(200, "application/json", "{\"ok\":true,\"accepted\":false}");
      return;
    }
    if (!time_->setFromBrowser(epoch)) {
      sendJsonError(400, "invalid_time");
      return;
    }
    server_.send(200, "application/json", "{\"ok\":true,\"accepted\":true}");
  });

  server_.on("/api/ntp", HTTP_POST, [this]() {
    if (!time_ || !network_ || !network_->connected()) {
      sendJsonError(503, "network_unavailable");
      return;
    }
    String host = server_.arg("server");
    host.trim();
    if (!time_->validNtpHost(host) || !testNtp(host) || !time_->setPrimaryNtp(host)) {
      sendJsonError(400, "invalid_ntp");
      return;
    }
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.on("/api/rtc-sync", HTTP_POST, [this]() {
    if (!rtc_ || !rtc_->present() || !rtc_->writeSystemTime()) {
      sendJsonError(409, "rtc_sync_failed");
      return;
    }
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.on("/api/display-test", HTTP_POST, [this]() {
    if (!display_ || !display_->showTest(autark_ && autark_->active())) {
      sendJsonError(404, "display_unavailable");
      return;
    }
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.on("/api/display-settings", HTTP_POST, [this]() {
    if (!display_ || !display_->present()) {
      sendJsonError(404, "display_unavailable");
      return;
    }

    const int brightness = server_.hasArg("brightness") ? server_.arg("brightness").toInt() : display_->brightness();
    const int dimBrightness = server_.hasArg("dimBrightness") ? server_.arg("dimBrightness").toInt() : display_->dimBrightness();
    const int dimAfter = server_.hasArg("dimAfter") ? server_.arg("dimAfter").toInt() : display_->dimAfterSeconds();
    const long offAfter = server_.hasArg("offAfter") ? server_.arg("offAfter").toInt() : static_cast<long>(display_->offAfterSeconds());
    const int layoutValue = server_.hasArg("layout") ? server_.arg("layout").toInt() : static_cast<int>(display_->layout());
    const bool wakeOnEvent = server_.hasArg("wakeOnEvent") ? boolArg(server_.arg("wakeOnEvent")) : display_->wakeOnEvent();
    const bool inverted = server_.hasArg("inverted") ? boolArg(server_.arg("inverted")) : display_->inverted();
    const bool rotation180 = server_.hasArg("rotation180") ? boolArg(server_.arg("rotation180")) : display_->rotation180();

    if (brightness < 1 || brightness > 255 ||
        dimBrightness < 1 || dimBrightness > 255 ||
        dimAfter < 5 || dimAfter > 3600 ||
        offAfter < 0 || offAfter > 86400 || (offAfter > 0 && offAfter < 5) ||
        layoutValue < 0 || layoutValue > 2) {
      sendJsonError(400, "invalid_display_settings");
      return;
    }

    if (!display_->setSettings(static_cast<uint8_t>(brightness),
                               static_cast<uint8_t>(dimBrightness),
                               static_cast<uint16_t>(dimAfter),
                               static_cast<uint32_t>(offAfter),
                               wakeOnEvent,
                               inverted,
                               rotation180,
                               static_cast<DisplayLayout>(layoutValue))) {
      sendJsonError(500, "display_settings_failed");
      return;
    }
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.on("/export.csv", HTTP_GET, [this]() { exportNormalCsv(); });
  server_.on("/archive.csv", HTTP_GET, [this]() { exportArchiveCsv(); });
  server_.on("/autark.csv", HTTP_GET, [this]() { exportAutarkCsv(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found"); });
}

void WebService::sendStatus() {
  const size_t fsTotal = storage_ ? storage_->fsTotalBytes() : 0;
  const size_t fsUsed = storage_ ? storage_->fsUsedBytes() : 0;

  String json;
  json.reserve(2100);
  json = "{\"ok\":true";
  json += ",\"version\":\"" + String(UicConfig::APP_VERSION) + "\"";
  json += ",\"deviceDate\":\"" + (time_ ? time_->localDate() : String("-")) + "\"";
  json += ",\"deviceTime\":\"" + (time_ ? time_->localTime() : String("-")) + "\"";
  json += ",\"timeSource\":\"" + String(time_ ? timeSourceName(time_->source()) : "none") + "\"";
  json += ",\"ntpPrimary\":\"" + (time_ ? time_->primaryNtp() : String("-")) + "\"";
  json += ",\"uptime\":" + String(millis() / 1000UL);
  json += ",\"last\":" + String(storage_ ? storage_->lastEvent() : 0);
  json += ",\"eventCount\":" + String(storage_ ? storage_->recentCount() : 0);
  json += ",\"ringCapacity\":" + String(storage_ ? storage_->recentCapacity() : 0);
  json += ",\"historyDays\":" + String(UicConfig::HISTORY_DAYS);
  json += ",\"webEventLimit\":" + String(UicConfig::WEB_EVENT_LIMIT);
  json += ",\"archiveCount\":" + String(storage_ ? storage_->archiveCount() : 0);
  json += ",\"archiveCapacity\":" + String(storage_ ? storage_->archiveCapacity() : 0);
  json += ",\"archiveSync\":" + String(storage_ && storage_->archiveSynchronized() ? "true" : "false");
  json += ",\"autarkCount\":" + String(storage_ ? storage_->autarkCount() : 0);
  json += ",\"autarkCapacity\":" + String(storage_ ? storage_->autarkCapacity() : 0);
  json += ",\"fsTotal\":" + String(static_cast<uint32_t>(fsTotal));
  json += ",\"fsUsed\":" + String(static_cast<uint32_t>(fsUsed));
  json += ",\"wifi\":" + String(network_ && network_->connected() ? "true" : "false");
  json += ",\"rssi\":" + String(network_ ? network_->rssi() : 0);
  json += ",\"ip\":\"" + (network_ ? network_->ip() : String("-")) + "\"";
  json += ",\"heapFree\":" + String(ESP.getFreeHeap());
  json += ",\"heapTotal\":" + String(ESP.getHeapSize());
  json += ",\"flashTotal\":" + String(ESP.getFlashChipSize());
  json += ",\"sketchUsed\":" + String(ESP.getSketchSize());
  json += ",\"sketchFree\":" + String(ESP.getFreeSketchSpace());
  json += ",\"rtcPresent\":" + String(rtc_ && rtc_->present() ? "true" : "false");
  json += ",\"rtcValid\":" + String(rtc_ && rtc_->timeValid() ? "true" : "false");
  json += ",\"rtcDate\":\"" + (rtc_ ? rtc_->dateText() : String("-")) + "\"";
  json += ",\"rtcTime\":\"" + (rtc_ ? rtc_->timeText() : String("-")) + "\"";
  json += ",\"displayPresent\":" + String(display_ && display_->present() ? "true" : "false");
  json += ",\"displayActive\":" + String(display_ && display_->active() ? "true" : "false");
  json += ",\"displayDimmed\":" + String(display_ && display_->dimmed() ? "true" : "false");
  json += ",\"displayBrightness\":" + String(display_ ? display_->brightness() : 0);
  json += ",\"displayDimBrightness\":" + String(display_ ? display_->dimBrightness() : 0);
  json += ",\"displayDimAfter\":" + String(display_ ? display_->dimAfterSeconds() : 0);
  json += ",\"displayOffAfter\":" + String(display_ ? display_->offAfterSeconds() : 0);
  json += ",\"displayWakeOnEvent\":" + String(display_ && display_->wakeOnEvent() ? "true" : "false");
  json += ",\"displayInverted\":" + String(display_ && display_->inverted() ? "true" : "false");
  json += ",\"displayRotation180\":" + String(display_ && display_->rotation180() ? "true" : "false");
  json += ",\"displayLayout\":" + String(display_ ? static_cast<uint8_t>(display_->layout()) : 0);
  json += ",\"displayFrameRevision\":" + String(display_ ? display_->frameRevision() : 0);
  json += ",\"autarkMode\":" + String(autark_ && autark_->active() ? "true" : "false");
  json += ",\"autarkSession\":" + String(autark_ ? autark_->sessionId() : 0);
  json += ",\"autarkElapsed\":" + String(autark_ ? (autark_->active() ? autark_->elapsedSeconds() : autark_->lastElapsedSeconds()) : 0);
  json += ",\"autarkSessionEvents\":" + String(autark_ ? autark_->sessionEvents() : 0);
  json += "}";

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", json);
}

void WebService::sendEvents() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "application/json",
               "{\"ok\":true,\"limit\":" + String(UicConfig::WEB_EVENT_LIMIT) +
               ",\"historyDays\":" + String(UicConfig::HISTORY_DAYS) +
               ",\"events\":[");

  bool first = true;
  if (storage_) {
    const uint32_t count = storage_->recentCount();
    const uint32_t start = count > UicConfig::WEB_EVENT_LIMIT ? count - UicConfig::WEB_EVENT_LIMIT : 0;
    uint32_t cutoff = 0;
    if (time_ && time_->valid()) {
      cutoff = time_->epoch() - static_cast<uint32_t>(UicConfig::HISTORY_DAYS) * 86400UL;
    }

    for (uint32_t i = start; i < count; i++) {
      uint32_t epoch = 0;
      if (!storage_->readRecent(i, epoch)) continue;
      if (cutoff && epoch < cutoff) continue;
      if (!first) server_.sendContent(",");
      first = false;
      server_.sendContent(String(epoch));
    }
  }

  server_.sendContent("]}");
}

void WebService::sendAutark() {
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "application/json", "{\"ok\":true,\"records\":[");

  bool first = true;
  if (storage_) {
    const uint32_t start = storage_->autarkCount() > 200 ? storage_->autarkCount() - 200 : 0;
    for (uint32_t i = start; i < storage_->autarkCount(); i++) {
      AutarkRecord record = {};
      if (!storage_->readAutark(i, record)) continue;
      if (!first) server_.sendContent(",");
      first = false;
      String row = "{\"session\":" + String(record.sessionId) +
                   ",\"type\":" + String(record.type) +
                   ",\"elapsed\":" + String(record.elapsedSec) +
                   ",\"epoch\":" + String(record.anchorEpoch) + "}";
      server_.sendContent(row);
    }
  }

  server_.sendContent("]}");
}

void WebService::sendAggregate() {
  if (!analytics_ || !storage_) {
    sendJsonError(503, "aggregate_unavailable");
    return;
  }

  const uint32_t started = millis();
  if (!analytics_->ensureBaseAggregates()) {
    sendJsonError(503, "aggregate_unavailable");
    return;
  }

  time_t now = time(nullptr);
  struct tm value = {};
  localtime_r(&now, &value);

  const int weekYear = server_.hasArg("weekYear") ? server_.arg("weekYear").toInt() : AnalyticsService::isoYear(value);
  const int week = server_.hasArg("week") ? server_.arg("week").toInt() : AnalyticsService::isoWeek(value);
  const int selectedYear = server_.hasArg("year") ? server_.arg("year").toInt() : analytics_->baseYear();

  if (!analytics_->ensureSelectedWeek(weekYear, week)) {
    sendJsonError(503, "week_unavailable");
    return;
  }

  const int selectedOffset = analytics_->baseYear() - selectedYear;
  String json;
  json.reserve(12000);
  json = "{\"ok\":true";
  json += ",\"stored\":" + String(storage_->archiveCount());
  json += ",\"capacity\":" + String(storage_->archiveCapacity());
  json += ",\"baseYear\":" + String(analytics_->baseYear());
  json += ",\"selectedYear\":" + String(selectedYear);
  json += ",\"selectedWeekYear\":" + String(weekYear);
  json += ",\"selectedWeek\":" + String(week);

  json += ",\"years\":[";
  bool first = true;
  for (uint8_t y = 0; y < UicConfig::LONGTERM_CACHE_YEARS; y++) {
    if (!analytics_->yearUsed(y) && y > 4) continue;
    if (!first) json += ',';
    first = false;
    json += String(analytics_->baseYear() - y);
  }

  json += "],\"weekdayHour\":[";
  for (uint8_t d = 0; d < 7; d++) {
    if (d) json += ',';
    json += '[';
    for (uint8_t h = 0; h < 24; h++) {
      if (h) json += ',';
      json += String(analytics_->selectedWeekdayHour(d, h));
    }
    json += ']';
  }

  json += "],\"monthWeek\":[";
  for (uint8_t m = 0; m < 12; m++) {
    if (m) json += ',';
    json += '[';
    for (uint8_t w = 0; w < 53; w++) {
      if (w) json += ',';
      const uint16_t amount = selectedOffset >= 0 && selectedOffset < UicConfig::LONGTERM_CACHE_YEARS
                                ? analytics_->monthWeek(selectedOffset, m, w)
                                : 0;
      json += String(amount);
    }
    json += ']';
  }

  json += "],\"yearMonth\":[";
  const uint8_t yearRows = min<uint8_t>(5, UicConfig::LONGTERM_CACHE_YEARS);
  for (uint8_t y = 0; y < yearRows; y++) {
    if (y) json += ',';
    json += '[';
    for (uint8_t m = 0; m < 12; m++) {
      if (m) json += ',';
      json += String(analytics_->yearMonth(y, m));
    }
    json += ']';
  }
  json += ']';
  json += ",\"buildMs\":" + String(millis() - started);
  json += '}';

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", json);
}

void WebService::sendDisplayPreview() {
  if (!display_ || !display_->present()) {
    sendJsonError(404, "display_unavailable");
    return;
  }

  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  const uint8_t* frame = display_->framebuffer();
  String hex;
  hex.reserve(DisplayService::FRAMEBUFFER_SIZE * 2);
  for (size_t i = 0; i < DisplayService::FRAMEBUFFER_SIZE; i++) {
    hex += HEX_DIGITS[(frame[i] >> 4) & 0x0F];
    hex += HEX_DIGITS[frame[i] & 0x0F];
  }

  String json;
  json.reserve(2450);
  json = "{\"ok\":true,\"width\":128,\"height\":64";
  json += ",\"active\":" + String(display_->active() ? "true" : "false");
  json += ",\"dimmed\":" + String(display_->dimmed() ? "true" : "false");
  json += ",\"brightness\":" + String(display_->brightness());
  json += ",\"effectiveBrightness\":" + String(display_->effectiveBrightness());
  json += ",\"inverted\":" + String(display_->inverted() ? "true" : "false");
  json += ",\"rotation180\":" + String(display_->rotation180() ? "true" : "false");
  json += ",\"layout\":" + String(static_cast<uint8_t>(display_->layout()));
  json += ",\"revision\":" + String(display_->frameRevision());
  json += ",\"frame\":\"" + hex + "\"}";

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", json);
}

void WebService::exportNormalCsv() {
  if (!storage_) {
    server_.send(503, "text/plain", "Storage unavailable");
    return;
  }

  const char* path = "/export.csv";
  LittleFS.remove(path);
  File out = LittleFS.open(path, FILE_WRITE);
  if (!out) {
    server_.send(500, "text/plain", "Export failed");
    return;
  }

  out.print("Datum;Zeit;Unixzeit\r\n");
  for (uint32_t i = 0; i < storage_->recentCount(); i++) {
    uint32_t epoch = 0;
    if (!storage_->readRecent(i, epoch)) continue;
    time_t raw = static_cast<time_t>(epoch);
    struct tm value = {};
    localtime_r(&raw, &value);
    out.printf("%02d.%02d.%04d;%02d:%02d:%02d;%lu\r\n",
               value.tm_mday,
               value.tm_mon + 1,
               value.tm_year + 1900,
               value.tm_hour,
               value.tm_min,
               value.tm_sec,
               static_cast<unsigned long>(epoch));
  }
  out.close();

  File file = LittleFS.open(path, FILE_READ);
  server_.sendHeader("Content-Disposition", attachmentHeader(exportDownloadName("unterbrechungen", time_)));
  server_.streamFile(file, "text/csv");
  file.close();
  LittleFS.remove(path);
}

void WebService::exportArchiveCsv() {
  if (!storage_ || !storage_->archiveReady()) {
    server_.send(503, "text/plain", "Archive unavailable");
    return;
  }

  server_.sendHeader("Content-Disposition", attachmentHeader(exportDownloadName("unterbrechungen_langzeit", time_)));
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/csv; charset=utf-8", "Datum;Zeit;Unixzeit\r\n");

  constexpr uint32_t BLOCK = 128;
  uint32_t epochs[BLOCK];
  const uint32_t total = storage_->archiveCount();

  for (uint32_t start = 0; start < total; start += BLOCK) {
    uint32_t readCount = 0;
    if (!storage_->readArchiveChunk(start, BLOCK, epochs, readCount)) break;

    String chunk;
    chunk.reserve(readCount * 32);
    for (uint32_t i = 0; i < readCount; i++) {
      time_t raw = static_cast<time_t>(epochs[i]);
      struct tm value = {};
      localtime_r(&raw, &value);
      char row[64];
      snprintf(row, sizeof(row), "%02d.%02d.%04d;%02d:%02d:%02d;%lu\r\n",
               value.tm_mday,
               value.tm_mon + 1,
               value.tm_year + 1900,
               value.tm_hour,
               value.tm_min,
               value.tm_sec,
               static_cast<unsigned long>(epochs[i]));
      chunk += row;
    }
    server_.sendContent(chunk);
    delay(0);
  }

  server_.sendContent("");
}

void WebService::exportAutarkCsv() {
  if (!storage_) {
    server_.send(503, "text/plain", "Storage unavailable");
    return;
  }

  const char* path = "/autark_export.csv";
  LittleFS.remove(path);
  File out = LittleFS.open(path, FILE_WRITE);
  if (!out) {
    server_.send(500, "text/plain", "Export failed");
    return;
  }

  out.print("Session;Typ;Seit_Start_s;Anker_Unixzeit\r\n");
  for (uint32_t i = 0; i < storage_->autarkCount(); i++) {
    AutarkRecord record = {};
    if (!storage_->readAutark(i, record)) continue;
    const char* type = record.type == static_cast<uint8_t>(AutarkRecordType::Start)
                         ? "Start"
                         : (record.type == static_cast<uint8_t>(AutarkRecordType::Event) ? "Event" : "End");
    out.printf("%lu;%s;%lu;%lu\r\n",
               static_cast<unsigned long>(record.sessionId),
               type,
               static_cast<unsigned long>(record.elapsedSec),
               static_cast<unsigned long>(record.anchorEpoch));
  }
  out.close();

  File file = LittleFS.open(path, FILE_READ);
  server_.sendHeader("Content-Disposition", attachmentHeader(exportDownloadName("autark", time_)));
  server_.streamFile(file, "text/csv");
  file.close();
  LittleFS.remove(path);
}

bool WebService::testNtp(const String& host) {
  IPAddress ip;
  if (WiFi.hostByName(host.c_str(), ip) != 1) return false;

  WiFiUDP udp;
  if (!udp.begin(2390)) return false;

  uint8_t packet[48] = {0};
  packet[0] = 0b11100011;
  const uint32_t started = millis();

  if (!udp.beginPacket(ip, 123)) {
    udp.stop();
    return false;
  }
  udp.write(packet, sizeof(packet));
  if (!udp.endPacket()) {
    udp.stop();
    return false;
  }

  while (millis() - started < 1800) {
    if (udp.parsePacket() >= 48) {
      udp.stop();
      return true;
    }
    delay(5);
  }

  udp.stop();
  return false;
}

void WebService::sendJsonError(int code, const char* error) {
  server_.send(code, "application/json", String("{\"ok\":false,\"error\":\"") + error + "\"}");
}
