#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(name: str) -> str:
    return (ROOT / name).read_text(encoding="utf-8")


def write(name: str, text: str) -> None:
    (ROOT / name).write_text(text, encoding="utf-8")


def replace_once(name: str, old: str, new: str) -> None:
    text = read(name)
    if text.count(old) != 1:
        raise SystemExit(f"marker mismatch in {name}: {old[:100]!r} count={text.count(old)}")
    write(name, text.replace(old, new, 1))


def regex_once(name: str, pattern: str, replacement: str, flags=0) -> None:
    text = read(name)
    new, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f"regex mismatch in {name}: {pattern!r} count={count}")
    write(name, new)


# ---------------------------------------------------------------------------
# Version and defaults
# ---------------------------------------------------------------------------
replace_once("config.h", 'constexpr char SOFTWARE_VERSION[] = "3.1.0";', 'constexpr char SOFTWARE_VERSION[] = "3.2.0";')

project = read("project_config.h")
project = project.replace(
    "constexpr ProjectPreferences::SoundMode INTERRUPTION_SOUND_MODE_DEFAULT = ProjectPreferences::SoundMode::Fixed;",
    "constexpr ProjectPreferences::SoundMode INTERRUPTION_SOUND_MODE_DEFAULT = ProjectPreferences::SoundMode::Rotate;",
    1,
)
project = project.replace(
    "constexpr bool INTERRUPTION_SOUND_DEFAULT = true;",
    "constexpr bool INTERRUPTION_SOUND_DEFAULT = true;\nconstexpr uint8_t SOUND_VOLUME_DEFAULT_PERCENT = 100;",
    1,
)
project = project.replace(
    "constexpr bool DISPLAY_ENABLED_DEFAULT = true;",
    "constexpr bool DISPLAY_ENABLED_DEFAULT = true;\nconstexpr bool DISPLAY_ROTATION_180_DEFAULT = false;",
    1,
)
project = project.replace("constexpr uint8_t DISPLAY_BRIGHTNESS_DEFAULT_PERCENT = 50;", "constexpr uint8_t DISPLAY_BRIGHTNESS_DEFAULT_PERCENT = 65;", 1)
project = project.replace("constexpr uint8_t DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT = 10;", "constexpr uint8_t DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT = 5;", 1)
write("project_config.h", project)
replace_once("hardware_config.h", "constexpr uint32_t DISPLAY_BOOT_SCREEN_MIN_MS = 2000;", "constexpr uint32_t DISPLAY_BOOT_SCREEN_MIN_MS = 4000;")


# ---------------------------------------------------------------------------
# Persistent project preferences
# ---------------------------------------------------------------------------
write("project_preferences.h", r'''#pragma once

#include <Arduino.h>

namespace ProjectPreferences {

enum class SoundMode : uint8_t {
  Fixed = 0,
  Rotate = 1
};

enum class DisplayMode : uint8_t {
  Standard = 0,
  CountOnly = 1,
  LastOnly = 2,
  DayProgress = 3,
  Focus = 4
};

void begin();

bool soundEnabled();
bool setSoundEnabled(bool enabled);
uint8_t soundVolumePercent();
bool setSoundVolumePercent(uint8_t percent);
uint16_t soundTrack();
bool setSoundTrack(uint16_t track);
SoundMode soundMode();
const char *soundModeName();
bool setSoundMode(SoundMode mode);
bool parseSoundMode(const char *value, SoundMode &mode);

const char *language();
bool languageStored();
bool setLanguage(const char *language);

bool displayEnabled();
bool setDisplayEnabled(bool enabled);
bool displayRotation180();
bool setDisplayRotation180(bool rotated);
bool displayFlashEnabled();
bool setDisplayFlashEnabled(bool enabled);
DisplayMode displayMode();
const char *displayModeName();
bool setDisplayMode(DisplayMode mode);
bool parseDisplayMode(const char *value, DisplayMode &mode);
uint8_t displayBrightnessPercent();
bool setDisplayBrightnessPercent(uint8_t percent);
uint16_t displayDimAfterMinutes();
bool setDisplayDimAfterMinutes(uint16_t minutes);
uint8_t displayDimBrightnessPercent();
bool setDisplayDimBrightnessPercent(uint8_t percent);

}  // namespace ProjectPreferences
''')

write("project_preferences.cpp", r'''#include "project_preferences.h"

#include <Preferences.h>
#include <cstring>

#include "config.h"
#include "project_config.h"
#include "serial_log.h"

namespace ProjectPreferences {
namespace {
bool initialized = false;
bool sound = ProjectConfig::INTERRUPTION_SOUND_DEFAULT;
uint8_t soundVolume = ProjectConfig::SOUND_VOLUME_DEFAULT_PERCENT;
uint16_t track = ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT;
SoundMode soundModeValue = ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
char languageValue[12]{};
bool languageStoredValue = false;
bool displayEnabledValue = ProjectConfig::DISPLAY_ENABLED_DEFAULT;
bool displayRotation180Value = ProjectConfig::DISPLAY_ROTATION_180_DEFAULT;
bool flash = ProjectConfig::DISPLAY_FLASH_DEFAULT;
DisplayMode mode = ProjectConfig::DISPLAY_MODE_DEFAULT;
uint8_t brightness = ProjectConfig::DISPLAY_BRIGHTNESS_DEFAULT_PERCENT;
uint16_t dimAfterMinutes = ProjectConfig::DISPLAY_DIM_AFTER_DEFAULT_MINUTES;
uint8_t dimBrightness = ProjectConfig::DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT;

bool persistBool(const char *key, bool value) {
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::PREF_NAMESPACE, false)) return false;
  const size_t written = prefs.putBool(key, value);
  prefs.end();
  return written > 0;
}

bool persistUChar(const char *key, uint8_t value) {
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::PREF_NAMESPACE, false)) return false;
  const size_t written = prefs.putUChar(key, value);
  prefs.end();
  return written > 0;
}

bool persistUShort(const char *key, uint16_t value) {
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::PREF_NAMESPACE, false)) return false;
  const size_t written = prefs.putUShort(key, value);
  prefs.end();
  return written > 0;
}

bool persistString(const char *key, const char *value) {
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::PREF_NAMESPACE, false)) return false;
  const size_t written = prefs.putString(key, value ? value : "");
  prefs.end();
  return written > 0;
}

bool validLanguage(const char *value) {
  if (!value) return false;
  static const char *const supported[] = {"de", "en", "it", "fr", "swg", "swg-alb", "swg-ob"};
  for (const char *candidate : supported) {
    if (strcmp(value, candidate) == 0) return true;
  }
  return false;
}

void copyLanguage(const char *value) {
  const char *source = validLanguage(value) ? value : AppConfig::FALLBACK_LANGUAGE;
  strncpy(languageValue, source, sizeof(languageValue) - 1U);
  languageValue[sizeof(languageValue) - 1U] = '\0';
}

SoundMode sanitizedSoundMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(SoundMode::Rotate)
             ? static_cast<SoundMode>(raw)
             : ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
}

DisplayMode sanitizedMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(DisplayMode::Focus)
             ? static_cast<DisplayMode>(raw)
             : ProjectConfig::DISPLAY_MODE_DEFAULT;
}
}  // namespace

void begin() {
  if (initialized) return;
  initialized = true;
  copyLanguage(AppConfig::FALLBACK_LANGUAGE);

  Preferences prefs;
  if (!prefs.begin(ProjectConfig::PREF_NAMESPACE, false)) {
    SerialLog::warning("PROJECT", "Project Preferences unavailable; using defaults");
    return;
  }
  sound = prefs.getBool("sound", ProjectConfig::INTERRUPTION_SOUND_DEFAULT);
  soundVolume = prefs.getUChar("sndvol", ProjectConfig::SOUND_VOLUME_DEFAULT_PERCENT);
  if (soundVolume > 100) soundVolume = ProjectConfig::SOUND_VOLUME_DEFAULT_PERCENT;
  track = prefs.getUShort("sndtrack", ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT);
  if (track < 2) track = ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT;
  soundModeValue = sanitizedSoundMode(prefs.getUChar("sndmode", static_cast<uint8_t>(ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT)));

  const String storedLanguage = prefs.getString("lang", "");
  if (validLanguage(storedLanguage.c_str())) {
    copyLanguage(storedLanguage.c_str());
    languageStoredValue = true;
  }

  displayEnabledValue = prefs.getBool("dispen", ProjectConfig::DISPLAY_ENABLED_DEFAULT);
  displayRotation180Value = prefs.getBool("disprot", ProjectConfig::DISPLAY_ROTATION_180_DEFAULT);
  flash = prefs.getBool("dispflash", ProjectConfig::DISPLAY_FLASH_DEFAULT);
  mode = sanitizedMode(prefs.getUChar("dispmode", static_cast<uint8_t>(ProjectConfig::DISPLAY_MODE_DEFAULT)));
  brightness = prefs.getUChar("dispbright", ProjectConfig::DISPLAY_BRIGHTNESS_DEFAULT_PERCENT);
  if (brightness < 1 || brightness > 100) brightness = ProjectConfig::DISPLAY_BRIGHTNESS_DEFAULT_PERCENT;
  dimAfterMinutes = prefs.getUShort("dimmins", ProjectConfig::DISPLAY_DIM_AFTER_DEFAULT_MINUTES);
  if (dimAfterMinutes > ProjectConfig::DISPLAY_DIM_AFTER_MAX_MINUTES) {
    dimAfterMinutes = ProjectConfig::DISPLAY_DIM_AFTER_DEFAULT_MINUTES;
  }
  dimBrightness = prefs.getUChar("dimbright", ProjectConfig::DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT);
  if (dimBrightness > 100) dimBrightness = ProjectConfig::DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT;
  prefs.end();

  SerialLog::infof("PROJECT", "Feedback settings | sound=%s | volume=%u%% | sound-mode=%s | track=%u | display=%s | rotate180=%s | display-flash=%s | display-mode=%s",
                   sound ? "ON" : "OFF", static_cast<unsigned int>(soundVolume), soundModeName(), static_cast<unsigned int>(track),
                   displayEnabledValue ? "ON" : "OFF", displayRotation180Value ? "YES" : "NO",
                   flash ? "ON" : "OFF", displayModeName());
  SerialLog::infof("PROJECT", "Display settings | language=%s | brightness=%u%% | dim-after=%u min | dim-brightness=%u%%",
                   languageValue, static_cast<unsigned int>(brightness), static_cast<unsigned int>(dimAfterMinutes),
                   static_cast<unsigned int>(dimBrightness));
}

bool soundEnabled() { return sound; }

bool setSoundEnabled(bool enabled) {
  if (sound == enabled) return true;
  if (!persistBool("sound", enabled)) return false;
  sound = enabled;
  SerialLog::infof("PROJECT", "Interruption sound changed | %s", sound ? "ON" : "OFF");
  return true;
}

uint8_t soundVolumePercent() { return soundVolume; }

bool setSoundVolumePercent(uint8_t percent) {
  if (percent > 100) return false;
  if (soundVolume == percent) return true;
  if (!persistUChar("sndvol", percent)) return false;
  soundVolume = percent;
  SerialLog::infof("PROJECT", "Sound volume changed | %u%%", static_cast<unsigned int>(soundVolume));
  return true;
}

uint16_t soundTrack() { return track; }

bool setSoundTrack(uint16_t value) {
  if (value < 2) return false;  // Track 1 is reserved exclusively for boot.
  if (track == value) return true;
  if (!persistUShort("sndtrack", value)) return false;
  track = value;
  SerialLog::infof("PROJECT", "Interruption track changed | track=%u", static_cast<unsigned int>(track));
  return true;
}

SoundMode soundMode() { return soundModeValue; }

const char *soundModeName() {
  switch (soundModeValue) {
    case SoundMode::Rotate: return "rotate";
    case SoundMode::Fixed:
    default: return "fixed";
  }
}

bool parseSoundMode(const char *value, SoundMode &parsed) {
  if (!value) return false;
  if (strcmp(value, "fixed") == 0) parsed = SoundMode::Fixed;
  else if (strcmp(value, "rotate") == 0) parsed = SoundMode::Rotate;
  else return false;
  return true;
}

bool setSoundMode(SoundMode value) {
  if (soundModeValue == value) return true;
  if (!persistUChar("sndmode", static_cast<uint8_t>(value))) return false;
  soundModeValue = value;
  SerialLog::infof("PROJECT", "Interruption sound mode changed | %s", soundModeName());
  return true;
}

const char *language() { return languageValue[0] ? languageValue : AppConfig::FALLBACK_LANGUAGE; }
bool languageStored() { return languageStoredValue; }

bool setLanguage(const char *value) {
  if (!validLanguage(value)) return false;
  if (languageStoredValue && strcmp(language(), value) == 0) return true;
  if (!persistString("lang", value)) return false;
  copyLanguage(value);
  languageStoredValue = true;
  SerialLog::infof("PROJECT", "Display/UI language changed | %s", languageValue);
  return true;
}

bool displayEnabled() { return displayEnabledValue; }

bool setDisplayEnabled(bool enabled) {
  if (displayEnabledValue == enabled) return true;
  if (!persistBool("dispen", enabled)) return false;
  displayEnabledValue = enabled;
  SerialLog::infof("PROJECT", "Display master switch changed | %s", displayEnabledValue ? "ON" : "OFF");
  return true;
}

bool displayRotation180() { return displayRotation180Value; }

bool setDisplayRotation180(bool rotated) {
  if (displayRotation180Value == rotated) return true;
  if (!persistBool("disprot", rotated)) return false;
  displayRotation180Value = rotated;
  SerialLog::infof("PROJECT", "Display rotation changed | 180deg=%s", rotated ? "YES" : "NO");
  return true;
}

bool displayFlashEnabled() { return flash; }

bool setDisplayFlashEnabled(bool enabled) {
  if (flash == enabled) return true;
  if (!persistBool("dispflash", enabled)) return false;
  flash = enabled;
  SerialLog::infof("PROJECT", "Display interruption flash changed | %s", flash ? "ON" : "OFF");
  return true;
}

DisplayMode displayMode() { return mode; }

const char *displayModeName() {
  switch (mode) {
    case DisplayMode::CountOnly: return "count";
    case DisplayMode::LastOnly: return "last";
    case DisplayMode::DayProgress: return "day-progress";
    case DisplayMode::Focus: return "focus";
    case DisplayMode::Standard:
    default: return "standard";
  }
}

bool parseDisplayMode(const char *value, DisplayMode &parsed) {
  if (!value) return false;
  if (strcmp(value, "standard") == 0) parsed = DisplayMode::Standard;
  else if (strcmp(value, "count") == 0) parsed = DisplayMode::CountOnly;
  else if (strcmp(value, "last") == 0) parsed = DisplayMode::LastOnly;
  else if (strcmp(value, "day-progress") == 0) parsed = DisplayMode::DayProgress;
  else if (strcmp(value, "focus") == 0) parsed = DisplayMode::Focus;
  else return false;
  return true;
}

bool setDisplayMode(DisplayMode value) {
  if (mode == value) return true;
  if (!persistUChar("dispmode", static_cast<uint8_t>(value))) return false;
  mode = value;
  SerialLog::infof("PROJECT", "Display mode changed | %s", displayModeName());
  return true;
}

uint8_t displayBrightnessPercent() { return brightness; }

bool setDisplayBrightnessPercent(uint8_t percent) {
  if (percent < 1 || percent > 100) return false;
  if (brightness == percent) return true;
  if (!persistUChar("dispbright", percent)) return false;
  brightness = percent;
  SerialLog::infof("PROJECT", "Display brightness changed | %u%%", static_cast<unsigned int>(brightness));
  return true;
}

uint16_t displayDimAfterMinutes() { return dimAfterMinutes; }

bool setDisplayDimAfterMinutes(uint16_t minutes) {
  if (minutes > ProjectConfig::DISPLAY_DIM_AFTER_MAX_MINUTES) return false;
  if (dimAfterMinutes == minutes) return true;
  if (!persistUShort("dimmins", minutes)) return false;
  dimAfterMinutes = minutes;
  SerialLog::infof("PROJECT", "Display dim timeout changed | %u min", static_cast<unsigned int>(dimAfterMinutes));
  return true;
}

uint8_t displayDimBrightnessPercent() { return dimBrightness; }

bool setDisplayDimBrightnessPercent(uint8_t percent) {
  if (percent > 100) return false;
  if (dimBrightness == percent) return true;
  if (!persistUChar("dimbright", percent)) return false;
  dimBrightness = percent;
  SerialLog::infof("PROJECT", "Display dim brightness changed | %u%%", static_cast<unsigned int>(dimBrightness));
  return true;
}

}  // namespace ProjectPreferences
''')


# ---------------------------------------------------------------------------
# SH1106: 180 degree rotation + central UTF-8 -> compact ASCII transliteration
# ---------------------------------------------------------------------------
replace_once(
    "display_sh1106.h",
    "bool setContrast(uint8_t contrast);\nbool showBootScreen();",
    "bool setContrast(uint8_t contrast);\n// Store/apply the controller orientation without redrawing pixels in software.\nbool setRotation180(bool rotated);\nvoid setBootStatusText(const char *text);\nbool showBootScreen();",
)

text = read("display_sh1106.cpp")
text = text.replace(
    'uint32_t manualTestUntilMs = 0;\n',
    'uint32_t manualTestUntilMs = 0;\nbool rotation180 = false;\nchar bootStatusText[16] = "STARTING";\n',
    1,
)
normalize_code = r'''
void normalizeDisplayText(const char *input, char *output, size_t capacity) {
  if (!output || capacity == 0U) return;
  output[0] = '\0';
  if (!input) return;
  size_t used = 0;
  auto append = [&](const char *ascii) {
    if (!ascii) return;
    while (*ascii && used + 1U < capacity) output[used++] = *ascii++;
    output[used] = '\0';
  };

  const uint8_t *p = reinterpret_cast<const uint8_t *>(input);
  while (*p && used + 1U < capacity) {
    if (*p < 0x80U) {
      output[used++] = static_cast<char>(*p++);
      output[used] = '\0';
      continue;
    }
    if (*p == 0xC3U && p[1] != 0U) {
      const uint8_t second = p[1];
      if (second == 0x84U || second == 0xA4U) append("AE");
      else if (second == 0x96U || second == 0xB6U) append("OE");
      else if (second == 0x9CU || second == 0xBCU) append("UE");
      else if (second == 0x9FU) append("SS");
      else if ((second >= 0x80U && second <= 0x85U) || (second >= 0xA0U && second <= 0xA5U)) append("A");
      else if (second == 0x87U || second == 0xA7U) append("C");
      else if ((second >= 0x88U && second <= 0x8BU) || (second >= 0xA8U && second <= 0xABU)) append("E");
      else if ((second >= 0x8CU && second <= 0x8FU) || (second >= 0xACU && second <= 0xAFU)) append("I");
      else if (second == 0x91U || second == 0xB1U) append("N");
      else if ((second >= 0x92U && second <= 0x95U) || second == 0x98U ||
               (second >= 0xB2U && second <= 0xB5U) || second == 0xB8U) append("O");
      else if ((second >= 0x99U && second <= 0x9BU) || (second >= 0xB9U && second <= 0xBBU)) append("U");
      else if (second == 0x9DU || second == 0xBDU || second == 0xBFU) append("Y");
      p += 2;
      continue;
    }
    // Unknown UTF-8 codepoint: skip the complete sequence instead of showing
    // a broken glyph. The web UI remains full UTF-8; this path is OLED-only.
    if ((*p & 0xF0U) == 0xE0U) p += 3;
    else if ((*p & 0xF8U) == 0xF0U) p += 4;
    else p += 2;
  }
}
'''
marker = "const uint8_t *glyph(char c) {"
if marker not in text:
    raise SystemExit("display glyph marker missing")
text = text.replace(marker, normalize_code + "\n" + marker, 1)

text, n = re.subn(
    r"void text\(int16_t x, int16_t y, const char \*value\) \{.*?\n\}\n\nvoid scaledText",
    r'''void text(int16_t x, int16_t y, const char *value) {
  char normalized[80];
  normalizeDisplayText(value, normalized, sizeof(normalized));
  const char *cursor = normalized;
  while (*cursor && x < HardwareConfig::DISPLAY_WIDTH - 5) {
    const uint8_t *g = glyph(*cursor++);
    for (uint8_t col = 0; col < 5; ++col) {
      for (uint8_t row = 0; row < 7; ++row) {
        if (g[col] & (1U << row)) pixel(static_cast<int16_t>(x + col), static_cast<int16_t>(y + row));
      }
    }
    x = static_cast<int16_t>(x + 6);
  }
}

void scaledText''',
    text,
    count=1,
    flags=re.S,
)
if n != 1:
    raise SystemExit("display text function replacement failed")
text, n = re.subn(
    r"void scaledText\(int16_t x, int16_t y, const char \*value, uint8_t scale\) \{.*?\n\}\n\nvoid centeredText",
    r'''void scaledText(int16_t x, int16_t y, const char *value, uint8_t scale) {
  if (!value || scale == 0) return;
  char normalized[80];
  normalizeDisplayText(value, normalized, sizeof(normalized));
  const char *cursor = normalized;
  while (*cursor && x < HardwareConfig::DISPLAY_WIDTH) {
    const uint8_t *g = glyph(*cursor++);
    for (uint8_t col = 0; col < 5; ++col) {
      for (uint8_t row = 0; row < 7; ++row) {
        if (!(g[col] & (1U << row))) continue;
        for (uint8_t dx = 0; dx < scale; ++dx) {
          for (uint8_t dy = 0; dy < scale; ++dy) {
            pixel(static_cast<int16_t>(x + static_cast<int16_t>(col * scale + dx)),
                  static_cast<int16_t>(y + static_cast<int16_t>(row * scale + dy)));
          }
        }
      }
    }
    x = static_cast<int16_t>(x + static_cast<int16_t>(6U * scale));
  }
}

void centeredText''',
    text,
    count=1,
    flags=re.S,
)
if n != 1:
    raise SystemExit("display scaledText replacement failed")
text, n = re.subn(
    r"void centeredText\(int16_t y, const char \*value\) \{.*?\n\}\n\nvoid drawChipIcon",
    r'''void centeredText(int16_t y, const char *value) {
  if (!value) return;
  char normalized[80];
  normalizeDisplayText(value, normalized, sizeof(normalized));
  size_t len = std::strlen(normalized);
  if (len > 21) len = 21;
  const int16_t width = static_cast<int16_t>(len * 6 - (len ? 1 : 0));
  const int16_t x = static_cast<int16_t>((HardwareConfig::DISPLAY_WIDTH - width) / 2);
  char clipped[22]{};
  std::memcpy(clipped, normalized, len);
  clipped[len] = '\0';
  text(x, y, clipped);
}

void drawChipIcon''',
    text,
    count=1,
    flags=re.S,
)
if n != 1:
    raise SystemExit("display centeredText replacement failed")
text = text.replace("      command(0xA1) &&\n      command(0xC8) &&", "      command(rotation180 ? 0xA0 : 0xA1) &&\n      command(rotation180 ? 0xC0 : 0xC8) &&", 1)
text = text.replace('  centeredText(52, "STARTING");', '  centeredText(52, bootStatusText);', 1)
text = text.replace('  centeredText(9, "DISPLAY TEST");', '  centeredText(9, "SH1106 TEST");', 1)
insert_after = '''bool setContrast(uint8_t contrast) {
  if (!isInitialized && !initialize()) return false;
  const bool ok = command2(0x81, contrast);
  if (ok) setHealth(StatusRegistry::State::Ok);
  return ok;
}
'''
addition = insert_after + r'''

bool setRotation180(bool rotated) {
  rotation180 = rotated;
  if (!isInitialized) return true;
  const bool ok = command(rotation180 ? 0xA0 : 0xA1) && command(rotation180 ? 0xC0 : 0xC8);
  if (ok) setHealth(StatusRegistry::State::Ok);
  return ok;
}

void setBootStatusText(const char *value) {
  normalizeDisplayText(value ? value : "STARTING", bootStatusText, sizeof(bootStatusText));
  if (!bootStatusText[0]) strncpy(bootStatusText, "STARTING", sizeof(bootStatusText) - 1U);
  bootStatusText[sizeof(bootStatusText) - 1U] = '\0';
}
'''
if text.count(insert_after) != 1:
    raise SystemExit("display contrast insertion marker mismatch")
text = text.replace(insert_after, addition, 1)
write("display_sh1106.cpp", text)


# ---------------------------------------------------------------------------
# Audio: persistent UI percent mapped centrally to DY-SV17F 0..30
# ---------------------------------------------------------------------------
replace_once(
    "audio_dy_sv17f.h",
    "bool playTestTone();\nbool stop();\nbool pause();\nbool setVolume(uint8_t volume);",
    "bool playTestTone();\nbool stop();\nbool pause();\n// High-level project volume. Mapping to the DY-SV17F 0..30 range lives here.\nvoid configureVolumePercent(uint8_t percent);\nbool setVolumePercent(uint8_t percent);\nuint8_t volumePercent();\nbool setVolume(uint8_t volume);",
)

text = read("audio_dy_sv17f.cpp")
text = text.replace(
    "uint16_t tracks = 0;\n",
    "uint16_t tracks = 0;\nbool uartReady = false;\nuint8_t desiredVolumePercent = 100;\nbool volumePending = true;\n",
    1,
)
text = text.replace("void updateBusyPin();\n", "void updateBusyPin();\nbool applyDesiredVolume();\n", 1)
command_idle = '''bool commandPathIdle() {
  return !probeActive && waitingFor == WaitKind::None && deferredAction == DeferredAction::None;
}
'''
volume_helpers = command_idle + r'''

uint8_t moduleVolumeForPercent(uint8_t percent) {
  if (percent > 100U) percent = 100U;
  return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 30U + 50U) / 100U);
}

bool applyDesiredVolume() {
  if (!uartReady || !volumePending || !commandPathIdle()) return false;
  const uint8_t moduleVolume = moduleVolumeForPercent(desiredVolumePercent);
  sendFrame(0x13, &moduleVolume, 1);
  volumePending = false;
  SerialLog::infof("AUDIO", "Volume applied | ui=%u%% | module=%u/30",
                   static_cast<unsigned int>(desiredVolumePercent), static_cast<unsigned int>(moduleVolume));
  return true;
}
'''
if text.count(command_idle) != 1:
    raise SystemExit("audio command idle marker missing")
text = text.replace(command_idle, volume_helpers, 1)
text = text.replace(
    "  if (shouldPlayBootTone) {\n",
    "  // Apply the user volume before scheduling the boot tone. The command has\n  // no response frame, so this adds no blocking wait.\n  applyDesiredVolume();\n\n  if (shouldPlayBootTone) {\n",
    1,
)
text = text.replace(
    "  audioSerial.begin(HardwareConfig::AUDIO_BAUD_RATE, SERIAL_8N1,\n                    HardwareConfig::AUDIO_RX_PIN, HardwareConfig::AUDIO_TX_PIN);",
    "  audioSerial.begin(HardwareConfig::AUDIO_BAUD_RATE, SERIAL_8N1,\n                    HardwareConfig::AUDIO_RX_PIN, HardwareConfig::AUDIO_TX_PIN);\n  uartReady = true;\n  volumePending = true;",
    1,
)
text = text.replace(
    "  handleTimeout();\n\n  const uint32_t now = millis();",
    "  handleTimeout();\n  if (volumePending && commandPathIdle()) applyDesiredVolume();\n\n  const uint32_t now = millis();",
    1,
)
public_marker = "uint16_t musicCount() { return tracks; }\n\n"
public_add = public_marker + r'''void configureVolumePercent(uint8_t percent) {
  desiredVolumePercent = percent > 100U ? 100U : percent;
  volumePending = true;
  if (uartReady && commandPathIdle()) applyDesiredVolume();
}

bool setVolumePercent(uint8_t percent) {
  if (percent > 100U) return false;
  desiredVolumePercent = percent;
  volumePending = true;
  if (!uartReady || !commandPathIdle()) return true;
  return applyDesiredVolume();
}

uint8_t volumePercent() { return desiredVolumePercent; }

'''
if text.count(public_marker) != 1:
    raise SystemExit("audio public marker missing")
text = text.replace(public_marker, public_add, 1)
write("audio_dy_sv17f.cpp", text)


# ---------------------------------------------------------------------------
# Summary: today's completed interval average for the new OLED day view
# ---------------------------------------------------------------------------
replace_once(
    "interruption_types.h",
    "  uint32_t todayCount = 0;\n  uint32_t unassignedCount = 0;",
    "  uint32_t todayCount = 0;\n  uint64_t todayIntervalSumSeconds = 0;\n  uint32_t todayIntervalSamples = 0;\n  uint32_t unassignedCount = 0;",
)

replace_once(
    "interruption_service.h",
    "bool setSoundEnabled(bool enabled);\nbool soundEnabled();",
    "bool setSoundEnabled(bool enabled);\nbool setSoundVolumePercent(uint8_t percent);\nbool soundEnabled();",
)

text = read("interruption_service.cpp")
refresh_marker = "void refreshCurrentDay(bool force) {"
rebuild_code = r'''void rebuildTodayIntervalsFromRetainedRaw() {
  currentSummary.todayIntervalSumSeconds = 0;
  currentSummary.todayIntervalSamples = 0;
  if (!currentDayValid || !InterruptionStore::ready()) return;

  const uint64_t first = InterruptionStore::oldestSequence();
  const uint64_t last = InterruptionStore::newestSequence();
  bool sawToday = false;
  if (first != 0U && last >= first) {
    for (uint64_t sequence = last;; --sequence) {
      InterruptionTypes::RawEvent raw;
      if (InterruptionStore::readSequence(sequence, raw) && raw.absoluteValid) {
        ProjectTime::LocalDateTime local;
        if (ProjectTime::fromEpochSeconds(raw.timeValueSeconds, local)) {
          if (local.dayIndex == currentDayIndex) {
            sawToday = true;
            // deltaSeconds belongs to the current event and closes the interval
            // that started at its retained predecessor. If the ring has wrapped,
            // the oldest record's predecessor is gone and its delta is excluded.
            if (sequence > first && raw.deltaSeconds > 0U && raw.deltaSeconds < InterruptionTypes::DELTA_UNKNOWN) {
              currentSummary.todayIntervalSumSeconds += raw.deltaSeconds;
              ++currentSummary.todayIntervalSamples;
            }
          } else if (sawToday && local.dayIndex < currentDayIndex) {
            break;
          }
        }
      }
      if (sequence == first) break;
    }
  }

  // Pending records are not yet in the raw ring but already represent real
  // completed intervals and should be visible on the local display immediately.
  for (uint8_t i = 0, idx = queueHead; i < queueCount; ++i,
       idx = static_cast<uint8_t>((idx + 1U) % ProjectConfig::PENDING_EVENT_CAPACITY)) {
    const auto &event = queue[idx];
    if (event.localCalendarValid && event.localDayIndex == currentDayIndex &&
        event.deltaSeconds > 0U && event.deltaSeconds < InterruptionTypes::DELTA_UNKNOWN) {
      currentSummary.todayIntervalSumSeconds += event.deltaSeconds;
      ++currentSummary.todayIntervalSamples;
    }
  }
}

'''
if refresh_marker not in text:
    raise SystemExit("refreshCurrentDay marker missing")
text = text.replace(refresh_marker, rebuild_code + refresh_marker, 1)

old_refresh_head = '''  if (!currentDayValid || local.dayIndex != currentDayIndex || force) {
    currentDayValid = true;
    currentDayIndex = local.dayIndex;
    currentSummary.todayCount = InterruptionAggregates::ready() ? InterruptionAggregates::countForDay(currentDayIndex) : 0;'''
new_refresh_head = '''  const bool dayChanged = !currentDayValid || local.dayIndex != currentDayIndex;
  if (dayChanged || force) {
    currentDayValid = true;
    currentDayIndex = local.dayIndex;
    currentSummary.todayCount = InterruptionAggregates::ready() ? InterruptionAggregates::countForDay(currentDayIndex) : 0;'''
if text.count(old_refresh_head) != 1:
    raise SystemExit("refresh current day head mismatch")
text = text.replace(old_refresh_head, new_refresh_head, 1)
old_refresh_tail = '''    if (!(anchorValid && anchorDayIndex == currentDayIndex)) anchorValid = false;
    bumpRevision();'''
new_refresh_tail = '''    if (dayChanged) rebuildTodayIntervalsFromRetainedRaw();
    if (!(anchorValid && anchorDayIndex == currentDayIndex)) anchorValid = false;
    bumpRevision();'''
if text.count(old_refresh_tail) != 1:
    raise SystemExit("refresh current day tail mismatch")
text = text.replace(old_refresh_tail, new_refresh_tail, 1)

old_capture = '''    if (!currentDayValid || event.localDayIndex != currentDayIndex) {
      currentDayValid = true;
      currentDayIndex = event.localDayIndex;
      currentSummary.todayCount = 0;
    }
    ++currentSummary.todayCount;'''
new_capture = '''    if (!currentDayValid || event.localDayIndex != currentDayIndex) {
      currentDayValid = true;
      currentDayIndex = event.localDayIndex;
      currentSummary.todayCount = 0;
      currentSummary.todayIntervalSumSeconds = 0;
      currentSummary.todayIntervalSamples = 0;
    }
    ++currentSummary.todayCount;
    if (event.deltaSeconds > 0U && event.deltaSeconds < InterruptionTypes::DELTA_UNKNOWN) {
      currentSummary.todayIntervalSumSeconds += event.deltaSeconds;
      ++currentSummary.todayIntervalSamples;
    }'''
if text.count(old_capture) != 1:
    raise SystemExit("capture summary marker mismatch")
text = text.replace(old_capture, new_capture, 1)

end_sound = '''bool setSoundEnabled(bool enabled) {
  if (!ProjectPreferences::setSoundEnabled(enabled)) return false;
  currentSummary.soundEnabled = enabled;
  bumpRevision();
  if (!enabled) audioPending = 0;
  return true;
}

bool soundEnabled() { return ProjectPreferences::soundEnabled(); }'''
new_end_sound = '''bool setSoundEnabled(bool enabled) {
  if (!ProjectPreferences::setSoundEnabled(enabled)) return false;
  currentSummary.soundEnabled = enabled;
  bumpRevision();
  if (!enabled) audioPending = 0;
  return true;
}

bool setSoundVolumePercent(uint8_t percent) {
  if (!ProjectPreferences::setSoundVolumePercent(percent)) return false;
  return AudioDySv17f::setVolumePercent(percent);
}

bool soundEnabled() { return ProjectPreferences::soundEnabled(); }'''
if text.count(end_sound) != 1:
    raise SystemExit("sound service tail marker mismatch")
text = text.replace(end_sound, new_end_sound, 1)
write("interruption_service.cpp", text)


# ---------------------------------------------------------------------------
# Localized OLED views + two new modes
# ---------------------------------------------------------------------------
write("display_views.cpp", r'''#include "display_views.h"

#include <algorithm>
#include <cstring>

#include "display_sh1106.h"
#include "project_config.h"
#include "project_preferences.h"
#include "project_time.h"
#include "time_service.h"
#include "wifi_module.h"

namespace DisplayViews {
namespace {

bool renderRequested = true;
bool flashRequested = false;
bool flashActive = false;
uint32_t flashUntilMs = 0;
uint32_t lastIdleEvaluationMs = 0;
uint32_t lastActivityMs = 0;
uint32_t lastCount = UINT32_MAX;
uint64_t lastIntervalSum = UINT64_MAX;
uint32_t lastIntervalSamples = UINT32_MAX;
bool lastWifi = false;
bool lastTimeOk = false;
char lastAge[16] = "";
ProjectPreferences::DisplayMode lastMode = ProjectPreferences::DisplayMode::Standard;
int16_t lastContrast = -1;
bool dimmed = false;
bool displayPowerOn = true;
bool manualTestWasActive = false;

struct Labels {
  const char *today;
  const char *last;
  const char *now;
  const char *focus;
  const char *average;
};

const Labels &labels() {
  static const Labels de{"HEUTE", "LETZTE", "JETZT", "FOKUS", "SCHNITT"};
  static const Labels en{"TODAY", "LAST", "NOW", "FOCUS", "AVG"};
  static const Labels fr{"JOUR", "DERNIER", "MAINT", "FOCUS", "MOY"};
  static const Labels it{"OGGI", "ULTIMA", "ORA", "FOCUS", "MEDIA"};
  static const Labels swg{"HEIT", "LETSCHTE", "JETZT", "FOKUS", "SCHNITT"};
  const char *language = ProjectPreferences::language();
  if (strcmp(language, "en") == 0) return en;
  if (strcmp(language, "fr") == 0) return fr;
  if (strcmp(language, "it") == 0) return it;
  if (strncmp(language, "swg", 3) == 0) return swg;
  return de;
}

bool due(uint32_t now, uint32_t deadline) { return static_cast<int32_t>(now - deadline) >= 0; }

void ageText(const InterruptionTypes::Summary &summary, char out[16]) {
  if (!summary.lastAvailable) { snprintf(out, 16, "--"); return; }
  uint64_t ageSeconds = 0;
  bool known = false;
  if (summary.lastAbsoluteValid) {
    const TimeTypes::Snapshot now = TimeService::now();
    if (now.valid && now.epochMs >= static_cast<int64_t>(summary.lastTimeValueSeconds) * 1000LL) {
      ageSeconds = static_cast<uint64_t>(now.epochMs / 1000LL) - summary.lastTimeValueSeconds;
      known = true;
    }
  } else {
    const uint64_t nowMono = TimeService::eventTimestamp().monotonicMs;
    if (summary.lastMonotonicMs > 0 && nowMono >= summary.lastMonotonicMs) {
      ageSeconds = (nowMono - summary.lastMonotonicMs) / 1000ULL;
      known = true;
    }
  }
  if (!known) { snprintf(out, 16, "--"); return; }
  if (ageSeconds < 10) snprintf(out, 16, "%s", labels().now);
  else if (ageSeconds < 60) snprintf(out, 16, "%llus", static_cast<unsigned long long>(ageSeconds));
  else if (ageSeconds < 3600) snprintf(out, 16, "%llum", static_cast<unsigned long long>(ageSeconds / 60ULL));
  else if (ageSeconds < 86400) {
    const uint64_t hours = ageSeconds / 3600ULL;
    const uint64_t minutes = (ageSeconds % 3600ULL) / 60ULL;
    if (minutes) snprintf(out, 16, "%lluh%02llum", static_cast<unsigned long long>(hours), static_cast<unsigned long long>(minutes));
    else snprintf(out, 16, "%lluh", static_cast<unsigned long long>(hours));
  } else {
    snprintf(out, 16, "%llud", static_cast<unsigned long long>(ageSeconds / 86400ULL));
  }
}

void intervalAverageText(const InterruptionTypes::Summary &summary, char out[16]) {
  if (summary.todayIntervalSamples == 0U) { snprintf(out, 16, "--"); return; }
  const uint64_t seconds = summary.todayIntervalSumSeconds / summary.todayIntervalSamples;
  if (seconds < 60ULL) snprintf(out, 16, "%llus", static_cast<unsigned long long>(seconds));
  else if (seconds < 3600ULL) snprintf(out, 16, "%llum", static_cast<unsigned long long>(seconds / 60ULL));
  else {
    const uint64_t hours = seconds / 3600ULL;
    const uint64_t minutes = (seconds % 3600ULL) / 60ULL;
    snprintf(out, 16, "%lluh%02llum", static_cast<unsigned long long>(hours), static_cast<unsigned long long>(minutes));
  }
}

void drawWifiIcon(int16_t x, int16_t y, bool connected) {
  if (!connected) { DisplaySh1106::drawText(x, y, "W-"); return; }
  DisplaySh1106::drawPixel(x + 5, y + 8);
  DisplaySh1106::drawHLine(x + 3, x + 7, y + 6);
  DisplaySh1106::drawHLine(x + 1, x + 9, y + 3);
}

void drawTimeIcon(int16_t x, int16_t y, bool ok) {
  DisplaySh1106::drawRect(x, y, 10, 10);
  if (ok) {
    DisplaySh1106::drawVLine(x + 5, y + 2, y + 5);
    DisplaySh1106::drawHLine(x + 5, x + 7, y + 5);
  } else {
    DisplaySh1106::drawHLine(x + 2, x + 7, y + 5);
  }
}

uint8_t fittedScale(const char *value, uint8_t maxScale = 9, uint8_t maxWidth = 126, uint8_t maxHeight = 62) {
  if (!value || !*value) return 1;
  const size_t length = strlen(value);
  uint8_t scale = maxScale;
  while (scale > 1) {
    const size_t width = length * 6U * scale - scale;
    const size_t height = 7U * scale;
    if (width <= maxWidth && height <= maxHeight) break;
    --scale;
  }
  return scale;
}

void drawCenteredScaledAt(const char *value, int16_t y, uint8_t maxScale, uint8_t maxWidth = 126) {
  const char *shown = value && *value ? value : "--";
  const uint8_t scale = fittedScale(shown, maxScale, maxWidth, 62);
  const size_t length = strlen(shown);
  const int16_t width = static_cast<int16_t>(length ? length * 6U * scale - scale : 0U);
  const int16_t x = std::max<int16_t>(0, static_cast<int16_t>((128 - width) / 2));
  DisplaySh1106::drawTextScaled(x, y, shown, scale);
}

void drawCenteredScaled(const char *value) {
  const char *shown = value && *value ? value : "--";
  const uint8_t scale = fittedScale(shown);
  const size_t length = strlen(shown);
  const int16_t width = static_cast<int16_t>(length ? length * 6U * scale - scale : 0U);
  const int16_t height = static_cast<int16_t>(7U * scale);
  const int16_t x = std::max<int16_t>(0, static_cast<int16_t>((128 - width) / 2));
  const int16_t y = std::max<int16_t>(0, static_cast<int16_t>((64 - height) / 2));
  DisplaySh1106::drawTextScaled(x, y, shown, scale);
}

bool renderStandard(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawText(2, 1, labels().today);
  drawTimeIcon(98, 0, timeOk);
  drawWifiIcon(114, 0, wifi);
  DisplaySh1106::drawHLine(0, 127, 12);

  char count[12];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(summary.todayCount));
  const size_t digits = strlen(count);
  const int16_t width = static_cast<int16_t>(digits * 18U - (digits ? 3U : 0U));
  const int16_t x = static_cast<int16_t>((128 - width) / 2);
  DisplaySh1106::drawTextScaled(x < 0 ? 0 : x, 17, count, 3);

  DisplaySh1106::drawHLine(0, 127, 43);
  DisplaySh1106::drawText(2, 48, labels().last);
  DisplaySh1106::drawText(80, 48, age);
  return DisplaySh1106::present();
}

bool renderCountOnly(const InterruptionTypes::Summary &summary) {
  char count[12];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(summary.todayCount));
  DisplaySh1106::frameClear();
  drawCenteredScaled(count);
  return DisplaySh1106::present();
}

bool renderLastOnly(const char *age) {
  DisplaySh1106::frameClear();
  drawCenteredScaled(age);
  return DisplaySh1106::present();
}

bool renderDayProgress(const InterruptionTypes::Summary &summary, bool wifi, bool timeOk) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawText(2, 1, labels().today);
  drawTimeIcon(98, 0, timeOk);
  drawWifiIcon(114, 0, wifi);
  DisplaySh1106::drawHLine(0, 127, 12);

  char count[12];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(summary.todayCount));
  const uint8_t countScale = fittedScale(count, 3, 58, 28);
  DisplaySh1106::drawTextScaled(4, 20, count, countScale);

  char average[16];
  intervalAverageText(summary, average);
  DisplaySh1106::drawText(68, 18, labels().average);
  DisplaySh1106::drawText(68, 31, average);

  // A tiny clock-progress bar gives the day overview useful visual structure
  // without inventing a score or target that the project does not know.
  DisplaySh1106::drawRect(5, 53, 118, 7);
  if (timeOk) {
    const TimeTypes::Snapshot now = TimeService::now();
    ProjectTime::LocalDateTime local;
    if (now.valid && ProjectTime::fromEpochMs(now.epochMs, local)) {
      const uint16_t minuteOfDay = static_cast<uint16_t>(local.hour) * 60U + local.minute;
      const int16_t filled = static_cast<int16_t>((static_cast<uint32_t>(minuteOfDay) * 114U) / 1439U);
      for (int16_t x = 7; x < 7 + filled; ++x) {
        DisplaySh1106::drawVLine(x, 55, 57);
      }
    }
  }
  return DisplaySh1106::present();
}

bool renderFocus(const InterruptionTypes::Summary &summary, const char *age) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawCenteredText(2, labels().focus);
  DisplaySh1106::drawHLine(0, 127, 12);
  drawCenteredScaledAt(age, 20, 4, 124);
  DisplaySh1106::drawHLine(0, 127, 48);
  char footer[22];
  snprintf(footer, sizeof(footer), "%s %lu", labels().today, static_cast<unsigned long>(summary.todayCount));
  DisplaySh1106::drawCenteredText(53, footer);
  return DisplaySh1106::present();
}

bool renderHome(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return false;
  switch (ProjectPreferences::displayMode()) {
    case ProjectPreferences::DisplayMode::CountOnly: return renderCountOnly(summary);
    case ProjectPreferences::DisplayMode::LastOnly: return renderLastOnly(age);
    case ProjectPreferences::DisplayMode::DayProgress: return renderDayProgress(summary, wifi, timeOk);
    case ProjectPreferences::DisplayMode::Focus: return renderFocus(summary, age);
    case ProjectPreferences::DisplayMode::Standard:
    default: return renderStandard(summary, age, wifi, timeOk);
  }
}

uint8_t contrastFromPercent(uint8_t percent) {
  return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255U + 50U) / 100U);
}

void updateContrast(uint32_t nowMs) {
  const uint16_t dimMinutes = ProjectPreferences::displayDimAfterMinutes();
  const uint32_t dimAfterMs = static_cast<uint32_t>(dimMinutes) * 60000UL;
  const bool shouldDim = dimMinutes > 0U && static_cast<uint32_t>(nowMs - lastActivityMs) >= dimAfterMs;
  const uint8_t percent = shouldDim ? ProjectPreferences::displayDimBrightnessPercent()
                                    : ProjectPreferences::displayBrightnessPercent();
  const int16_t desired = contrastFromPercent(percent);
  if (desired == lastContrast && shouldDim == dimmed) return;
  if (DisplaySh1106::setContrast(static_cast<uint8_t>(desired))) {
    lastContrast = desired;
    dimmed = shouldDim;
  }
}

}  // namespace

void begin(const InterruptionTypes::Summary &summary) {
  lastActivityMs = millis();
  lastMode = ProjectPreferences::displayMode();
  DisplaySh1106::setRotation180(ProjectPreferences::displayRotation180());
  displayPowerOn = true;
  renderRequested = true;
  update(summary);
}

void notifyInterruption(bool flashEnabled) {
  const uint32_t nowMs = millis();
  lastActivityMs = nowMs;
  dimmed = false;
  renderRequested = true;
  lastContrast = -1;
  if (!ProjectPreferences::displayEnabled() || DisplaySh1106::bootScreenActive()) {
    flashRequested = false;
    return;
  }
  flashRequested = flashEnabled;
}

void requestHomeRefresh() { renderRequested = true; }

void settingsChanged() {
  lastActivityMs = millis();
  dimmed = false;
  lastContrast = -1;
  renderRequested = true;

  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  DisplaySh1106::setRotation180(ProjectPreferences::displayRotation180());
  if (DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) return;

  if (!ProjectPreferences::displayEnabled()) {
    if (flashActive) DisplaySh1106::setInverted(false);
    flashActive = false;
    flashRequested = false;
    if (DisplaySh1106::setPower(false)) displayPowerOn = false;
    return;
  }
  if (DisplaySh1106::setPower(true)) displayPowerOn = true;
}

void update(const InterruptionTypes::Summary &summary) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  const uint32_t nowMs = millis();

  if (DisplaySh1106::bootScreenActive()) {
    displayPowerOn = true;
    return;
  }
  if (DisplaySh1106::manualTestActive()) {
    manualTestWasActive = true;
    displayPowerOn = true;
    return;
  }
  if (manualTestWasActive) {
    manualTestWasActive = false;
    renderRequested = true;
    lastContrast = -1;
  }

  if (!ProjectPreferences::displayEnabled()) {
    if (flashActive) DisplaySh1106::setInverted(false);
    flashActive = false;
    flashRequested = false;
    if (displayPowerOn && DisplaySh1106::setPower(false)) displayPowerOn = false;
    return;
  }

  if (!displayPowerOn) {
    if (!DisplaySh1106::setPower(true)) return;
    displayPowerOn = true;
    lastContrast = -1;
    renderRequested = true;
  }

  updateContrast(nowMs);

  if (!renderRequested && !flashRequested && !flashActive &&
      static_cast<uint32_t>(nowMs - lastIdleEvaluationMs) < 1000U) return;
  if (flashActive && !due(nowMs, flashUntilMs)) return;
  lastIdleEvaluationMs = nowMs;

  if (flashActive && due(nowMs, flashUntilMs)) {
    DisplaySh1106::setInverted(false);
    flashActive = false;
    renderRequested = true;
  }

  char age[16]; ageText(summary, age);
  const bool wifi = WifiModule::stationConnected();
  const bool timeOk = TimeService::now().valid;
  const auto mode = ProjectPreferences::displayMode();
  const bool changed = summary.todayCount != lastCount ||
                       summary.todayIntervalSumSeconds != lastIntervalSum ||
                       summary.todayIntervalSamples != lastIntervalSamples ||
                       wifi != lastWifi || timeOk != lastTimeOk || strcmp(age, lastAge) != 0 || mode != lastMode;
  if (!renderRequested && !changed) return;

  if (renderHome(summary, age, wifi, timeOk)) {
    lastCount = summary.todayCount;
    lastIntervalSum = summary.todayIntervalSumSeconds;
    lastIntervalSamples = summary.todayIntervalSamples;
    lastWifi = wifi;
    lastTimeOk = timeOk;
    lastMode = mode;
    strncpy(lastAge, age, sizeof(lastAge) - 1);
    lastAge[sizeof(lastAge) - 1] = '\0';
    renderRequested = false;
    if (flashRequested) {
      flashRequested = false;
      if (DisplaySh1106::setInverted(true)) {
        flashActive = true;
        flashUntilMs = nowMs + ProjectConfig::DISPLAY_FLASH_MS;
      }
    }
  }
}

}  // namespace DisplayViews
''')


# ---------------------------------------------------------------------------
# API preferences + bootstrap-visible daily interval metadata
# ---------------------------------------------------------------------------
text = read("interruption_api.cpp")
old_pref = '''  fieldBool(out, "soundEnabled", ProjectPreferences::soundEnabled());
  fieldUInt(out, "soundTrack", ProjectPreferences::soundTrack());
  fieldString(out, "soundMode", ProjectPreferences::soundModeName());
  fieldUInt(out, "soundTrackCount", AudioDySv17f::musicCount());
  fieldBool(out, "displayEnabled", ProjectPreferences::displayEnabled());
  fieldBool(out, "displayFlashEnabled", ProjectPreferences::displayFlashEnabled());
  fieldString(out, "displayMode", ProjectPreferences::displayModeName());
  fieldUInt(out, "displayBrightness", ProjectPreferences::displayBrightnessPercent());
  fieldUInt(out, "displayDimAfterMinutes", ProjectPreferences::displayDimAfterMinutes());
  fieldUInt(out, "displayDimBrightness", ProjectPreferences::displayDimBrightnessPercent(), false);'''
new_pref = '''  fieldBool(out, "soundEnabled", ProjectPreferences::soundEnabled());
  fieldUInt(out, "soundVolume", ProjectPreferences::soundVolumePercent());
  fieldUInt(out, "soundTrack", ProjectPreferences::soundTrack());
  fieldString(out, "soundMode", ProjectPreferences::soundModeName());
  fieldUInt(out, "soundTrackCount", AudioDySv17f::musicCount());
  fieldString(out, "language", ProjectPreferences::language());
  fieldBool(out, "languageStored", ProjectPreferences::languageStored());
  fieldBool(out, "displayEnabled", ProjectPreferences::displayEnabled());
  fieldBool(out, "displayRotation180", ProjectPreferences::displayRotation180());
  fieldBool(out, "displayFlashEnabled", ProjectPreferences::displayFlashEnabled());
  fieldString(out, "displayMode", ProjectPreferences::displayModeName());
  fieldUInt(out, "displayBrightness", ProjectPreferences::displayBrightnessPercent());
  fieldUInt(out, "displayDimAfterMinutes", ProjectPreferences::displayDimAfterMinutes());
  fieldUInt(out, "displayDimBrightness", ProjectPreferences::displayDimBrightnessPercent(), false);'''
if text.count(old_pref) != 1:
    raise SystemExit("interruption API preference object mismatch")
text = text.replace(old_pref, new_pref, 1)
text = text.replace(
    '  fieldUInt(out, "todayCount", summary.todayCount);\n',
    '  fieldUInt(out, "todayCount", summary.todayCount);\n  fieldUInt(out, "todayIntervalSamples", summary.todayIntervalSamples);\n  fieldUInt(out, "todayIntervalAverageSeconds", summary.todayIntervalSamples ? static_cast<uint32_t>(summary.todayIntervalSumSeconds / summary.todayIntervalSamples) : 0U);\n',
    1,
)
write("interruption_api.cpp", text)


# ---------------------------------------------------------------------------
# Project preference HTTP endpoint: one field per request remains the contract
# ---------------------------------------------------------------------------
text = read("web_server.cpp")
text = text.replace(
    '  const bool hasTrack = server.hasArg("soundTrack");\n  const bool hasSoundMode = server.hasArg("soundMode");',
    '  const bool hasVolume = server.hasArg("soundVolume");\n  const bool hasTrack = server.hasArg("soundTrack");\n  const bool hasSoundMode = server.hasArg("soundMode");\n  const bool hasLanguage = server.hasArg("language");',
    1,
)
text = text.replace(
    '  const bool hasDisplayEnabled = server.hasArg("displayEnabled");\n  const bool hasFlash = server.hasArg("displayFlashEnabled");',
    '  const bool hasDisplayEnabled = server.hasArg("displayEnabled");\n  const bool hasRotation = server.hasArg("displayRotation180");\n  const bool hasFlash = server.hasArg("displayFlashEnabled");',
    1,
)
old_fields = '''  const uint8_t fields = static_cast<uint8_t>(hasSound) + static_cast<uint8_t>(hasTrack) +
                         static_cast<uint8_t>(hasSoundMode) + static_cast<uint8_t>(hasDisplayEnabled) +
                         static_cast<uint8_t>(hasFlash) + static_cast<uint8_t>(hasMode) +
                         static_cast<uint8_t>(hasBrightness) + static_cast<uint8_t>(hasDimAfter) +
                         static_cast<uint8_t>(hasDimBrightness);'''
new_fields = '''  const uint8_t fields = static_cast<uint8_t>(hasSound) + static_cast<uint8_t>(hasVolume) +
                         static_cast<uint8_t>(hasTrack) + static_cast<uint8_t>(hasSoundMode) +
                         static_cast<uint8_t>(hasLanguage) + static_cast<uint8_t>(hasDisplayEnabled) +
                         static_cast<uint8_t>(hasRotation) + static_cast<uint8_t>(hasFlash) +
                         static_cast<uint8_t>(hasMode) + static_cast<uint8_t>(hasBrightness) +
                         static_cast<uint8_t>(hasDimAfter) + static_cast<uint8_t>(hasDimBrightness);'''
if text.count(old_fields) != 1:
    raise SystemExit("web preference field-count marker mismatch")
text = text.replace(old_fields, new_fields, 1)
text = text.replace(
    '''    ok = InterruptionService::setSoundEnabled(value);
  } else if (hasTrack) {''',
    '''    ok = InterruptionService::setSoundEnabled(value);
  } else if (hasVolume) {
    uint32_t value = 0;
    if (!parseUnsignedArg(server.arg("soundVolume"), 0, 100, value)) {
      server.send(400, "application/json; charset=utf-8", "{\\\"ok\\\":false,\\\"error\\\":\\\"invalid_sound_volume\\\"}");
      return;
    }
    ok = InterruptionService::setSoundVolumePercent(static_cast<uint8_t>(value));
  } else if (hasTrack) {''',
    1,
)
text = text.replace(
    '''    ok = ProjectPreferences::setSoundMode(value);
  } else if (hasDisplayEnabled) {''',
    '''    ok = ProjectPreferences::setSoundMode(value);
  } else if (hasLanguage) {
    ok = ProjectPreferences::setLanguage(server.arg("language").c_str());
    if (!ok) {
      server.send(400, "application/json; charset=utf-8", "{\\\"ok\\\":false,\\\"error\\\":\\\"invalid_language\\\"}");
      return;
    }
    displayChanged = true;
  } else if (hasDisplayEnabled) {''',
    1,
)
text = text.replace(
    '''    ok = ProjectPreferences::setDisplayEnabled(value);
    displayChanged = ok;
  } else if (hasFlash) {''',
    '''    ok = ProjectPreferences::setDisplayEnabled(value);
    displayChanged = ok;
  } else if (hasRotation) {
    bool value = false;
    if (!parseBoolArg(server.arg("displayRotation180"), value)) {
      server.send(400, "application/json; charset=utf-8", "{\\\"ok\\\":false,\\\"error\\\":\\\"invalid_display_rotation\\\"}");
      return;
    }
    ok = ProjectPreferences::setDisplayRotation180(value);
    displayChanged = ok;
  } else if (hasFlash) {''',
    1,
)
write("web_server.cpp", text)


# ---------------------------------------------------------------------------
# Preferences must be loaded before hardware so boot rotation/language/volume
# are already known on the first visible/audio output.
# ---------------------------------------------------------------------------
text = read("Unterbrechungszaehler.ino")
text = text.replace(
    '#include "hardware_registry.h"\n',
    '#include "hardware_registry.h"\n#include "audio_dy_sv17f.h"\n#include "display_sh1106.h"\n#include "project_preferences.h"\n',
    1,
)
helper = r'''
namespace {
const char *bootStatusForLanguage(const char *language) {
  if (!language) return "STARTING";
  if (strcmp(language, "de") == 0) return "START";
  if (strcmp(language, "fr") == 0) return "DEMARRAGE";
  if (strcmp(language, "it") == 0) return "AVVIO";
  if (strncmp(language, "swg", 3) == 0) return "START";
  return "STARTING";
}
}  // namespace

'''
text = text.replace("void setup() {\n", helper + "void setup() {\n", 1)
text = text.replace(
    '''  SerialLog::infof("SYSTEM", "Boot start | project=%s | version=%s",
                   AppConfig::PROJECT_NAME, AppConfig::SOFTWARE_VERSION);

  HardwareRegistry::begin();''',
    '''  SerialLog::infof("SYSTEM", "Boot start | project=%s | version=%s",
                   AppConfig::PROJECT_NAME, AppConfig::SOFTWARE_VERSION);

  ProjectPreferences::begin();
  DisplaySh1106::setRotation180(ProjectPreferences::displayRotation180());
  DisplaySh1106::setBootStatusText(bootStatusForLanguage(ProjectPreferences::language()));
  AudioDySv17f::configureVolumePercent(ProjectPreferences::soundVolumePercent());

  HardwareRegistry::begin();''',
    1,
)
write("Unterbrechungszaehler.ino", text)

print("3.2.0 firmware patch applied")
