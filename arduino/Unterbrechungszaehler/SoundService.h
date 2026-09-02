#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include "ModuleStatus.h"

enum class SoundHardwareState : uint8_t {
  Probing = 0,
  Ready,
  Lost
};

inline const char* soundHardwareStateName(SoundHardwareState state) {
  switch (state) {
    case SoundHardwareState::Probing: return "probing";
    case SoundHardwareState::Ready: return "ready";
    case SoundHardwareState::Lost: return "lost";
    default: return "probing";
  }
}

class SoundService {
public:
  void begin();
  void tick();

  bool requestPlay();
  bool requestPlay(uint16_t track);
  bool test();
  bool setSettings(bool enabled, uint8_t volume, uint16_t track);
  bool requestHardwareCheck();

  bool present() const { return hardwareState_ == SoundHardwareState::Ready; }
  bool enabled() const { return enabled_; }
  bool busy() const { return playback_ != Playback::Idle; }
  bool hardwareCheckActive() const { return hardwareProbeActive_; }
  uint32_t hardwareCheckAgeMs() const;
  uint8_t volume() const { return volume_; }
  uint16_t track() const { return track_; }
  uint8_t playState() const { return playState_; }
  bool waitingForCompletion() const { return busy(); }
  uint32_t sentCount() const { return sentCount_; }
  uint32_t completedCount() const { return completedCount_; }
  uint32_t failedCount() const { return failedCount_; }
  uint32_t timeoutCount() const { return timeoutCount_; }
  uint32_t responseCount() const { return responseCount_; }
  uint32_t errorCount() const { return errorCount_; }
  uint8_t confirmationCount() const { return confirmationCount_; }
  SoundHardwareState hardwareState() const { return hardwareState_; }
  ModuleState moduleState() const;
  const char* statusDetail() const;

private:
  enum class Playback : uint8_t {
    Idle = 0,
    AwaitingStart,
    Playing
  };

  void loadSettings();
  bool saveSettings();
  void sendFrame(uint8_t command, const uint8_t* payload = nullptr, uint8_t payloadLength = 0);
  void queryState();
  void setVolume();
  void setSingleStopMode();
  void playSpecified(uint16_t track);
  void parseSerial();
  void handleFrame(const uint8_t* frame, uint8_t length);
  void registerPlaybackStatus(uint8_t state);
  void startHardwareProbe(bool bootProbe);
  void finishHardwareProbe(bool detected);
  void failPlayback(bool timeout, const char* reason);

  HardwareSerial serial_{2};
  SoundHardwareState hardwareState_ = SoundHardwareState::Probing;
  Playback playback_ = Playback::Idle;
  bool enabled_ = false;
  bool hardwareProbeActive_ = false;
  bool playbackResponseSeen_ = false;
  uint8_t hardwareProbeResponses_ = 0;
  uint8_t hardwareProbeRequired_ = 1;
  uint8_t volume_ = 20;
  uint16_t track_ = 1;
  uint8_t playState_ = 0xFF;
  bool queryPending_ = false;
  uint8_t confirmationCount_ = 0;
  uint32_t sentCount_ = 0;
  uint32_t completedCount_ = 0;
  uint32_t failedCount_ = 0;
  uint32_t timeoutCount_ = 0;
  uint32_t responseCount_ = 0;
  uint32_t errorCount_ = 0;
  uint32_t lastQueryAt_ = 0;
  uint32_t lastResponseAt_ = 0;
  uint32_t hardwareProbeStartedAt_ = 0;
  uint32_t lastHardwareCheckAt_ = 0;
  uint32_t playbackStartedAt_ = 0;
  const char* lastPlaybackError_ = "-";
  uint8_t rx_[16] = {};
  uint8_t rxLen_ = 0;
};
