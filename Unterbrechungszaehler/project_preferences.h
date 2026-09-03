#pragma once

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
