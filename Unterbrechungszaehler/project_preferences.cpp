#include "project_preferences.h"

#include <Preferences.h>
#include <cstring>

#include "project_config.h"
#include "serial_log.h"

namespace ProjectPreferences {
namespace {
bool sound = ProjectConfig::INTERRUPTION_SOUND_DEFAULT;
uint16_t track = ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT;
SoundMode soundModeValue = ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
bool displayEnabledValue = ProjectConfig::DISPLAY_ENABLED_DEFAULT;
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

SoundMode sanitizedSoundMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(SoundMode::Rotate)
             ? static_cast<SoundMode>(raw)
             : ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
}

DisplayMode sanitizedMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(DisplayMode::LastOnly)
             ? static_cast<DisplayMode>(raw)
             : ProjectConfig::DISPLAY_MODE_DEFAULT;
}
}  // namespace

void begin() {
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::PREF_NAMESPACE, false)) {
    SerialLog::warning("PROJECT", "Project Preferences unavailable; using defaults");
    return;
  }
  sound = prefs.getBool("sound", ProjectConfig::INTERRUPTION_SOUND_DEFAULT);
  track = prefs.getUShort("sndtrack", ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT);
  if (track < 2) track = ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT;
  soundModeValue = sanitizedSoundMode(prefs.getUChar("sndmode", static_cast<uint8_t>(ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT)));
  displayEnabledValue = prefs.getBool("dispen", ProjectConfig::DISPLAY_ENABLED_DEFAULT);
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

  SerialLog::infof("PROJECT", "Feedback settings | sound=%s | sound-mode=%s | track=%u | display=%s | display-flash=%s | display-mode=%s",
                   sound ? "ON" : "OFF", soundModeName(), static_cast<unsigned int>(track),
                   displayEnabledValue ? "ON" : "OFF", flash ? "ON" : "OFF", displayModeName());
  SerialLog::infof("PROJECT", "Display settings | brightness=%u%% | dim-after=%u min | dim-brightness=%u%%",
                   static_cast<unsigned int>(brightness), static_cast<unsigned int>(dimAfterMinutes),
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

uint16_t soundTrack() { return track; }

bool setSoundTrack(uint16_t value) {
  // Track 1 is reserved exclusively for the boot sound.
  if (value < 2) return false;
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

bool displayEnabled() { return displayEnabledValue; }

bool setDisplayEnabled(bool enabled) {
  if (displayEnabledValue == enabled) return true;
  if (!persistBool("dispen", enabled)) return false;
  displayEnabledValue = enabled;
  SerialLog::infof("PROJECT", "Display master switch changed | %s", displayEnabledValue ? "ON" : "OFF");
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
    case DisplayMode::Standard:
    default: return "standard";
  }
}

bool parseDisplayMode(const char *value, DisplayMode &parsed) {
  if (!value) return false;
  if (strcmp(value, "standard") == 0) parsed = DisplayMode::Standard;
  else if (strcmp(value, "count") == 0) parsed = DisplayMode::CountOnly;
  else if (strcmp(value, "last") == 0) parsed = DisplayMode::LastOnly;
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
