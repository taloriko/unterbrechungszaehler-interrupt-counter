#include "NetworkService.h"

#include <Preferences.h>
#include <esp_wifi.h>

#include "Config.h"
#include "Secrets.h"

namespace {
Preferences networkPrefs;

String configText(const uint8_t* data, size_t maxLength) {
  size_t length = 0;
  while (length < maxLength && data[length] != 0) length++;
  return String(reinterpret_cast<const char*>(data)).substring(0, length);
}
}

void NetworkService::begin() {
  if (running_) return;

  // STA einmal initialisieren, damit eine eventuell bereits im ESP32-WLAN-NVS
  // gespeicherte Konfiguration gelesen werden kann. Das ist wichtig fuer OTA:
  // neue Firmware darf nicht durch Platzhalter aus Secrets.example.h das bisherige WLAN ueberschreiben.
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(UicConfig::HOSTNAME);
  WiFi.setAutoReconnect(true);

  loadCredentials();

  running_ = true;
  WiFi.mode(WIFI_AP_STA);
  startFallbackAccessPoint();
  connectStation();
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
  WiFi.disconnect(false, false);
  connectStation();
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
  // eraseap=false: gespeicherte WLAN-Daten duerfen bei Autark/Neustart/OTA nicht geloescht werden.
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  running_ = false;
}

String NetworkService::ip() const {
  if (connected()) return WiFi.localIP().toString();
  if (accessPointActive_) return WiFi.softAPIP().toString();
  return "-";
}

void NetworkService::loadCredentials() {
  ssid_ = "";
  password_ = "";
  credentialsStored_ = false;

  if (networkPrefs.begin("uic-network", true)) {
    ssid_ = networkPrefs.getString("ssid", "");
    password_ = networkPrefs.getString("pass", "");
    networkPrefs.end();
  }

  if (ssid_.length()) {
    credentialsStored_ = true;
    Serial.printf("[WLAN] Persistente Konfiguration geladen: %s\n", ssid_.c_str());
    return;
  }

  // Bei der ersten 2.x-Firmware vorhandene ESP32-WLAN-Konfiguration uebernehmen.
  // Damit bleibt ein bereits verbundenes Geraet auch dann im WLAN, wenn die OTA-BIN
  // aus GitHub absichtlich keine privaten Secrets enthaelt.
  if (importEspWifiCredentials()) return;

  if (validCompileTimeCredentials()) {
    storeCredentials(String(WIFI_SSID), String(WIFI_PASSWORD));
    Serial.printf("[WLAN] Konfiguration aus Secrets.h uebernommen: %s\n", ssid_.c_str());
    return;
  }

  Serial.println("[WLAN] Keine gueltigen Zugangsdaten vorhanden - Fallback-AP bleibt aktiv");
}

bool NetworkService::importEspWifiCredentials() {
  wifi_config_t config = {};
  if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK) return false;

  const String currentSsid = configText(config.sta.ssid, sizeof(config.sta.ssid));
  const String currentPassword = configText(config.sta.password, sizeof(config.sta.password));
  if (!currentSsid.length() || currentSsid == "DEIN_WLAN") return false;

  if (!storeCredentials(currentSsid, currentPassword)) return false;
  Serial.printf("[WLAN] Bestehende ESP32-WLAN-Konfiguration fuer OTA uebernommen: %s\n", ssid_.c_str());
  return true;
}

bool NetworkService::storeCredentials(const String& ssid, const String& password) {
  if (!ssid.length()) return false;
  if (!networkPrefs.begin("uic-network", false)) return false;
  const size_t a = networkPrefs.putString("ssid", ssid);
  const size_t b = networkPrefs.putString("pass", password);
  networkPrefs.end();
  if (!a || (password.length() && !b)) return false;

  ssid_ = ssid;
  password_ = password;
  credentialsStored_ = true;
  return true;
}

bool NetworkService::validCompileTimeCredentials() const {
  if (!WIFI_SSID || !WIFI_PASSWORD) return false;
  const String ssid(WIFI_SSID);
  if (!ssid.length() || ssid == "DEIN_WLAN") return false;
  return true;
}

void NetworkService::connectStation() {
  if (!ssid_.length()) return;
  WiFi.begin(ssid_.c_str(), password_.c_str());
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
