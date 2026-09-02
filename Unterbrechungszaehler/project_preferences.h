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
  LastOnly = 2
};

void begin();

bool soundEnabled();
bool setSoundEnabled(bool enabled);
uint16_t soundTrack();
bool setSoundTrack(uint16_t track);
SoundMode soundMode();
const char *soundModeName();
bool setSoundMode(SoundMode mode);
bool parseSoundMode(const char *value, SoundMode &mode);

bool displayEnabled();
bool setDisplayEnabled(bool enabled);
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
