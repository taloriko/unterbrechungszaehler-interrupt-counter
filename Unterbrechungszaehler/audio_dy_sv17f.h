#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace AudioDySv17f {

enum class PlayState : uint8_t { Unknown, Stopped, Playing, Paused };

bool begin();
void update();
bool probe();

bool enabled();
bool detected();
bool checking();
StatusRegistry::State health();
uint32_t lastCheckMs();
const char *lastError();
HardwareTypes::FeedbackType feedbackType();

PlayState playState();
const char *playStateName();
bool busyKnown();
bool busy();
uint8_t onlineDevices();
uint16_t musicCount();

// Small capability API for later project modules. Control commands do not
// pretend to be confirmed: commands that can be checked schedule a targeted
// play-state query afterwards.
bool playTrack(uint16_t trackNumber);
bool playTestTone();
bool stop();
bool pause();
// High-level project volume. Mapping to the DY-SV17F 0..30 range lives here.
void configureVolumePercent(uint8_t percent);
bool setVolumePercent(uint8_t percent);
uint8_t volumePercent();
bool setVolume(uint8_t volume);

}  // namespace AudioDySv17f
