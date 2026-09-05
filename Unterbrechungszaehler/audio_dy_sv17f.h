#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace AudioDySv17f {

enum class PlayState : uint8_t { Unknown, Stopped, Playing, Paused };

struct Diagnostics {
  bool uartResponseSeen = false;
  bool onlineDeviceKnown = false;
  bool currentDeviceKnown = false;
  bool trackCountKnown = false;
  bool currentTrackKnown = false;
  uint8_t onlineDevice = 0xFF;
  uint8_t currentDevice = 0xFF;
  uint16_t trackCount = 0;
  uint16_t currentTrack = 0;
  uint8_t desiredVolumePercent = 100;
  uint8_t lastVolumeStep = 30;
  uint8_t lastCommand = 0;
  uint16_t lastRequestedTrack = 0;
  uint32_t txFrames = 0;
  uint32_t rxFrames = 0;
  uint32_t queryTimeouts = 0;
  uint32_t checksumErrors = 0;
  uint32_t unexpectedFrames = 0;
  uint32_t playCommands = 0;
  uint32_t playRetries = 0;
  uint32_t busyConfirmedPlays = 0;
  uint32_t busyEdges = 0;
  uint32_t volumeCommands = 0;
  uint32_t lastBusyChangeMs = 0;
  uint32_t lastTxMs = 0;
  uint32_t lastRxMs = 0;
};

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
uint8_t currentDevice();
uint16_t musicCount();
uint16_t currentTrack();
const char *deviceName(uint8_t device);
const Diagnostics &diagnostics();

bool playTrack(uint16_t trackNumber);
bool playTestTone();
bool stop();
bool pause();
void configureVolumePercent(uint8_t percent);
bool setVolumePercent(uint8_t percent);
uint8_t volumePercent();
bool setVolume(uint8_t volume);

}  // namespace AudioDySv17f
