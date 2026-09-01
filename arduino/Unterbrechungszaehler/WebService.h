#pragma once

#include <Arduino.h>
#include <WebServer.h>

class StorageService;
class TimeService;
class NetworkService;
class CounterService;
class AutarkService;
class RtcService;
class DisplayService;
class AnalyticsService;
class SoundService;
class WatchdogService;

class WebService {
public:
  WebService();
  void begin(StorageService* storage,
             TimeService* time,
             NetworkService* network,
             CounterService* counter,
             AutarkService* autark,
             RtcService* rtc,
             DisplayService* display,
             AnalyticsService* analytics,
             SoundService* sound,
             WatchdogService* watchdog);
  void tick(bool enabled);
  void stop();
  bool started() const { return started_; }
  uint32_t lastSlowRequestUs() const { return lastSlowRequestUs_; }
  uint32_t lastSlowRequestAt() const { return lastSlowRequestAt_; }
  const String& lastSlowRequest() const { return lastSlowRequest_; }

private:
  void registerRoutes();
  void sendStatus();
  void sendEvents();
  void sendAutark();
  void sendAggregate();
  void sendDisplayPreview();
  void sendTrackList();
  void saveTrackList();
  void exportNormalCsv();
  void exportArchiveCsv();
  void exportAutarkCsv();
  bool testNtp(const String& host);
  void sendJsonError(int code, const char* error);
  void startIfNeeded();

  WebServer server_;
  StorageService* storage_ = nullptr;
  TimeService* time_ = nullptr;
  NetworkService* network_ = nullptr;
  CounterService* counter_ = nullptr;
  AutarkService* autark_ = nullptr;
  RtcService* rtc_ = nullptr;
  DisplayService* display_ = nullptr;
  AnalyticsService* analytics_ = nullptr;
  SoundService* sound_ = nullptr;
  WatchdogService* watchdog_ = nullptr;
  bool routesRegistered_ = false;
  bool started_ = false;
  bool updateOk_ = false;
  uint32_t restartAt_ = 0;
  uint32_t lastSlowRequestUs_ = 0;
  uint32_t lastSlowRequestAt_ = 0;
  String lastSlowRequest_ = "-";
};
