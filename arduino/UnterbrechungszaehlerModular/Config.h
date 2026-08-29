#pragma once

#include <Arduino.h>

namespace UicConfig {

static constexpr char APP_VERSION[] = "2026-08-29-reboot-8";
static constexpr char HOSTNAME[] = "unterbrechungen";
static constexpr char FALLBACK_AP_SSID[] = "Unterbrechungszaehler";
static constexpr char TZ_INFO[] = "CET-1CEST,M3.5.0,M10.5.0/3";
static constexpr char DEFAULT_NTP_1[] = "pool.ntp.org";
static constexpr char NTP_2[] = "time.cloudflare.com";
static constexpr char NTP_3[] = "time.google.com";

static constexpr uint8_t BUTTON_PIN = 27;
static constexpr uint8_t LED_PIN = 2;
static constexpr uint8_t AUTARK_PIN = 33;
static constexpr uint8_t I2C_SDA = 21;
static constexpr uint8_t I2C_SCL = 22;
static constexpr bool LED_ACTIVE_LOW = false;

static constexpr uint32_t DEBOUNCE_MS = 50;
static constexpr uint32_t AUTARK_SWITCH_DEBOUNCE_MS = 80;
static constexpr uint32_t LONG_PRESS_MS = 3000;
static constexpr uint32_t WIFI_RETRY_MS = 10000;
static constexpr uint32_t DISPLAY_BOOT_MS = 15000;
static constexpr uint32_t DIAGNOSTIC_INTERVAL_MS = 60000;

// Die Weboberflaeche arbeitet bewusst nur mit einem begrenzten Rohdatenfenster.
// Export und Langzeit-Heatmaps bleiben davon unberuehrt.
static constexpr uint16_t HISTORY_DAYS = 30;
static constexpr uint16_t WEB_EVENT_LIMIT = 5000;

static constexpr uint32_t VALID_TIME_MIN = 1700000000UL;
static constexpr uint32_t VALID_TIME_MAX = 4102444800UL;

static constexpr char RECENT_FILE[] = "/events.bin";
static constexpr char LEGACY_FILE[] = "/events.txt";
static constexpr uint32_t RECENT_MAGIC = 0x55494331;
static constexpr uint16_t RECENT_VERSION = 1;
static constexpr uint32_t RECENT_CAPACITY = 10000;

static constexpr char AUTARK_FILE[] = "/autark.bin";
static constexpr uint32_t AUTARK_MAGIC = 0x55494131;
static constexpr uint16_t AUTARK_VERSION = 1;
static constexpr uint32_t AUTARK_CAPACITY = 10000;

static constexpr char LONGTERM_FILE[] = "/events_10y.bin";
static constexpr uint32_t LONGTERM_MAGIC = 0x55494C31;
static constexpr uint16_t LONGTERM_VERSION = 1;
static constexpr uint32_t LONGTERM_CAPACITY = 100000;
static constexpr uint8_t LONGTERM_CACHE_YEARS = 16;

// Persistenter Statistik-Cache fuer die Heatmaps. Die Rohdaten bleiben die
// Quelle der Wahrheit; diese Datei darf jederzeit verworfen und neu aufgebaut
// werden. Dadurch ist der Heatmap-Start nach einem Neustart sofort moeglich.
static constexpr char ANALYTICS_CACHE_FILE[] = "/analytics.bin";
static constexpr char ANALYTICS_CACHE_TMP_FILE[] = "/analytics.tmp";
static constexpr uint32_t ANALYTICS_CACHE_MAGIC = 0x55494147;
static constexpr uint16_t ANALYTICS_CACHE_VERSION = 1;

static constexpr uint8_t RTC_ADDRESS = 0x68;
static constexpr uint8_t OLED_ADDRESS_1 = 0x3C;
static constexpr uint8_t OLED_ADDRESS_2 = 0x3D;

}  // namespace UicConfig

enum class AppMode : uint8_t {
  Normal = 0,
  Autark = 1
};

enum class TimeSource : uint8_t {
  None = 0,
  Rtc = 1,
  Browser = 2,
  Ntp = 3
};

inline const char* timeSourceName(TimeSource source) {
  switch (source) {
    case TimeSource::Rtc: return "rtc";
    case TimeSource::Browser: return "browser";
    case TimeSource::Ntp: return "ntp";
    default: return "none";
  }
}
