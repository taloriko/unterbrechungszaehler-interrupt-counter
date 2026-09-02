#include "wifi_module.h"

#include <Preferences.h>
#include <WiFi.h>
#include <cstring>

#include "config.h"
#include "serial_log.h"
#include "status_registry.h"

namespace WifiModule {
namespace {

constexpr char WIFI_PREF_NAMESPACE[] = "espui-wifi";
constexpr char WIFI_PREF_SSID[] = "ssid";
constexpr char WIFI_PREF_PASSWORD[] = "pass";

bool credentialsAvailable = false;
bool apActive = false;
bool startupInterfaceAvailable = false;
bool startupSettledFlag = false;
String stationSsid;
String stationPassword;
uint32_t stationAttemptStartedMs = 0;
uint32_t disconnectedSinceMs = 0;
uint32_t lastStateCheckMs = 0;
uint32_t lastStatusLogMs = 0;
uint32_t lastFallbackAttemptMs = 0;

bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(now - since) >= interval;
}

bool wifiCredentialsConfigured() {
  return std::strcmp(AppConfig::WIFI_SSID, "WIFI_SSID") != 0 && AppConfig::WIFI_SSID[0] != '\0';
}

bool loadStationCredentials() {
  stationSsid = "";
  stationPassword = "";

  const bool compileTimeCredentials = wifiCredentialsConfigured();
  Preferences preferences;
  const bool storageReady = preferences.begin(WIFI_PREF_NAMESPACE, false);

  if (compileTimeCredentials) {
    stationSsid = AppConfig::WIFI_SSID;
    stationPassword = AppConfig::WIFI_PASSWORD;

    if (storageReady) {
      const String storedSsid = preferences.getString(WIFI_PREF_SSID, "");
      const String storedPassword = preferences.getString(WIFI_PREF_PASSWORD, "");
      if (storedSsid != stationSsid || storedPassword != stationPassword) {
        const size_t ssidBytes = preferences.putString(WIFI_PREF_SSID, stationSsid);
        const size_t passwordBytes = preferences.putString(WIFI_PREF_PASSWORD, stationPassword);
        if (ssidBytes > 0 && (stationPassword.length() == 0 || passwordBytes > 0)) {
          SerialLog::info("WIFI", "Station credentials saved/updated in NVS");
        } else {
          SerialLog::warning("WIFI", "Station credentials are usable for this boot but could not be persisted in NVS");
        }
      }
    } else {
      SerialLog::warning("WIFI", "NVS credential store unavailable; using compile-time Wi-Fi credentials for this boot");
    }
  } else if (storageReady) {
    stationSsid = preferences.getString(WIFI_PREF_SSID, "");
    stationPassword = preferences.getString(WIFI_PREF_PASSWORD, "");
    if (stationSsid.length()) {
      SerialLog::infof("WIFI", "Using stored station credentials from NVS | SSID=%s", stationSsid.c_str());
    }
  }

  if (storageReady) preferences.end();
  return stationSsid.length() > 0;
}

void makeAccessPointName(char *target, size_t targetSize) {
  if (!target || targetSize == 0) return;

  // IEEE 802.11 SSIDs are limited to 32 bytes. Copy only the configured bytes
  // that fit and never cut through a UTF-8 continuation sequence.
  constexpr size_t MAX_SSID_BYTES = 32;
  const size_t capacity = (targetSize - 1 < MAX_SSID_BYTES) ? targetSize - 1 : MAX_SSID_BYTES;
  const size_t sourceLength = std::strlen(AppConfig::PROJECT_NAME);
  size_t copyLength = sourceLength < capacity ? sourceLength : capacity;
  if (copyLength < sourceLength) {
    while (copyLength > 0 &&
           (static_cast<uint8_t>(AppConfig::PROJECT_NAME[copyLength]) & 0xC0U) == 0x80U) {
      --copyLength;
    }
  }
  std::memcpy(target, AppConfig::PROJECT_NAME, copyLength);
  target[copyLength] = '\0';
}

bool startFallbackAccessPoint() {
  char apName[33];
  makeAccessPointName(apName, sizeof(apName));

  // If credentials exist, AP+STA keeps automatic station reconnection alive.
  // Without credentials, AP-only avoids running an unused station interface.
  WiFi.mode(credentialsAvailable ? WIFI_AP_STA : WIFI_AP);
  const bool started = WiFi.softAP(apName, AppConfig::FALLBACK_AP_PASSWORD);
  lastFallbackAttemptMs = millis();

  if (!started) {
    apActive = false;
    SerialLog::errorf("WIFI", "Fallback AP could not be started | SSID=%s", apName);
    return false;
  }

  apActive = true;
  startupInterfaceAvailable = true;
  const String apIp = WiFi.softAPIP().toString();
  SerialLog::warningf("WIFI", "Fallback AP active | SSID=%s | password protected | IP=%s",
                      apName, apIp.c_str());
  return true;
}

void stopFallbackAccessPoint() {
  if (!apActive) return;

  WiFi.softAPdisconnect(false);
  apActive = false;
  WiFi.mode(WIFI_STA);
  const String currentIp = WiFi.localIP().toString();
  SerialLog::successf("WIFI", "Station reconnected | fallback AP stopped | IP=%s | RSSI=%ld dBm",
                      currentIp.c_str(), static_cast<long>(WiFi.RSSI()));
  lastStatusLogMs = millis();
}

void logStationStatus() {
  const String currentSsid = WiFi.SSID();
  const String currentIp = WiFi.localIP().toString();
  SerialLog::infof("WIFI", "Status: connected | SSID=%s | IP=%s | RSSI=%ld dBm",
                   currentSsid.c_str(), currentIp.c_str(), static_cast<long>(WiFi.RSSI()));
}

void logAccessPointStatus() {
  const String currentSsid = WiFi.softAPSSID();
  const String currentIp = WiFi.softAPIP().toString();
  SerialLog::infof("WIFI", "Status: station not connected | AP active | SSID=%s | IP=%s | clients=%u | RSSI=n/a",
                   currentSsid.c_str(), currentIp.c_str(),
                   static_cast<unsigned int>(WiFi.softAPgetStationNum()));
}

void syncStatusProvider() {
  if (stationConnected()) StatusRegistry::setState("wifi", StatusRegistry::State::Ok);
  else if (apActive) StatusRegistry::setState("wifi", StatusRegistry::State::AccessPoint);
  else if (credentialsAvailable && !startupSettledFlag) StatusRegistry::setState("wifi", StatusRegistry::State::Checking);
  else if (credentialsAvailable) StatusRegistry::setState("wifi", StatusRegistry::State::Disconnected);
  else StatusRegistry::setState("wifi", StatusRegistry::State::Error);
}

void onStationConnected(bool initialConnection) {
  startupSettledFlag = true;
  startupInterfaceAvailable = true;
  disconnectedSinceMs = 0;

  if (apActive) {
    stopFallbackAccessPoint();
    return;
  }

  if (initialConnection) {
    const String currentIp = WiFi.localIP().toString();
    SerialLog::successf("WIFI", "Connected | IP=%s | RSSI=%ld dBm",
                        currentIp.c_str(), static_cast<long>(WiFi.RSSI()));
    lastStatusLogMs = millis();
  }
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("wifi", "status.wifi", "wifi", true);

  credentialsAvailable = false;
  apActive = false;
  startupInterfaceAvailable = false;
  startupSettledFlag = false;
  stationAttemptStartedMs = 0;
  disconnectedSinceMs = 0;
  lastStateCheckMs = 0;
  lastStatusLogMs = millis();
  lastFallbackAttemptMs = 0;

  // Keep Wi-Fi driver writes in RAM; our tiny Preferences record is updated only
  // when configured credentials actually change. OTA does not touch this NVS data.
  WiFi.persistent(false);
  credentialsAvailable = loadStationCredentials();

  if (!credentialsAvailable) {
    SerialLog::warning("WIFI", "No station credentials available; starting fallback AP");
    const bool started = startFallbackAccessPoint();
    startupSettledFlag = true;
    syncStatusProvider();
    return started;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  SerialLog::infof("WIFI", "Connecting to station network asynchronously | SSID=%s", stationSsid.c_str());
  stationAttemptStartedMs = millis();
  disconnectedSinceMs = stationAttemptStartedMs;
  WiFi.begin(stationSsid.c_str(), stationPassword.c_str());
  syncStatusProvider();
  return false;
}

void update() {
  const uint32_t now = millis();

  if (elapsed(now, lastStateCheckMs, AppConfig::WIFI_STATE_CHECK_INTERVAL_MS)) {
    lastStateCheckMs = now;
    const bool connected = stationConnected();

    if (connected) {
      const bool initialConnection = !startupSettledFlag;
      onStationConnected(initialConnection);
    } else if (credentialsAvailable) {
      if (!startupSettledFlag) {
        if (elapsed(now, stationAttemptStartedMs, AppConfig::WIFI_CONNECT_TIMEOUT_MS)) {
          startupSettledFlag = true;
          SerialLog::warningf("WIFI", "Station connection failed after %lu ms; starting fallback AP",
                              static_cast<unsigned long>(AppConfig::WIFI_CONNECT_TIMEOUT_MS));
          const bool retryDue = lastFallbackAttemptMs == 0 ||
                                elapsed(now, lastFallbackAttemptMs, AppConfig::WIFI_AP_RETRY_INTERVAL_MS);
          if (retryDue) startFallbackAccessPoint();
        }
      } else {
        if (disconnectedSinceMs == 0) {
          disconnectedSinceMs = now;
          SerialLog::warning("WIFI", "Station connection lost; waiting for automatic reconnect");
        }

        const bool fallbackDue = elapsed(now, disconnectedSinceMs, AppConfig::WIFI_CONNECT_TIMEOUT_MS);
        const bool retryDue = lastFallbackAttemptMs == 0 ||
                              elapsed(now, lastFallbackAttemptMs, AppConfig::WIFI_AP_RETRY_INTERVAL_MS);
        if (!apActive && fallbackDue && retryDue) startFallbackAccessPoint();
      }
    } else if (!apActive) {
      // AP-only projects have no station interface that could recover the
      // network for us. Retry a failed softAP start cooperatively instead of
      // leaving the device unreachable until the next reboot.
      const bool retryDue = lastFallbackAttemptMs == 0 ||
                            elapsed(now, lastFallbackAttemptMs, AppConfig::WIFI_AP_RETRY_INTERVAL_MS);
      if (retryDue) startFallbackAccessPoint();
    }
    syncStatusProvider();
  }

  if (elapsed(now, lastStatusLogMs, AppConfig::WIFI_STATUS_LOG_INTERVAL_MS)) {
    lastStatusLogMs = now;
    logStatusNow();
  }
}

bool stationConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool accessPointActive() {
  return apActive;
}

bool startupSettled() {
  return startupSettledFlag;
}

Mode mode() {
  if (stationConnected()) return Mode::Station;
  if (apActive) return Mode::AccessPoint;
  if (credentialsAvailable && !startupSettledFlag) return Mode::Connecting;
  if (credentialsAvailable) return Mode::Disconnected;
  if (!startupInterfaceAvailable) return Mode::Error;
  return Mode::Disabled;
}

const char *modeName() {
  switch (mode()) {
    case Mode::Station: return "station";
    case Mode::AccessPoint: return "ap";
    case Mode::Connecting: return "connecting";
    case Mode::Disconnected: return "disconnected";
    case Mode::Error: return "error";
    case Mode::Disabled:
    default: return "disabled";
  }
}

const char *stateName() {
  if (stationConnected()) return "ok";
  if (apActive) return "ap";
  if (credentialsAvailable && !startupSettledFlag) return "checking";
  if (credentialsAvailable) return "disconnected";
  return "error";
}

String ssid() {
  if (stationConnected()) return WiFi.SSID();
  if (apActive) return WiFi.softAPSSID();
  return stationSsid;
}

String ip() {
  if (stationConnected()) return WiFi.localIP().toString();
  if (apActive) return WiFi.softAPIP().toString();
  return String();
}

int32_t rssi() {
  return stationConnected() ? WiFi.RSSI() : 0;
}

uint8_t accessPointClients() {
  return apActive ? WiFi.softAPgetStationNum() : 0;
}

void logStatusNow() {
  if (stationConnected()) {
    logStationStatus();
    return;
  }
  if (apActive) {
    logAccessPointStatus();
    return;
  }
  if (credentialsAvailable && !startupSettledFlag) {
    SerialLog::infof("WIFI", "Status: connecting | SSID=%s | IP=pending | RSSI=n/a", stationSsid.c_str());
  } else if (credentialsAvailable) {
    SerialLog::warning("WIFI", "Status: station not connected | AP not active | RSSI=n/a");
  } else {
    SerialLog::error("WIFI", "Status: no network interface available | RSSI=n/a");
  }
}

}  // namespace WifiModule
