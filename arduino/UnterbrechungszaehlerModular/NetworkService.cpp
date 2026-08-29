#include "NetworkService.h"

#include "Config.h"
#include "Secrets.h"

void NetworkService::begin() {
  if (running_) return;
  running_ = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(UicConfig::HOSTNAME);
  WiFi.setAutoReconnect(true);
  startFallbackAccessPoint();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastRetryAt_ = millis();
}

void NetworkService::tick(bool allowNetwork) {
  if (!allowNetwork) {
    if (running_) stop();
    return;
  }

  if (!running_) begin();

  if (connected()) {
    if (accessPointActive_) stopFallbackAccessPoint();
    if (!mdnsActive_) startMdns();
    return;
  }

  if (mdnsActive_) {
    MDNS.end();
    mdnsActive_ = false;
  }

  if (!accessPointActive_) startFallbackAccessPoint();
  if (millis() - lastRetryAt_ < UicConfig::WIFI_RETRY_MS) return;

  lastRetryAt_ = millis();
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void NetworkService::stop() {
  if (!running_) return;
  if (mdnsActive_) {
    MDNS.end();
    mdnsActive_ = false;
  }
  if (accessPointActive_) {
    WiFi.softAPdisconnect(true);
    accessPointActive_ = false;
  }
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  running_ = false;
}

String NetworkService::ip() const {
  if (connected()) return WiFi.localIP().toString();
  if (accessPointActive_) return WiFi.softAPIP().toString();
  return "-";
}

void NetworkService::startFallbackAccessPoint() {
  if (accessPointActive_) return;

  const IPAddress ip(192, 168, 4, 1);
  const IPAddress gateway(192, 168, 4, 1);
  const IPAddress subnet(255, 255, 255, 0);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip, gateway, subnet);
  accessPointActive_ = WiFi.softAP(UicConfig::FALLBACK_AP_SSID);
}

void NetworkService::stopFallbackAccessPoint() {
  WiFi.softAPdisconnect(true);
  accessPointActive_ = false;
  if (connected()) WiFi.mode(WIFI_STA);
}

void NetworkService::startMdns() {
  if (!connected()) return;
  if (MDNS.begin(UicConfig::HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsActive_ = true;
  }
}
