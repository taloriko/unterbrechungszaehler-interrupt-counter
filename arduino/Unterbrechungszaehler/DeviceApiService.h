#pragma once

#include <Arduino.h>
#include <WebServer.h>

class RtcService;
class TimeService;
class DisplayService;
class SoundService;
class WatchdogService;

class DeviceApiService {
public:
  void begin(WebServer* server,
             RtcService* rtc,
             TimeService* time,
             DisplayService* display,
             SoundService* sound,
             WatchdogService* watchdog);

private:
  void registerRoutes();
  void sendHardwareStatus();
  void sendJsonError(int code, const char* error);

  WebServer* server_ = nullptr;
  RtcService* rtc_ = nullptr;
  TimeService* time_ = nullptr;
  DisplayService* display_ = nullptr;
  SoundService* sound_ = nullptr;
  WatchdogService* watchdog_ = nullptr;
  bool routesRegistered_ = false;
};
