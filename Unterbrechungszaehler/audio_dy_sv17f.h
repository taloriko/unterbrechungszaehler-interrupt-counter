#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace AudioDySv17f {

enum class PlayState : uint8_t { Unknown, Stopped, Playing, Paused };
enum class BusyPolarity : uint8_t { Unconfirmed, ActiveLow, ActiveHigh };
enum class AudioTestState : uint8_t { NotRun, Running, Ok, Partial, Warning, Error };

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
bool playStateKnown();
uint32_t playStateMeasuredAtMs();

// BUSY is diagnostic only. The raw level is reported without assuming polarity
// until a complete manual audio test has confirmed the relationship.
bool busyKnown();
bool busyLevelHigh();
uint32_t busyMeasuredAtMs();
uint32_t busyChangedAtMs();
BusyPolarity busyPolarity();
const char *busyPolarityName();

AudioTestState audioTestState();
const char *audioTestStateName();
uint32_t audioTestStartedAtMs();
uint32_t audioTestFinishedAtMs();
bool audioTestUartPlayingConfirmed();
bool audioTestUartStoppedConfirmed();
bool audioTestBusyTransitionSeen();

uint8_t onlineDevices();
uint16_t musicCount();

// Existing playback path. 3.3.1 deliberately changes diagnostics only.
bool playTrack(uint16_t trackNumber);
bool playTestTone();
bool stop();
bool pause();
void configureVolumePercent(uint8_t percent);
bool setVolumePercent(uint8_t percent);
uint8_t volumePercent();
bool setVolume(uint8_t volume);

}  // namespace AudioDySv17f
