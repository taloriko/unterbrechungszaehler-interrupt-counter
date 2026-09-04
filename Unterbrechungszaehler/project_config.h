#pragma once

#include <Arduino.h>

#include "project_preferences.h"

namespace ProjectConfig {

// The physical interruption button reuses the generic GPIO layer. DI1 is
// GPIO13, INPUT_PULLUP, active-low in hardware_config.h.
constexpr char INTERRUPTION_INPUT_ID[] = "di1";

// Device-local calendar rules for logging and statistics. Absolute timestamps
// remain UTC; this POSIX TZ is used only to derive local date/hour/week/month.
constexpr char TIMEZONE_NAME[] = "Europe/Berlin";
constexpr char TIMEZONE_POSIX[] = "CET-1CEST,M3.5.0,M10.5.0/3";

// Feedback is independent from persistence: the event is captured first, then
// display/audio/storage work is scheduled cooperatively.
constexpr uint16_t INTERRUPTION_SOUND_TRACK_DEFAULT = 2;
constexpr ProjectPreferences::SoundMode INTERRUPTION_SOUND_MODE_DEFAULT = ProjectPreferences::SoundMode::Rotate;
constexpr bool INTERRUPTION_SOUND_DEFAULT = true;
constexpr uint8_t SOUND_VOLUME_DEFAULT_PERCENT = 100;
constexpr ProjectPreferences::RadioMode RF_MODE_DEFAULT = ProjectPreferences::RadioMode::Universal433;
constexpr bool DISPLAY_ENABLED_DEFAULT = true;
constexpr bool DISPLAY_ROTATION_180_DEFAULT = false;
constexpr bool DISPLAY_FLASH_DEFAULT = true;
constexpr uint32_t DISPLAY_FLASH_MS = 220;

// OLED project preferences are persisted in NVS. Brightness values are percent
// and converted to the SH1106 0..255 contrast register only when needed.
constexpr ProjectPreferences::DisplayMode DISPLAY_MODE_DEFAULT = ProjectPreferences::DisplayMode::Standard;
constexpr uint8_t DISPLAY_BRIGHTNESS_DEFAULT_PERCENT = 65;
constexpr uint16_t DISPLAY_DIM_AFTER_DEFAULT_MINUTES = 10;
constexpr uint16_t DISPLAY_DIM_AFTER_MAX_MINUTES = 1440;
constexpr uint8_t DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT = 5;

// Raw binary ring: 100,000 * 9 bytes = 900,000 bytes.
constexpr uint32_t RAW_EVENT_CAPACITY = 100000;
constexpr uint8_t RAW_RECORD_SIZE = 9;

// 2,300 daily slots cover >6.2 years. Each record is 64 bytes. This preserves
// at least five complete years plus the current year independently of raw-ring
// overwrites.
constexpr uint16_t DAILY_AGGREGATE_CAPACITY = 2300;
constexpr uint8_t DAILY_RECORD_SIZE = 64;
constexpr uint8_t DAILY_FORMAT_VERSION = 1;

// Small fixed pending queue: no heap allocation per interruption.
constexpr uint8_t PENDING_EVENT_CAPACITY = 64;
constexpr uint32_t STORAGE_RETRY_INTERVAL_MS = 1500;

constexpr char RAW_DATA_PATH[] = "/interrupt.raw";
constexpr char RAW_META_PATH[] = "/interrupt.meta";
constexpr char DAILY_DATA_PATH[] = "/daily.bin";
constexpr char DAILY_META_PATH[] = "/daily.meta";

// Preferences namespaces are kept project-specific so the frozen base and
// unrelated future projects do not overwrite each other's persistent values.
constexpr char PREF_NAMESPACE[] = "interrupt";
constexpr char FS_PREF_NAMESPACE[] = "interruptfs";

// Host/browser live summary refresh. This is intentionally not used by the
// ESP32 firmware as a periodic task; the frontend polls only while Home or
// Analytics is active and the document is visible.
constexpr uint32_t LIVE_POLL_INTERVAL_MS = 1000;

}  // namespace ProjectConfig
