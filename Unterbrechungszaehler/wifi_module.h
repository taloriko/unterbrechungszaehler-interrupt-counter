#pragma once

#include <Arduino.h>

// Owns all Wi-Fi behavior for the base firmware. Other modules consume this
// small interface instead of depending directly on WiFi configuration details.
namespace WifiModule {

enum class Mode : uint8_t {
  Disabled,
  Connecting,
  Station,
  AccessPoint,
  Disconnected,
  Error
};

// Starts Wi-Fi without waiting for the station connection. The return value is
// true only when a usable interface is already available immediately (for
// example the fallback AP when no STA credentials exist).
bool begin();

// Cooperative maintenance. Call from loop(); station timeout, AP fallback,
// reconnect handling and the 15 s status log are all millis()-driven.
void update();

bool stationConnected();
bool accessPointActive();
// Becomes true once the initial STA attempt has either connected or reached its
// configured timeout. TimeService uses this to defer its one boot NTP probe.
bool startupSettled();
Mode mode();
const char *modeName();
const char *stateName();

String ssid();
String ip();
int32_t rssi();
uint8_t accessPointClients();

// Useful for boot diagnostics or explicit module diagnostics.
void logStatusNow();

}  // namespace WifiModule
