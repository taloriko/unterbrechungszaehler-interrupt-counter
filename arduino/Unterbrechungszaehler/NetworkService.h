#pragma once

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

class NetworkService {
public:
  void begin();
  void tick(bool allowNetwork);
  void stop();

  bool connected() const { return WiFi.status() == WL_CONNECTED; }
  bool accessPointActive() const { return accessPointActive_; }
  bool running() const { return running_; }
  String ip() const;
  int32_t rssi() const { return connected() ? WiFi.RSSI() : 0; }
  String configuredSsid() const { return ssid_; }
  bool credentialsStored() const { return credentialsStored_; }

private:
  void startFallbackAccessPoint();
  void stopFallbackAccessPoint();
  void startMdns();
  void loadCredentials();
  bool importEspWifiCredentials();
  bool storeCredentials(const String& ssid, const String& password);
  bool validCompileTimeCredentials() const;
  void connectStation();

  bool running_ = false;
  bool accessPointActive_ = false;
  bool mdnsActive_ = false;
  bool credentialsStored_ = false;
  uint32_t lastRetryAt_ = 0;
  String ssid_;
  String password_;
};
