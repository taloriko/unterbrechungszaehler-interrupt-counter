#pragma once

#include <Arduino.h>

// Central project configuration. Change values here instead of scattering
// project metadata through firmware and frontend code.
namespace AppConfig {

constexpr char PROJECT_NAME[] = "Unterbrechungszähler";
constexpr char PROJECT_ICON[] = "interrupt";  // Must match an icon id in ui-src/index.html.
constexpr char SOFTWARE_VERSION[] = "3.2.0";
constexpr char FIRMWARE_NAME[] = "Unterbrechungszaehler";
constexpr char BOARD_NAME[] = "ESP32 Dev Module";
constexpr char FOOTER_COMMENT[] = "Gemacht, aus dem Schmerz herraus...";

// Optional footer values. Leave empty to hide the corresponding element.
constexpr char GITHUB_USER[] = "taloriko";
constexpr char GITHUB_USER_URL[] = "https://github.com/taloriko";
constexpr char GITHUB_PROJECT_URL[] = "https://github.com/taloriko/unterbrechungszaehler-interrupt-counter";

// Replace these placeholders before uploading. They are intentionally kept in
// one place and are never written to flash by the web UI.
constexpr char WIFI_SSID[] = "WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "WIWI_PASSWORD";
constexpr char FALLBACK_AP_PASSWORD[] = "Unterbrechungszähler";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_STATUS_LOG_INTERVAL_MS = 15000;
constexpr uint32_t WIFI_STATE_CHECK_INTERVAL_MS = 500;
constexpr uint32_t WIFI_AP_RETRY_INTERVAL_MS = 15000;

constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr size_t STATUS_PROVIDER_CAPACITY = 16;
constexpr uint32_t OTA_RESTART_DELAY_MS = 1800;

// Central time policy. NTP is queried only at boot and on an explicit user
// check; this base intentionally does not run a periodic SNTP client.
constexpr char DEFAULT_NTP_SERVER[] = "fritz.box";
constexpr uint16_t NTP_LOCAL_UDP_PORT = 2390;
constexpr uint32_t NTP_RESPONSE_TIMEOUT_MS = 1500;
constexpr size_t NTP_SERVER_MAX_LENGTH = 96;
constexpr uint16_t TIME_MIN_VALID_YEAR = 2025;
constexpr uint16_t TIME_MAX_VALID_YEAR = 2099;
constexpr bool RTC_SYNC_FROM_NTP_ENABLED = true;
constexpr bool BROWSER_TIME_FALLBACK_ENABLED = true;

constexpr char FALLBACK_LANGUAGE[] = "en";
constexpr char AVAILABLE_LANGUAGES_JSON[] = "[\"de\",\"en\",\"it\",\"fr\",\"swg\",\"swg-alb\",\"swg-ob\"]";
constexpr char DEFAULT_THEME[] = "system";

// The generated asset ETag guarantees that a firmware with a changed web UI
// invalidates the browser cache without a service worker.
constexpr char WEB_CACHE_CONTROL[] = "public, max-age=0, must-revalidate";

}  // namespace AppConfig
