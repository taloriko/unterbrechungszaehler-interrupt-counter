#include "DeviceApiService.h"

#include <math.h>

#include "DisplayService.h"
#include "RtcService.h"
#include "SoundService.h"
#include "TimeService.h"
#include "WatchdogService.h"

void DeviceApiService::begin(WebServer* server,
                             RtcService* rtc,
                             TimeService* time,
                             DisplayService* display,
                             SoundService* sound,
                             WatchdogService* watchdog) {
  server_ = server;
  rtc_ = rtc;
  time_ = time;
  display_ = display;
  sound_ = sound;
  watchdog_ = watchdog;
  registerRoutes();
}

void DeviceApiService::registerRoutes() {
  if (routesRegistered_ || !server_) return;
  routesRegistered_ = true;

  server_->on("/api/device-hardware", HTTP_GET, [this]() {
    sendHardwareStatus();
  });

  server_->on("/api/rtc-check", HTTP_POST, [this]() {
    if (!rtc_) {
      sendJsonError(503, "rtc_unavailable");
      return;
    }
    rtc_->checkNow();
    server_->send(200, "application/json",
                  String("{\"ok\":true,\"detected\":") +
                  (rtc_->present() ? "true" : "false") + "}");
  });

  server_->on("/api/rtc-from-system", HTTP_POST, [this]() {
    if (!rtc_ || !time_ || !time_->valid() || !rtc_->writeSystemTime()) {
      sendJsonError(409, "rtc_write_failed");
      return;
    }
    server_->send(200, "application/json", "{\"ok\":true}");
  });

  server_->on("/api/rtc-to-system", HTTP_POST, [this]() {
    if (!time_ || !time_->setFromRtc()) {
      sendJsonError(409, "rtc_read_failed");
      return;
    }
    server_->send(200, "application/json", "{\"ok\":true}");
  });

  server_->on("/api/sound-check", HTTP_POST, [this]() {
    if (!sound_) {
      sendJsonError(503, "sound_unavailable");
      return;
    }
    if (!sound_->requestHardwareCheck()) {
      sendJsonError(409, "sound_check_busy");
      return;
    }
    server_->send(202, "application/json", "{\"ok\":true,\"checking\":true}");
  });

  server_->on("/api/watchdog-reset-max", HTTP_POST, [this]() {
    if (!watchdog_) {
      sendJsonError(503, "watchdog_unavailable");
      return;
    }
    watchdog_->resetMaximums();
    server_->send(200, "application/json", "{\"ok\":true}");
  });
}

void DeviceApiService::sendHardwareStatus() {
  if (!server_) return;

  String json;
  json.reserve(950);
  json = "{\"ok\":true";

  json += ",\"system\":{";
  json += "\"date\":\"" + (time_ ? time_->localDate() : String("-")) + "\"";
  json += ",\"time\":\"" + (time_ ? time_->localTime() : String("-")) + "\"";
  json += ",\"source\":\"" + String(time_ ? timeSourceName(time_->source()) : "none") + "\"}";

  json += ",\"rtc\":{";
  json += "\"detected\":" + String(rtc_ && rtc_->present() ? "true" : "false");
  json += ",\"communicationOk\":" + String(rtc_ && rtc_->communicationOk() ? "true" : "false");
  json += ",\"timeValid\":" + String(rtc_ && rtc_->timeValid() ? "true" : "false");
  json += ",\"osf\":" + String(rtc_ && rtc_->oscillatorStopFlag() ? "true" : "false");
  json += ",\"date\":\"" + (rtc_ ? rtc_->dateText() : String("-")) + "\"";
  json += ",\"time\":\"" + (rtc_ ? rtc_->timeText() : String("-")) + "\"";
  json += ",\"checkAgeMs\":" + String(rtc_ ? rtc_->lastCheckAgeMs() : 0xFFFFFFFFUL);
  if (rtc_ && !isnan(rtc_->temperatureC())) {
    json += ",\"temperatureC\":" + String(rtc_->temperatureC(), 2);
  } else {
    json += ",\"temperatureC\":null";
  }
  json += "}";

  json += ",\"display\":{";
  json += "\"detected\":" + String(display_ && display_->present() ? "true" : "false");
  json += ",\"simulation\":" + String(display_ && display_->simulationEnabled() ? "true" : "false");
  json += ",\"active\":" + String(display_ && display_->active() ? "true" : "false");
  json += ",\"address\":" + String(display_ ? display_->address() : 0) + "}";

  json += ",\"sound\":{";
  json += "\"detected\":" + String(sound_ && sound_->present() ? "true" : "false");
  json += ",\"checking\":" + String(sound_ && sound_->hardwareCheckActive() ? "true" : "false");
  json += ",\"checkAgeMs\":" + String(sound_ ? sound_->hardwareCheckAgeMs() : 0xFFFFFFFFUL);
  json += ",\"enabled\":" + String(sound_ && sound_->enabled() ? "true" : "false");
  json += ",\"busy\":" + String(sound_ && sound_->busy() ? "true" : "false");
  json += ",\"state\":\"" + String(sound_ ? soundHardwareStateName(sound_->hardwareState()) : "unavailable") + "\"}";

  json += "}";
  server_->sendHeader("Cache-Control", "no-store");
  server_->send(200, "application/json", json);
}

void DeviceApiService::sendJsonError(int code, const char* error) {
  if (!server_) return;
  server_->send(code,
                "application/json",
                String("{\"ok\":false,\"error\":\"") + error + "\"}");
}
