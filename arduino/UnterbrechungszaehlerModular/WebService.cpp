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
  if (!enabled) { stop(); return; }
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
    server_.send_P(200, "text/html; charset=utf-8", WEB_UI);
  });
  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/events", HTTP_GET, [this]() { sendEvents(); });
  server_.on("/api/autark", HTTP_GET, [this]() { sendAutark(); });
  server_.on("/api/aggregate", HTTP_GET, [this]() { sendAggregate(); });
  server_.on("/api/add", HTTP_POST, [this]() {
    if (autark_ && autark_->active()) { sendJsonError(409, "autark_active"); return; }
    if (!counter_ || !counter_->addNormalEvent(false)) { sendJsonError(503, "event_not_stored"); return; }
    if (display_) display_->notifyActivity(false);
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/delete-last", HTTP_POST, [this]() {
    if (autark_ && autark_->active()) { sendJsonError(409, "autark_active"); return; }
    if (!counter_ || !counter_->deleteNormalEvent()) { sendJsonError(404, "no_event"); return; }
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/time", HTTP_POST, [this]() {
    if (!time_) { sendJsonError(500, "time_unavailable"); return; }
    const uint32_t epoch = strtoul(server_.arg("epoch").c_str(), nullptr, 10);
    if (time_->valid()) { server_.send(200, "application/json", "{\"ok\":true,\"accepted\":false}"); return; }
    if (!time_->setFromBrowser(epoch)) { sendJsonError(400, "invalid_time"); return; }
    server_.send(200, "application/json", "{\"ok\":true,\"accepted\":true}");
  });
  server_.on("/api/ntp", HTTP_POST, [this]() {
    if (!time_ || !network_ || !network_->connected()) { sendJsonError(503, "network_unavailable"); return; }
    String host = server_.arg("server"); host.trim();
    if (!time_->validNtpHost(host) || !testNtp(host) || !time_->setPrimaryNtp(host)) { sendJsonError(400, "invalid_ntp"); return; }
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/rtc-sync", HTTP_POST, [this]() {
    if (!rtc_ || !rtc_->present() || !rtc_->writeSystemTime()) { sendJsonError(409, "rtc_sync_failed"); return; }
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/display-test", HTTP_POST, [this]() {
    if (!display_ || !display_->showTest(autark_ && autark_->active())) { sendJsonError(404, "display_unavailable"); return; }
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/api/display-settings", HTTP_POST, [this]() {
    if (!display_ || !display_->present()) { sendJsonError(404, "display_unavailable"); return; }
    const int brightness = server_.arg("brightness").toInt();
    const int dimAfter = server_.arg("dimAfter").toInt();
    if (brightness < 1 || brightness > 255 || dimAfter < 5 || dimAfter > 3600) { sendJsonError(400, "invalid_display_settings"); return; }
    if (!display_->setSettings(static_cast<uint8_t>(brightness), static_cast<uint16_t>(dimAfter))) { sendJsonError(500, "display_settings_failed"); return; }
    server_.send(200, "application/json", "{\"ok\":true}");
  });
  server_.on("/export.csv", HTTP_GET, [this]() { exportNormalCsv(); });
  server_.on("/autark.csv", HTTP_GET, [this]() { exportAutarkCsv(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found"); });
}

void WebService::sendStatus() {
  const size_t fsTotal = storage_ ? storage_->fsTotalBytes() : 0;
  const size_t fsUsed = storage_ ? storage_->fsUsedBytes() : 0;
  String json;
  json.reserve(1500);
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
  json += ",\"ip\":\"" + (network_ ? network_->ip() : String("-")) + "\"";
  json += ",\"heapFree\":" + String(ESP.getFreeHeap());
  json += ",\"flashTotal\":" + String(ESP.getFlashChipSize());
  json += ",\"sketchUsed\":" + String(ESP.getSketchSize());
  json += ",\"rtcPresent\":" + String(rtc_ && rtc_->present() ? "true" : "false");
  json += ",\"rtcValid\":" + String(rtc_ && rtc_->timeValid() ? "true" : "false");
  json += ",\"rtcDate\":\"" + (rtc_ ? rtc_->dateText() : String("-")) + "\"";
  json += ",\"rtcTime\":\"" + (rtc_ ? rtc_->timeText() : String("-")) + "\"";
  json += ",\"displayPresent\":" + String(display_ && display_->present() ? "true" : "false");
  json += ",\"displayActive\":" + String(display_ && display_->active() ? "true" : "false");
  json += ",\"displayDimmed\":" + String(display_ && display_->dimmed() ? "true" : "false");
  json += ",\"displayBrightness\":" + String(display_ ? display_->brightness() : 0);
  json += ",\"displayDimAfter\":" + String(display_ ? display_->dimAfterSeconds() : 0);
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
  server_.send(200, "application/json", "{\"ok\":true,\"limit\":" + String(UicConfig::WEB_EVENT_LIMIT) + ",\"historyDays\":" + String(UicConfig::HISTORY_DAYS) + ",\"events\":[");
  bool first = true;
  if (storage_) {
    const uint32_t count = storage_->recentCount();
    uint32_t start = count > UicConfig::WEB_EVENT_LIMIT ? count - UicConfig::WEB_EVENT_LIMIT : 0;
    uint32_t cutoff = 0;
    if (time_ && time_->valid()) cutoff = time_->epoch() - static_cast<uint32_t>(UicConfig::HISTORY_DAYS) * 86400UL;

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
  if (!analytics_ || !storage_ || !analytics_->ensureBaseAggregates()) { sendJsonError(503, "aggregate_unavailable"); return; }

  time_t now = time(nullptr);
  struct tm value = {};
  localtime_r(&now, &value);
  int weekYear = server_.hasArg("weekYear") ? server_.arg("weekYear").toInt() : AnalyticsService::isoYear(value);
  int week = server_.hasArg("week") ? server_.arg("week").toInt() : AnalyticsService::isoWeek(value);
  int selectedYear = server_.hasArg("year") ? server_.arg("year").toInt() : analytics_->baseYear();
  if (!analytics_->ensureSelectedWeek(weekYear, week)) { sendJsonError(503, "week_unavailable"); return; }

  const int selectedOffset = analytics_->baseYear() - selectedYear;
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "application/json", "{\"ok\":true");
  server_.sendContent(",\"stored\":" + String(storage_->archiveCount()));
  server_.sendContent(",\"capacity\":" + String(storage_->archiveCapacity()));
  server_.sendContent(",\"baseYear\":" + String(analytics_->baseYear()));
  server_.sendContent(",\"selectedYear\":" + String(selectedYear));
  server_.sendContent(",\"selectedWeekYear\":" + String(weekYear));
  server_.sendContent(",\"selectedWeek\":" + String(week));
  server_.sendContent(",\"years\":[");
  bool first = true;
  for (uint8_t y = 0; y < UicConfig::LONGTERM_CACHE_YEARS; y++) {
    if (!analytics_->yearUsed(y) && y > 4) continue;
    if (!first) server_.sendContent(","); first = false;
    server_.sendContent(String(analytics_->baseYear() - y));
  }
  server_.sendContent("],\"weekdayHour\":[");
  for (uint8_t d = 0; d < 7; d++) {
    if (d) server_.sendContent(","); server_.sendContent("[");
    for (uint8_t h = 0; h < 24; h++) { if (h) server_.sendContent(","); server_.sendContent(String(analytics_->selectedWeekdayHour(d, h))); }
    server_.sendContent("]");
  }
  server_.sendContent("],\"monthWeek\":[");
  for (uint8_t m = 0; m < 12; m++) {
    if (m) server_.sendContent(","); server_.sendContent("[");
    for (uint8_t w = 0; w < 53; w++) { if (w) server_.sendContent(","); server_.sendContent(String(selectedOffset >= 0 && selectedOffset < UicConfig::LONGTERM_CACHE_YEARS ? analytics_->monthWeek(selectedOffset, m, w) : 0)); }
    server_.sendContent("]");
  }
  server_.sendContent("],\"yearMonth\":[");
  for (uint8_t y = 0; y < UicConfig::LONGTERM_CACHE_YEARS; y++) {
    if (y) server_.sendContent(","); server_.sendContent("[");
    for (uint8_t m = 0; m < 12; m++) { if (m) server_.sendContent(","); server_.sendContent(String(analytics_->yearMonth(y, m))); }
    server_.sendContent("]");
  }
  server_.sendContent("]}");
}

void WebService::exportNormalCsv() {
  if (!storage_) { server_.send(503, "text/plain", "Storage unavailable"); return; }
  const char* path = "/export.csv";
  LittleFS.remove(path);
  File out = LittleFS.open(path, FILE_WRITE);
  if (!out) { server_.send(500, "text/plain", "Export failed"); return; }
  out.print("Datum;Zeit;Unixzeit\r\n");
  for (uint32_t i = 0; i < storage_->recentCount(); i++) {
    uint32_t epoch = 0; if (!storage_->readRecent(i, epoch)) continue;
    time_t raw = static_cast<time_t>(epoch); struct tm value = {}; localtime_r(&raw, &value);
    out.printf("%02d.%02d.%04d;%02d:%02d:%02d;%lu\r\n", value.tm_mday, value.tm_mon + 1, value.tm_year + 1900, value.tm_hour, value.tm_min, value.tm_sec, static_cast<unsigned long>(epoch));
  }
  out.close();
  File file = LittleFS.open(path, FILE_READ);
  server_.sendHeader("Content-Disposition", "attachment; filename=\"unterbrechungen.csv\"");
  server_.streamFile(file, "text/csv"); file.close(); LittleFS.remove(path);
}

void WebService::exportAutarkCsv() {
  if (!storage_) { server_.send(503, "text/plain", "Storage unavailable"); return; }
  const char* path = "/autark_export.csv";
  LittleFS.remove(path);
  File out = LittleFS.open(path, FILE_WRITE);
  if (!out) { server_.send(500, "text/plain", "Export failed"); return; }
  out.print("Session;Typ;Seit_Start_s;Anker_Unixzeit\r\n");
  for (uint32_t i = 0; i < storage_->autarkCount(); i++) {
    AutarkRecord r = {}; if (!storage_->readAutark(i, r)) continue;
    const char* type = r.type == static_cast<uint8_t>(AutarkRecordType::Start) ? "Start" : (r.type == static_cast<uint8_t>(AutarkRecordType::Event) ? "Event" : "End");
    out.printf("%lu;%s;%lu;%lu\r\n", static_cast<unsigned long>(r.sessionId), type, static_cast<unsigned long>(r.elapsedSec), static_cast<unsigned long>(r.anchorEpoch));
  }
  out.close();
  File file = LittleFS.open(path, FILE_READ);
  server_.sendHeader("Content-Disposition", "attachment; filename=\"autark.csv\"");
  server_.streamFile(file, "text/csv"); file.close(); LittleFS.remove(path);
}

bool WebService::testNtp(const String& host) {
  IPAddress ip;
  if (WiFi.hostByName(host.c_str(), ip) != 1) return false;
  WiFiUDP udp;
  if (!udp.begin(2390)) return false;
  uint8_t packet[48] = {0}; packet[0] = 0b11100011;
  const uint32_t started = millis();
  if (!udp.beginPacket(ip, 123)) { udp.stop(); return false; }
  udp.write(packet, sizeof(packet));
  if (!udp.endPacket()) { udp.stop(); return false; }
  while (millis() - started < 1800) {
    if (udp.parsePacket() >= 48) { udp.stop(); return true; }
    delay(5);
  }
  udp.stop(); return false;
}

void WebService::sendJsonError(int code, const char* error) {
  server_.send(code, "application/json", String("{\"ok\":false,\"error\":\"") + error + "\"}");
}
