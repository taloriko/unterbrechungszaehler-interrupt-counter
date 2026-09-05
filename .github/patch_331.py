from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Unterbrechungszaehler"


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    (ROOT / rel).write_text(text, encoding="utf-8")


def replace_once(rel, old, new):
    text = read(rel)
    if old not in text:
        raise SystemExit(f"missing anchor in {rel}: {old[:120]!r}")
    write(rel, text.replace(old, new, 1))


# ---------------------------------------------------------------------------
# Version
# ---------------------------------------------------------------------------
replace_once("Unterbrechungszaehler/config.h", 'SOFTWARE_VERSION[] = "3.3.0"', 'SOFTWARE_VERSION[] = "3.3.1"')

# ---------------------------------------------------------------------------
# Audio diagnostics public API
# ---------------------------------------------------------------------------
audio_h = r'''#pragma once

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
'''
write("Unterbrechungszaehler/audio_dy_sv17f.h", audio_h)

# ---------------------------------------------------------------------------
# DY-SV17F diagnostics implementation
# - no permanent BUSY polling
# - silent manual probe queries UART + takes one BUSY snapshot
# - manual tone test uses existing 0x07 playback command, verifies UART PLAY,
#   observes BUSY edges only while the test is active, and confirms STOP by UART
# ---------------------------------------------------------------------------
audio_cpp = r'''#include "audio_dy_sv17f.h"

#include <HardwareSerial.h>

#include "hardware_config.h"
#include "serial_log.h"
#include "status_registry.h"

namespace AudioDySv17f {
namespace {

HardwareSerial audioSerial(HardwareConfig::AUDIO_UART_PORT);
StatusRegistry::State moduleHealth = StatusRegistry::State::Unknown;
bool isDetected = false;
bool probeActive = false;
bool probeHadSecondaryFailure = false;
bool probeWasBoot = false;
uint32_t checkedAtMs = 0;
const char *errorText = "";
PlayState currentPlayState = PlayState::Unknown;
bool currentPlayStateKnown = false;
uint32_t currentPlayStateMeasuredAtMs = 0;
uint8_t devicesOnline = 0;
uint16_t tracks = 0;
bool uartReady = false;
uint8_t desiredVolumePercent = 100;
bool volumePending = true;

bool busyLevelKnown = false;
bool currentBusyLevelHigh = false;
uint32_t currentBusyMeasuredAtMs = 0;
uint32_t currentBusyChangedAtMs = 0;
BusyPolarity confirmedBusyPolarity = BusyPolarity::Unconfirmed;
volatile bool busyIrqArmed = false;
volatile bool busyIrqPending = false;

AudioTestState lastAudioTestState = AudioTestState::NotRun;
bool manualTestActive = false;
uint32_t manualTestStartedAt = 0;
uint32_t manualTestFinishedAt = 0;
bool testUartPlaying = false;
bool testUartStopped = false;
bool testBusyTransition = false;
bool testBusyBeforeKnown = false;
bool testBusyBeforeHigh = false;
bool testBusyDuringKnown = false;
bool testBusyDuringHigh = false;
bool testBusyEndKnown = false;
bool testBusyEndHigh = false;

enum class WaitKind : uint8_t {
  None,
  ProbePlay,
  ProbeDevices,
  ProbeCount,
  VerifyPlay,
  TestPrePlay,
  TestPlaying,
  TestTransition
};
enum class VerifyExpectation : uint8_t { Any, Playing, Stopped };
WaitKind waitingFor = WaitKind::None;
VerifyExpectation verifyExpectation = VerifyExpectation::Any;
uint8_t expectedCommand = 0;
uint32_t responseDeadlineMs = 0;

enum class DeferredAction : uint8_t {
  None,
  StartProbe,
  QueryDevices,
  QueryCount,
  VerifyPlay,
  BootTone,
  TestPlay,
  TestVerifyPlaying
};
DeferredAction deferredAction = DeferredAction::None;
uint32_t deferredAtMs = 0;

uint8_t rxBuffer[16]{};
size_t rxLength = 0;
size_t rxExpected = 0;

bool due(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

void setHealth(StatusRegistry::State state, const char *message = "") {
  moduleHealth = state;
  errorText = message ? message : "";
  StatusRegistry::setState("audio", state);
}

uint8_t checksum(const uint8_t *data, size_t length) {
  uint16_t sum = 0;
  for (size_t i = 0; i < length; ++i) sum += data[i];
  return static_cast<uint8_t>(sum & 0xFF);
}

void sendFrame(uint8_t command, const uint8_t *data = nullptr, uint8_t length = 0) {
  uint8_t frame[12];
  const size_t total = static_cast<size_t>(length) + 4U;
  if (total > sizeof(frame)) return;
  frame[0] = 0xAA;
  frame[1] = command;
  frame[2] = length;
  for (uint8_t i = 0; i < length; ++i) frame[3 + i] = data[i];
  frame[3 + length] = checksum(frame, 3U + length);
  audioSerial.write(frame, total);
}

void startWait(WaitKind kind, uint8_t command) {
  waitingFor = kind;
  expectedCommand = command;
  responseDeadlineMs = millis() + HardwareConfig::AUDIO_RESPONSE_TIMEOUT_MS;
}

void sendQuery(WaitKind kind, uint8_t command) {
  sendFrame(command);
  startWait(kind, command);
}

bool transportIdle() {
  return !probeActive && waitingFor == WaitKind::None && deferredAction == DeferredAction::None;
}

bool commandPathIdle() {
  return transportIdle() && !manualTestActive;
}

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

void setPlayStateFromByte(uint8_t value) {
  currentPlayState = value == 0x01 ? PlayState::Playing : value == 0x02 ? PlayState::Paused : PlayState::Stopped;
  currentPlayStateKnown = true;
  currentPlayStateMeasuredAtMs = millis();
  isDetected = true;
}

void sampleBusyNow(bool edge = false) {
  if (HardwareConfig::AUDIO_BUSY_PIN < 0) return;
  const bool levelHigh = digitalRead(HardwareConfig::AUDIO_BUSY_PIN) == HIGH;
  const uint32_t now = millis();
  if (edge || (busyLevelKnown && levelHigh != currentBusyLevelHigh)) currentBusyChangedAtMs = now;
  currentBusyLevelHigh = levelHigh;
  busyLevelKnown = true;
  currentBusyMeasuredAtMs = now;
}

void IRAM_ATTR onBusyEdge() {
  if (busyIrqArmed) busyIrqPending = true;
}

void disarmBusyTestMonitor() {
  busyIrqArmed = false;
  busyIrqPending = false;
}

void resetAudioTest() {
  lastAudioTestState = AudioTestState::Running;
  manualTestActive = true;
  manualTestStartedAt = millis();
  manualTestFinishedAt = 0;
  testUartPlaying = false;
  testUartStopped = false;
  testBusyTransition = false;
  testBusyBeforeKnown = false;
  testBusyBeforeHigh = false;
  testBusyDuringKnown = false;
  testBusyDuringHigh = false;
  testBusyEndKnown = false;
  testBusyEndHigh = false;
}

void finishAudioTest(AudioTestState state, StatusRegistry::State healthState, const char *message) {
  lastAudioTestState = state;
  manualTestActive = false;
  manualTestFinishedAt = millis();
  checkedAtMs = manualTestFinishedAt;
  waitingFor = WaitKind::None;
  deferredAction = DeferredAction::None;
  disarmBusyTestMonitor();
  setHealth(healthState, message);

  SerialLog::infof("AUDIO", "AUDIO TEST RESULT | result=%s | uart_play=%s | uart_stop=%s | busy_edge=%s | polarity=%s",
                   audioTestStateName(), testUartPlaying ? "yes" : "no", testUartStopped ? "yes" : "no",
                   testBusyTransition ? "yes" : "no", busyPolarityName());
}

void finishProbe(StatusRegistry::State state, const char *message = "") {
  const bool shouldPlayBootTone = probeWasBoot && isDetected && HardwareConfig::AUDIO_BOOT_TONE_ENABLED;
  probeWasBoot = false;
  probeActive = false;
  waitingFor = WaitKind::None;
  checkedAtMs = millis();
  setHealth(state, message);

  if (state == StatusRegistry::State::Ok) {
    SerialLog::successf("AUDIO", "DY-SV17F: OK | uart_play=%s | online=0x%02X | files=%u | BUSY=%s",
                        playStateName(), devicesOnline, tracks,
                        busyLevelKnown ? (currentBusyLevelHigh ? "HIGH" : "LOW") : "n/a");
  } else if (state == StatusRegistry::State::Warning) {
    SerialLog::warningf("AUDIO", "DY-SV17F: WARNING | communication confirmed | optional query missing | play=%s",
                        playStateName());
  } else {
    SerialLog::error("AUDIO", "DY-SV17F: NO RESPONSE | UART play-state query timed out");
  }

  applyDesiredVolume();

  if (shouldPlayBootTone) {
    deferredAction = DeferredAction::BootTone;
    deferredAtMs = millis() + HardwareConfig::AUDIO_BOOT_TONE_DELAY_MS;
    SerialLog::infof("AUDIO", "Boot tone scheduled | track=%u | delay=%lu ms",
                     HardwareConfig::AUDIO_BOOT_TONE_TRACK,
                     static_cast<unsigned long>(HardwareConfig::AUDIO_BOOT_TONE_DELAY_MS));
  }
}

void scheduleVerify(uint32_t delayMs, VerifyExpectation expectation) {
  verifyExpectation = expectation;
  deferredAction = DeferredAction::VerifyPlay;
  deferredAtMs = millis() + delayMs;
  setHealth(StatusRegistry::State::Checking);
}

void sendPlayCommand(uint16_t trackNumber) {
  const uint8_t data[2] = {static_cast<uint8_t>((trackNumber >> 8) & 0xFF),
                           static_cast<uint8_t>(trackNumber & 0xFF)};
  sendFrame(0x07, data, sizeof(data));
  SerialLog::infof("AUDIO", "Play track command sent | track=%u", trackNumber);
}

void handleFrame(uint8_t command, const uint8_t *data, uint8_t length) {
  if (command != expectedCommand || waitingFor == WaitKind::None) return;

  switch (waitingFor) {
    case WaitKind::ProbePlay:
      if (length != 1) return;
      setPlayStateFromByte(data[0]);
      waitingFor = WaitKind::None;
      deferredAction = DeferredAction::QueryDevices;
      deferredAtMs = millis() + HardwareConfig::AUDIO_INTER_COMMAND_DELAY_MS;
      break;

    case WaitKind::ProbeDevices:
      if (length == 1) devicesOnline = data[0];
      else probeHadSecondaryFailure = true;
      waitingFor = WaitKind::None;
      deferredAction = DeferredAction::QueryCount;
      deferredAtMs = millis() + HardwareConfig::AUDIO_INTER_COMMAND_DELAY_MS;
      break;

    case WaitKind::ProbeCount:
      if (length == 2) tracks = static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
      else probeHadSecondaryFailure = true;
      finishProbe(probeHadSecondaryFailure ? StatusRegistry::State::Warning : StatusRegistry::State::Ok,
                  probeHadSecondaryFailure ? "optional query failed" : "");
      break;

    case WaitKind::VerifyPlay: {
      if (length != 1) return;
      setPlayStateFromByte(data[0]);
      waitingFor = WaitKind::None;
      const bool expectedPlaying = verifyExpectation == VerifyExpectation::Playing;
      const bool expectedStopped = verifyExpectation == VerifyExpectation::Stopped;
      const bool matches = (!expectedPlaying && !expectedStopped) ||
                           (expectedPlaying && currentPlayState == PlayState::Playing) ||
                           (expectedStopped && currentPlayState == PlayState::Stopped);
      verifyExpectation = VerifyExpectation::Any;
      if (matches) {
        setHealth(StatusRegistry::State::Ok);
        SerialLog::successf("AUDIO", "Command verification OK | play-state=%s", playStateName());
      } else {
        setHealth(StatusRegistry::State::Warning, "play-state does not match command");
        SerialLog::warningf("AUDIO", "Command answered, but play-state does not match expectation | play-state=%s", playStateName());
      }
      break;
    }

    case WaitKind::TestPrePlay:
      if (length != 1) return;
      setPlayStateFromByte(data[0]);
      waitingFor = WaitKind::None;
      if (currentPlayState != PlayState::Stopped) {
        finishAudioTest(AudioTestState::Warning, StatusRegistry::State::Warning,
                        "audio test requires stopped module");
        return;
      }
      sampleBusyNow();
      testBusyBeforeKnown = busyLevelKnown;
      testBusyBeforeHigh = currentBusyLevelHigh;
      busyIrqArmed = HardwareConfig::AUDIO_BUSY_PIN >= 0;
      deferredAction = DeferredAction::TestPlay;
      deferredAtMs = millis() + HardwareConfig::AUDIO_INTER_COMMAND_DELAY_MS;
      SerialLog::infof("AUDIO", "AUDIO TEST START | UART before=stopped | BUSY before=%s",
                       testBusyBeforeKnown ? (testBusyBeforeHigh ? "HIGH" : "LOW") : "n/a");
      break;

    case WaitKind::TestPlaying:
      if (length != 1) return;
      setPlayStateFromByte(data[0]);
      waitingFor = WaitKind::None;
      if (currentPlayState != PlayState::Playing) {
        finishAudioTest(AudioTestState::Warning, StatusRegistry::State::Warning,
                        "module did not report playback after test command");
        return;
      }
      testUartPlaying = true;
      sampleBusyNow();
      testBusyDuringKnown = busyLevelKnown;
      testBusyDuringHigh = currentBusyLevelHigh;
      setHealth(StatusRegistry::State::Checking);
      SerialLog::successf("AUDIO", "AUDIO TEST | UART playback confirmed | BUSY now=%s",
                          testBusyDuringKnown ? (testBusyDuringHigh ? "HIGH" : "LOW") : "n/a");
      break;

    case WaitKind::TestTransition:
      if (length != 1) return;
      setPlayStateFromByte(data[0]);
      waitingFor = WaitKind::None;
      sampleBusyNow();
      if (currentPlayState == PlayState::Playing) {
        testBusyDuringKnown = busyLevelKnown;
        testBusyDuringHigh = currentBusyLevelHigh;
        setHealth(StatusRegistry::State::Checking);
        SerialLog::infof("AUDIO", "AUDIO TEST | BUSY edge checked by UART | still playing | BUSY=%s",
                         testBusyDuringKnown ? (testBusyDuringHigh ? "HIGH" : "LOW") : "n/a");
        return;
      }
      if (currentPlayState == PlayState::Stopped) {
        testUartStopped = true;
        testBusyEndKnown = busyLevelKnown;
        testBusyEndHigh = currentBusyLevelHigh;
        const bool completeCycle = testBusyBeforeKnown && testBusyDuringKnown && testBusyEndKnown &&
                                   testBusyBeforeHigh == testBusyEndHigh &&
                                   testBusyDuringHigh != testBusyEndHigh;
        if (completeCycle) {
          confirmedBusyPolarity = testBusyDuringHigh ? BusyPolarity::ActiveHigh : BusyPolarity::ActiveLow;
          finishAudioTest(AudioTestState::Ok, StatusRegistry::State::Ok, "");
        } else {
          confirmedBusyPolarity = BusyPolarity::Unconfirmed;
          finishAudioTest(AudioTestState::Partial, StatusRegistry::State::Warning,
                          "UART playback works; BUSY polarity not confirmed");
        }
        return;
      }
      finishAudioTest(AudioTestState::Warning, StatusRegistry::State::Warning,
                      "unexpected playback state during audio test");
      break;

    case WaitKind::None:
    default:
      break;
  }
}

void feedParser(uint8_t value) {
  if (rxLength == 0) {
    if (value != 0xAA) return;
    rxBuffer[rxLength++] = value;
    rxExpected = 0;
    return;
  }

  if (rxLength >= sizeof(rxBuffer)) {
    rxLength = 0;
    rxExpected = 0;
    return;
  }

  rxBuffer[rxLength++] = value;
  if (rxLength == 3) {
    rxExpected = static_cast<size_t>(rxBuffer[2]) + 4U;
    if (rxExpected > sizeof(rxBuffer) || rxExpected < 4U) {
      rxLength = 0;
      rxExpected = 0;
      return;
    }
  }

  if (rxExpected == 0 || rxLength < rxExpected) return;
  const uint8_t expectedChecksum = checksum(rxBuffer, rxExpected - 1U);
  const uint8_t actualChecksum = rxBuffer[rxExpected - 1U];
  if (expectedChecksum == actualChecksum) {
    handleFrame(rxBuffer[1], &rxBuffer[3], rxBuffer[2]);
  } else {
    SerialLog::warning("AUDIO", "Ignored DY-SV17F frame with invalid checksum");
  }
  rxLength = 0;
  rxExpected = 0;
}

void handleTimeout() {
  if (waitingFor == WaitKind::None || !due(millis(), responseDeadlineMs)) return;
  const WaitKind timedOut = waitingFor;
  waitingFor = WaitKind::None;

  if (timedOut == WaitKind::ProbePlay) {
    isDetected = false;
    currentPlayState = PlayState::Unknown;
    currentPlayStateKnown = false;
    finishProbe(StatusRegistry::State::NoResponse, "play-state query timeout");
    return;
  }
  if (timedOut == WaitKind::ProbeDevices) {
    probeHadSecondaryFailure = true;
    devicesOnline = 0;
    deferredAction = DeferredAction::QueryCount;
    deferredAtMs = millis() + HardwareConfig::AUDIO_INTER_COMMAND_DELAY_MS;
    return;
  }
  if (timedOut == WaitKind::ProbeCount) {
    probeHadSecondaryFailure = true;
    tracks = 0;
    finishProbe(StatusRegistry::State::Warning, "optional query timeout");
    return;
  }
  if (timedOut == WaitKind::VerifyPlay) {
    verifyExpectation = VerifyExpectation::Any;
    setHealth(StatusRegistry::State::NoResponse, "command verification timeout");
    SerialLog::error("AUDIO", "Command verification failed | UART play-state query timed out");
    return;
  }
  if (timedOut == WaitKind::TestPrePlay || timedOut == WaitKind::TestPlaying || timedOut == WaitKind::TestTransition) {
    finishAudioTest(AudioTestState::Error, StatusRegistry::State::NoResponse,
                    "audio test UART query timeout");
  }
}

void serviceBusyEdge() {
  if (!busyIrqPending) return;
  busyIrqPending = false;
  if (!manualTestActive) return;

  sampleBusyNow(true);
  testBusyTransition = true;
  SerialLog::infof("AUDIO", "AUDIO TEST | BUSY edge | GPIO%d=%s",
                   HardwareConfig::AUDIO_BUSY_PIN, currentBusyLevelHigh ? "HIGH" : "LOW");

  if (testUartPlaying && waitingFor == WaitKind::None && deferredAction == DeferredAction::None) {
    sendQuery(WaitKind::TestTransition, 0x01);
  }
}

bool startProbe(bool bootProbe) {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F) {
    setHealth(StatusRegistry::State::Disabled);
    return false;
  }
  if (!bootProbe && !commandPathIdle()) {
    SerialLog::warning("AUDIO", "Health check rejected | audio command/verification is still active");
    return false;
  }
  while (audioSerial.available() > 0) audioSerial.read();
  rxLength = 0;
  rxExpected = 0;
  deferredAction = DeferredAction::None;
  probeActive = true;
  probeWasBoot = bootProbe;
  probeHadSecondaryFailure = false;
  sampleBusyNow();  // one diagnostic snapshot; no permanent polling
  setHealth(StatusRegistry::State::Checking);
  sendQuery(WaitKind::ProbePlay, 0x01);
  SerialLog::infof("AUDIO", "%s | querying play state | BUSY snapshot=%s",
                   bootProbe ? "DY-SV17F boot health check started" : "DY-SV17F health check started",
                   busyLevelKnown ? (currentBusyLevelHigh ? "HIGH" : "LOW") : "n/a");
  return true;
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("audio", "status.audio", "audio", HardwareConfig::ENABLE_AUDIO_DY_SV17F);
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F) {
    setHealth(StatusRegistry::State::Disabled);
    StatusRegistry::setVisible("audio", false);
    return false;
  }

  if (HardwareConfig::AUDIO_BUSY_PIN >= 0) {
    pinMode(HardwareConfig::AUDIO_BUSY_PIN, INPUT);
    sampleBusyNow();
    attachInterrupt(digitalPinToInterrupt(HardwareConfig::AUDIO_BUSY_PIN), onBusyEdge, CHANGE);
  }
  audioSerial.begin(HardwareConfig::AUDIO_BAUD_RATE, SERIAL_8N1,
                    HardwareConfig::AUDIO_RX_PIN, HardwareConfig::AUDIO_TX_PIN);
  uartReady = true;
  volumePending = true;
  SerialLog::infof("AUDIO", "DY-SV17F UART ready | UART%u | RX=%d | TX=%d | BUSY=%d | %lu baud",
                   HardwareConfig::AUDIO_UART_PORT, HardwareConfig::AUDIO_RX_PIN,
                   HardwareConfig::AUDIO_TX_PIN, HardwareConfig::AUDIO_BUSY_PIN,
                   static_cast<unsigned long>(HardwareConfig::AUDIO_BAUD_RATE));
  setHealth(StatusRegistry::State::Checking);
  deferredAction = DeferredAction::StartProbe;
  deferredAtMs = millis() + HardwareConfig::AUDIO_BOOT_GRACE_MS;
  SerialLog::infof("AUDIO", "DY-SV17F boot grace | first query in %lu ms",
                   static_cast<unsigned long>(HardwareConfig::AUDIO_BOOT_GRACE_MS));
  return true;
}

void update() {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F) return;

  while (audioSerial.available() > 0) feedParser(static_cast<uint8_t>(audioSerial.read()));
  handleTimeout();
  serviceBusyEdge();
  if (volumePending && commandPathIdle()) applyDesiredVolume();

  const uint32_t now = millis();
  if (manualTestActive && due(now, manualTestStartedAt + HardwareConfig::AUDIO_DIAGNOSTIC_TEST_TIMEOUT_MS) &&
      waitingFor == WaitKind::None && deferredAction == DeferredAction::None) {
    finishAudioTest(testUartPlaying ? AudioTestState::Partial : AudioTestState::Warning,
                    StatusRegistry::State::Warning,
                    "audio test timeout; track end was not confirmed");
  }

  if (deferredAction != DeferredAction::None && due(now, deferredAtMs)) {
    const DeferredAction action = deferredAction;
    deferredAction = DeferredAction::None;

    if (action == DeferredAction::StartProbe) {
      if (!probeActive && waitingFor == WaitKind::None) startProbe(true);
    } else if (action == DeferredAction::QueryDevices) {
      if (probeActive && waitingFor == WaitKind::None) sendQuery(WaitKind::ProbeDevices, 0x09);
    } else if (action == DeferredAction::QueryCount) {
      if (probeActive && waitingFor == WaitKind::None) sendQuery(WaitKind::ProbeCount, 0x0C);
    } else if (action == DeferredAction::VerifyPlay) {
      if (!probeActive && !manualTestActive && waitingFor == WaitKind::None) sendQuery(WaitKind::VerifyPlay, 0x01);
    } else if (action == DeferredAction::BootTone) {
      if (commandPathIdle()) {
        SerialLog::infof("AUDIO", "Boot tone | track=%u", HardwareConfig::AUDIO_BOOT_TONE_TRACK);
        playTrack(HardwareConfig::AUDIO_BOOT_TONE_TRACK);
      }
    } else if (action == DeferredAction::TestPlay) {
      if (manualTestActive && waitingFor == WaitKind::None) {
        sendPlayCommand(HardwareConfig::AUDIO_TEST_TRACK);
        deferredAction = DeferredAction::TestVerifyPlaying;
        deferredAtMs = millis() + HardwareConfig::AUDIO_COMMAND_VERIFY_DELAY_MS;
      }
    } else if (action == DeferredAction::TestVerifyPlaying) {
      if (manualTestActive && waitingFor == WaitKind::None) sendQuery(WaitKind::TestPlaying, 0x01);
    }
  }
}

bool probe() { return startProbe(false); }

bool enabled() { return HardwareConfig::ENABLE_AUDIO_DY_SV17F; }
bool detected() { return isDetected; }
bool checking() {
  return probeActive || manualTestActive || waitingFor != WaitKind::None ||
         deferredAction == DeferredAction::StartProbe ||
         deferredAction == DeferredAction::QueryDevices ||
         deferredAction == DeferredAction::QueryCount ||
         deferredAction == DeferredAction::VerifyPlay ||
         deferredAction == DeferredAction::TestPlay ||
         deferredAction == DeferredAction::TestVerifyPlaying;
}
StatusRegistry::State health() { return moduleHealth; }
uint32_t lastCheckMs() { return checkedAtMs; }
const char *lastError() { return errorText; }
HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::ProtocolResponse; }
PlayState playState() { return currentPlayState; }

const char *playStateName() {
  switch (currentPlayState) {
    case PlayState::Stopped: return "stopped";
    case PlayState::Playing: return "playing";
    case PlayState::Paused: return "paused";
    case PlayState::Unknown:
    default: return "unknown";
  }
}

bool playStateKnown() { return currentPlayStateKnown; }
uint32_t playStateMeasuredAtMs() { return currentPlayStateMeasuredAtMs; }
bool busyKnown() { return busyLevelKnown; }
bool busyLevelHigh() { return currentBusyLevelHigh; }
uint32_t busyMeasuredAtMs() { return currentBusyMeasuredAtMs; }
uint32_t busyChangedAtMs() { return currentBusyChangedAtMs; }
BusyPolarity busyPolarity() { return confirmedBusyPolarity; }

const char *busyPolarityName() {
  switch (confirmedBusyPolarity) {
    case BusyPolarity::ActiveLow: return "active_low";
    case BusyPolarity::ActiveHigh: return "active_high";
    case BusyPolarity::Unconfirmed:
    default: return "unconfirmed";
  }
}

AudioTestState audioTestState() { return lastAudioTestState; }
const char *audioTestStateName() {
  switch (lastAudioTestState) {
    case AudioTestState::Running: return "running";
    case AudioTestState::Ok: return "ok";
    case AudioTestState::Partial: return "partial";
    case AudioTestState::Warning: return "warning";
    case AudioTestState::Error: return "error";
    case AudioTestState::NotRun:
    default: return "not_run";
  }
}
uint32_t audioTestStartedAtMs() { return manualTestStartedAt; }
uint32_t audioTestFinishedAtMs() { return manualTestFinishedAt; }
bool audioTestUartPlayingConfirmed() { return testUartPlaying; }
bool audioTestUartStoppedConfirmed() { return testUartStopped; }
bool audioTestBusyTransitionSeen() { return testBusyTransition; }
uint8_t onlineDevices() { return devicesOnline; }
uint16_t musicCount() { return tracks; }

void configureVolumePercent(uint8_t percent) {
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

bool playTrack(uint16_t trackNumber) {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || trackNumber == 0 || !commandPathIdle()) return false;
  // Existing playback command deliberately remains unchanged.
  sendPlayCommand(trackNumber);
  scheduleVerify(HardwareConfig::AUDIO_COMMAND_VERIFY_DELAY_MS, VerifyExpectation::Playing);
  return true;
}

bool playTestTone() {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || !commandPathIdle()) return false;
  resetAudioTest();
  sampleBusyNow();
  testBusyBeforeKnown = busyLevelKnown;
  testBusyBeforeHigh = currentBusyLevelHigh;
  setHealth(StatusRegistry::State::Checking);
  sendQuery(WaitKind::TestPrePlay, 0x01);
  SerialLog::infof("AUDIO", "Manual audio test requested | track=%u | silent pre-check started",
                   HardwareConfig::AUDIO_TEST_TRACK);
  return true;
}

bool stop() {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || !commandPathIdle()) return false;
  sendFrame(0x04);
  scheduleVerify(HardwareConfig::AUDIO_COMMAND_VERIFY_DELAY_MS, VerifyExpectation::Stopped);
  return true;
}

bool pause() {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || !commandPathIdle()) return false;
  sendFrame(0x03);
  scheduleVerify(HardwareConfig::AUDIO_COMMAND_VERIFY_DELAY_MS, VerifyExpectation::Any);
  return true;
}

bool setVolume(uint8_t volume) {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || volume > 30 || !commandPathIdle()) return false;
  sendFrame(0x13, &volume, 1);
  SerialLog::infof("AUDIO", "Volume command sent | volume=%u | no protocol response expected", volume);
  return true;
}

}  // namespace AudioDySv17f
'''
write("Unterbrechungszaehler/audio_dy_sv17f.cpp", audio_cpp)

# Diagnostic timeout, not a claimed track duration.
replace_once(
    "Unterbrechungszaehler/hardware_config.h",
    "constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;\n",
    "constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;\nconstexpr uint32_t AUDIO_DIAGNOSTIC_TEST_TIMEOUT_MS = 120000; // safety timeout only; not a track-duration assumption\n",
)

# ---------------------------------------------------------------------------
# Hardware JSON: separate UART response, raw BUSY level, measurement times,
# polarity confidence and the last manual audio-test result.
# ---------------------------------------------------------------------------
hw_path = "Unterbrechungszaehler/hardware_registry.cpp"
hw = read(hw_path)
old_audio = '''  if (AudioDySv17f::enabled()) {\n    beginModule(out, firstModule, "audio", "hardware.audio", "audio", true, effectiveHealth(AudioDySv17f::health()),\n                AudioDySv17f::feedbackType(), effectiveCheckedAt(AudioDySv17f::lastCheckMs()), effectiveError(AudioDySv17f::lastError()));\n    bool first = true;\n    appendInfoString(out, first, "hardware.info.model", "DY-SV17F");\n    appendInfoString(out, first, "hardware.info.transport", "UART2 / 9600 8N1");\n    appendInfoString(out, first, "hardware.info.pins",\n                     String("DY TX -> GPIO") + String(static_cast<int>(HardwareConfig::AUDIO_RX_PIN)) + " (ESP RX), DY RX <- GPIO" + String(static_cast<int>(HardwareConfig::AUDIO_TX_PIN)) + " (ESP TX), BUSY -> GPIO" + String(static_cast<int>(HardwareConfig::AUDIO_BUSY_PIN)) + "/VN");\n    if (AudioDySv17f::detected()) {\n      appendInfoString(out, first, "hardware.info.playState", String(AudioDySv17f::playStateName()), "stateKey");\n      appendInfoString(out, first, "hardware.info.onlineDevices", hexByte(AudioDySv17f::onlineDevices()));\n      appendInfoUInt(out, first, "hardware.info.fileCount", AudioDySv17f::musicCount());\n    }\n    if (AudioDySv17f::busyKnown()) appendInfoBool(out, first, "hardware.info.busy", AudioDySv17f::busy());\n    appendInfoUInt(out, first, "hardware.info.testTrack", HardwareConfig::AUDIO_TEST_TRACK);\n    endModule(out, "test", "action.audioTest", "audio");\n  }'''
new_audio = '''  if (AudioDySv17f::enabled()) {\n    beginModule(out, firstModule, "audio", "hardware.audio", "audio", true, effectiveHealth(AudioDySv17f::health()),\n                AudioDySv17f::feedbackType(), effectiveCheckedAt(AudioDySv17f::lastCheckMs()), effectiveError(AudioDySv17f::lastError()));\n    bool first = true;\n    appendInfoString(out, first, "hardware.info.model", "DY-SV17F");\n    appendInfoString(out, first, "hardware.info.transport", "UART2 / 9600 8N1");\n    appendInfoString(out, first, "hardware.info.pins",\n                     String("DY TX -> GPIO") + String(static_cast<int>(HardwareConfig::AUDIO_RX_PIN)) + " (ESP RX), DY RX <- GPIO" + String(static_cast<int>(HardwareConfig::AUDIO_TX_PIN)) + " (ESP TX), BUSY -> GPIO" + String(static_cast<int>(HardwareConfig::AUDIO_BUSY_PIN)) + "/VN");\n    appendInfoString(out, first, "hardware.info.uartCommunication", AudioDySv17f::detected() ? "ok" : "no_response", "statusKey");\n    appendInfoString(out, first, "hardware.info.playState", AudioDySv17f::playStateKnown() ? String(AudioDySv17f::playStateName()) : String("unknown"), "stateKey");\n    if (AudioDySv17f::playStateMeasuredAtMs() > 0) appendInfoUInt(out, first, "hardware.info.playStateMeasured", AudioDySv17f::playStateMeasuredAtMs(), "checkTime");\n    if (AudioDySv17f::detected()) {\n      appendInfoString(out, first, "hardware.info.mediaStatus", "response", "mediaStatus");\n      appendInfoString(out, first, "hardware.info.mediaRaw", hexByte(AudioDySv17f::onlineDevices()));\n      appendInfoUInt(out, first, "hardware.info.fileCount", AudioDySv17f::musicCount());\n    }\n    if (AudioDySv17f::busyKnown()) {\n      appendInfoString(out, first, "hardware.info.busyLevel", AudioDySv17f::busyLevelHigh() ? "HIGH" : "LOW");\n      appendInfoUInt(out, first, "hardware.info.busyMeasured", AudioDySv17f::busyMeasuredAtMs(), "checkTime");\n      if (AudioDySv17f::busyChangedAtMs() > 0) appendInfoUInt(out, first, "hardware.info.busyChanged", AudioDySv17f::busyChangedAtMs(), "checkTime");\n    }\n    appendInfoString(out, first, "hardware.info.busyInterpretation", AudioDySv17f::busyPolarityName(), "busyPolarity");\n    appendInfoString(out, first, "hardware.info.audioTest", AudioDySv17f::audioTestStateName(), "audioTestState");\n    if (AudioDySv17f::audioTestStartedAtMs() > 0) appendInfoUInt(out, first, "hardware.info.audioTestStarted", AudioDySv17f::audioTestStartedAtMs(), "checkTime");\n    if (AudioDySv17f::audioTestState() != AudioDySv17f::AudioTestState::NotRun) {\n      appendInfoBool(out, first, "hardware.info.uartPlayConfirmed", AudioDySv17f::audioTestUartPlayingConfirmed());\n      appendInfoBool(out, first, "hardware.info.busyEdgeSeen", AudioDySv17f::audioTestBusyTransitionSeen());\n      appendInfoBool(out, first, "hardware.info.uartStopConfirmed", AudioDySv17f::audioTestUartStoppedConfirmed());\n    }\n    appendInfoUInt(out, first, "hardware.info.testTrack", HardwareConfig::AUDIO_TEST_TRACK);\n    endModule(out, "test", "action.audioTest", "audio");\n  }'''
if old_audio not in hw:
    raise SystemExit("hardware audio block anchor missing")
hw = hw.replace(old_audio, new_audio, 1)
write(hw_path, hw)

# ---------------------------------------------------------------------------
# UI translations and generic formatters for the richer hardware fields.
# ---------------------------------------------------------------------------
app_path = "Unterbrechungszaehler/ui-src/app.js"
js = read(app_path)
insert_after = "  Object.entries(I18N_330).forEach(([code, labels]) => Object.assign(I18N[code], labels));\n"
if insert_after not in js:
    raise SystemExit("I18N_330 anchor missing")

overlay = r'''

  const I18N_331 = {
    de: {
      'hardware.info.uartCommunication': 'UART-Kommunikation', 'hardware.info.playState': 'Wiedergabe laut Modul',
      'hardware.info.playStateMeasured': 'Letzte Statusabfrage', 'hardware.info.mediaStatus': 'Datenträger', 'hardware.info.mediaRaw': 'Datenträger-Rohwert',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY gemessen', 'hardware.info.busyChanged': 'Letzte BUSY-Änderung',
      'hardware.info.busyInterpretation': 'BUSY-Auswertung', 'hardware.info.audioTest': 'Audiotest', 'hardware.info.audioTestStarted': 'Audiotest gestartet',
      'hardware.info.uartPlayConfirmed': 'UART PLAY bestätigt', 'hardware.info.uartStopConfirmed': 'UART Ende bestätigt', 'hardware.info.busyEdgeSeen': 'BUSY-Flanke erkannt',
      'hardware.media.response': 'Antwort erhalten', 'hardware.busy.unconfirmed': 'Nicht bestätigt', 'hardware.busy.active_low': 'LOW = Wiedergabe', 'hardware.busy.active_high': 'HIGH = Wiedergabe',
      'hardware.audioTest.not_run': 'Nicht durchgeführt', 'hardware.audioTest.running': 'Läuft', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'Unvollständig',
      'hardware.audioTest.warning': 'Warnung', 'hardware.audioTest.error': 'Fehler'
    },
    en: {
      'hardware.info.uartCommunication': 'UART communication', 'hardware.info.playState': 'Playback reported by module',
      'hardware.info.playStateMeasured': 'Last status query', 'hardware.info.mediaStatus': 'Media', 'hardware.info.mediaRaw': 'Media raw value',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY sampled', 'hardware.info.busyChanged': 'Last BUSY change',
      'hardware.info.busyInterpretation': 'BUSY interpretation', 'hardware.info.audioTest': 'Audio test', 'hardware.info.audioTestStarted': 'Audio test started',
      'hardware.info.uartPlayConfirmed': 'UART PLAY confirmed', 'hardware.info.uartStopConfirmed': 'UART end confirmed', 'hardware.info.busyEdgeSeen': 'BUSY edge seen',
      'hardware.media.response': 'Response received', 'hardware.busy.unconfirmed': 'Not confirmed', 'hardware.busy.active_low': 'LOW = playback', 'hardware.busy.active_high': 'HIGH = playback',
      'hardware.audioTest.not_run': 'Not run', 'hardware.audioTest.running': 'Running', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'Incomplete',
      'hardware.audioTest.warning': 'Warning', 'hardware.audioTest.error': 'Error'
    },
    it: {
      'hardware.info.uartCommunication': 'Comunicazione UART', 'hardware.info.playState': 'Riproduzione secondo il modulo',
      'hardware.info.playStateMeasured': 'Ultima richiesta stato', 'hardware.info.mediaStatus': 'Supporto', 'hardware.info.mediaRaw': 'Valore supporto grezzo',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY misurato', 'hardware.info.busyChanged': 'Ultimo cambio BUSY',
      'hardware.info.busyInterpretation': 'Interpretazione BUSY', 'hardware.info.audioTest': 'Test audio', 'hardware.info.audioTestStarted': 'Test audio avviato',
      'hardware.info.uartPlayConfirmed': 'PLAY UART confermato', 'hardware.info.uartStopConfirmed': 'Fine UART confermata', 'hardware.info.busyEdgeSeen': 'Transizione BUSY rilevata',
      'hardware.media.response': 'Risposta ricevuta', 'hardware.busy.unconfirmed': 'Non confermato', 'hardware.busy.active_low': 'LOW = riproduzione', 'hardware.busy.active_high': 'HIGH = riproduzione',
      'hardware.audioTest.not_run': 'Non eseguito', 'hardware.audioTest.running': 'In corso', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'Incompleto',
      'hardware.audioTest.warning': 'Avviso', 'hardware.audioTest.error': 'Errore'
    },
    fr: {
      'hardware.info.uartCommunication': 'Communication UART', 'hardware.info.playState': 'Lecture signalée par le module',
      'hardware.info.playStateMeasured': 'Dernière requête d’état', 'hardware.info.mediaStatus': 'Support', 'hardware.info.mediaRaw': 'Valeur brute du support',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY mesuré', 'hardware.info.busyChanged': 'Dernier changement BUSY',
      'hardware.info.busyInterpretation': 'Interprétation BUSY', 'hardware.info.audioTest': 'Test audio', 'hardware.info.audioTestStarted': 'Test audio démarré',
      'hardware.info.uartPlayConfirmed': 'PLAY UART confirmé', 'hardware.info.uartStopConfirmed': 'Fin UART confirmée', 'hardware.info.busyEdgeSeen': 'Front BUSY détecté',
      'hardware.media.response': 'Réponse reçue', 'hardware.busy.unconfirmed': 'Non confirmé', 'hardware.busy.active_low': 'LOW = lecture', 'hardware.busy.active_high': 'HIGH = lecture',
      'hardware.audioTest.not_run': 'Non exécuté', 'hardware.audioTest.running': 'En cours', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'Incomplet',
      'hardware.audioTest.warning': 'Avertissement', 'hardware.audioTest.error': 'Erreur'
    },
    swg: {
      'hardware.info.uartCommunication': 'UART-Verbindung', 'hardware.info.playState': 'Wiedergab laut Modul',
      'hardware.info.playStateMeasured': 'Letzte Statusabfrag', 'hardware.info.mediaStatus': 'Datenträger', 'hardware.info.mediaRaw': 'Datenträger-Rohwert',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY gmessa', 'hardware.info.busyChanged': 'Letzte BUSY-Änderung',
      'hardware.info.busyInterpretation': 'BUSY-Auswertung', 'hardware.info.audioTest': 'Audiotest', 'hardware.info.audioTestStarted': 'Audiotest gstartet',
      'hardware.info.uartPlayConfirmed': 'UART PLAY bestätigt', 'hardware.info.uartStopConfirmed': 'UART End bestätigt', 'hardware.info.busyEdgeSeen': 'BUSY-Flanke gseha',
      'hardware.media.response': 'Antwort komma', 'hardware.busy.unconfirmed': 'No net bestätigt', 'hardware.busy.active_low': 'LOW = Wiedergab', 'hardware.busy.active_high': 'HIGH = Wiedergab',
      'hardware.audioTest.not_run': 'No net gmacht', 'hardware.audioTest.running': 'Läuft', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'Net komplett',
      'hardware.audioTest.warning': 'Warnung', 'hardware.audioTest.error': 'Fehler'
    },
    'swg-alb': {
      'hardware.info.uartCommunication': 'UART-Verbindung', 'hardware.info.playState': 'Wiedergab laut Modul',
      'hardware.info.playStateMeasured': 'Letzte Statusabfrag', 'hardware.info.mediaStatus': 'Datenträger', 'hardware.info.mediaRaw': 'Datenträger-Rohwert',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY gmessa', 'hardware.info.busyChanged': 'Letzte BUSY-Änderung',
      'hardware.info.busyInterpretation': 'BUSY-Auswertung', 'hardware.info.audioTest': 'Audiotest', 'hardware.info.audioTestStarted': 'Audiotest gstartet',
      'hardware.info.uartPlayConfirmed': 'UART PLAY bestätigt', 'hardware.info.uartStopConfirmed': 'UART End bestätigt', 'hardware.info.busyEdgeSeen': 'BUSY-Flanke gseha',
      'hardware.media.response': 'Antwort komma', 'hardware.busy.unconfirmed': 'No net bestätigt', 'hardware.busy.active_low': 'LOW = Wiedergab', 'hardware.busy.active_high': 'HIGH = Wiedergab',
      'hardware.audioTest.not_run': 'No net gmacht', 'hardware.audioTest.running': 'Läuft', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'Net komplett',
      'hardware.audioTest.warning': 'Warnung', 'hardware.audioTest.error': 'Fehler'
    },
    'swg-ob': {
      'hardware.info.uartCommunication': 'UART-Verbindung', 'hardware.info.playState': 'Wiedergab laut Modul',
      'hardware.info.playStateMeasured': 'Letzte Statusabfrag', 'hardware.info.mediaStatus': 'Datenträger', 'hardware.info.mediaRaw': 'Datenträger-Rohwert',
      'hardware.info.busyLevel': 'BUSY GPIO39', 'hardware.info.busyMeasured': 'BUSY gmessa', 'hardware.info.busyChanged': 'Letzte BUSY-Änderung',
      'hardware.info.busyInterpretation': 'BUSY-Auswertung', 'hardware.info.audioTest': 'Audiotest', 'hardware.info.audioTestStarted': 'Audiotest gstartet',
      'hardware.info.uartPlayConfirmed': 'UART PLAY bestätigt', 'hardware.info.uartStopConfirmed': 'UART End bestätigt', 'hardware.info.busyEdgeSeen': 'BUSY-Flanke gseha',
      'hardware.media.response': 'Antwort komma', 'hardware.busy.unconfirmed': 'No it bestätigt', 'hardware.busy.active_low': 'LOW = Wiedergab', 'hardware.busy.active_high': 'HIGH = Wiedergab',
      'hardware.audioTest.not_run': 'No it gmacht', 'hardware.audioTest.running': 'Läuft', 'hardware.audioTest.ok': 'OK', 'hardware.audioTest.partial': 'It komplett',
      'hardware.audioTest.warning': 'Warnung', 'hardware.audioTest.error': 'Fehler'
    }
  };
  Object.entries(I18N_331).forEach(([code, labels]) => Object.assign(I18N[code], labels));
'''
if "const I18N_331" not in js:
    js = js.replace(insert_after, insert_after + overlay, 1)

fmt_anchor = "    stateKey(value) { return t(`hardware.play.${value || 'unknown'}`); },\n"
fmt_add = "    statusKey(value) { return t(`status.${value || 'unknown'}`); },\n    mediaStatus(value) { return t(`hardware.media.${value || 'response'}`); },\n    busyPolarity(value) { return t(`hardware.busy.${value || 'unconfirmed'}`); },\n    audioTestState(value) { return t(`hardware.audioTest.${value || 'not_run'}`); },\n"
if fmt_anchor not in js:
    raise SystemExit("Formats stateKey anchor missing")
if "busyPolarity(value)" not in js:
    js = js.replace(fmt_anchor, fmt_anchor + fmt_add, 1)
write(app_path, js)

# ---------------------------------------------------------------------------
# Release checks
# ---------------------------------------------------------------------------
rc_path = "Unterbrechungszaehler/tools/release_check.py"
rc = read(rc_path)
rc = rc.replace("Unterbrechungszaehler 3.3.0", "Unterbrechungszaehler 3.3.1", 1)
rc = rc.replace('SOFTWARE_VERSION[] = "3.3.0"', 'SOFTWARE_VERSION[] = "3.3.1"', 1)
rc = rc.replace('"project version 3.3.0"', '"project version 3.3.1"', 1)
rc = rc.replace('f"3.3.0 UI additions present for {language}"', 'f"3.3.1 UI additions present for {language}"', 1)
anchor = '    check("AUDIO_RX_PIN = 18" in hardware and "AUDIO_TX_PIN = 19" in hardware and "AUDIO_BUSY_PIN = 39" in hardware, "DY-SV17F pin map")\n'
checks = r'''    audio_cpp = (ROOT / "audio_dy_sv17f.cpp").read_text(encoding="utf-8")
    audio_h = (ROOT / "audio_dy_sv17f.h").read_text(encoding="utf-8")
    update_body = audio_cpp.split("void update() {", 1)[1].split("bool probe()", 1)[0]
    check("sampleBusyNow()" not in update_body, "no permanent BUSY polling in normal audio update")
    check("attachInterrupt" in audio_cpp and "busyIrqArmed" in audio_cpp and "onBusyEdge" in audio_cpp, "BUSY edge monitoring is event-driven and test-gated")
    check("sendQuery(WaitKind::TestPrePlay, 0x01)" in audio_cpp, "manual audio test starts with silent UART status query")
    check("sendPlayCommand(HardwareConfig::AUDIO_TEST_TRACK)" in audio_cpp and "sendFrame(0x07" in audio_cpp, "manual test keeps existing DY-SV17F play command")
    check("currentPlayStateMeasuredAtMs" in audio_cpp and "busyMeasuredAtMs" in audio_h, "UART and BUSY diagnostics carry measurement times")
    check("BusyPolarity::Unconfirmed" in audio_cpp and "active_low" in audio_cpp and "active_high" in audio_cpp, "BUSY polarity is unconfirmed until full test cycle")
    check("AudioTestState::Partial" in audio_cpp and "audioTestUartPlayingConfirmed" in audio_h, "manual audio-test result is explicit")
    check("command verification timeout" in audio_cpp and "StatusRegistry::State::NoResponse" in audio_cpp, "UART no-response remains a real diagnostic error")
    check("hardware.info.uartCommunication" in JS and "hardware.info.busyInterpretation" in JS and "hardware.info.audioTest" in JS, "richer DY-SV17F diagnostic UI")
'''
if anchor not in rc:
    raise SystemExit("release check audio anchor missing")
if "no permanent BUSY polling" not in rc:
    rc = rc.replace(anchor, anchor + checks, 1)
write(rc_path, rc)

# ---------------------------------------------------------------------------
# Docs / changelog
# ---------------------------------------------------------------------------
chg = read("CHANGELOG.md")
entry = '''## 3.3.1\n\n- DY-SV17F-Diagnose trennt UART-Status und BUSY-Hardwarepegel klar voneinander und zeigt deren Messzeitpunkte\n- BUSY wird nicht mehr permanent im Audio-Loop gepollt; ein Snapshot erfolgt nur bei Boot/Prüfung und Flanken werden ausschließlich während des manuellen Audiotests beobachtet\n- **Prüfen** bleibt lautlos und fragt Wiedergabestatus, Datenträger und Dateianzahl gezielt per UART ab\n- **Ton testen** wird zum nicht blockierenden End-to-End-Diagnosetest: UART PLAY bestätigen, BUSY-Flanken beobachten und ein vermutetes Ende einmal per UART gegenprüfen\n- BUSY-Polarität startet bewusst unbestätigt und wird nur nach einem vollständigen Ruhe → Wiedergabe → Ruhe-Zyklus als HIGH/LOW=Wiedergabe übernommen\n- Ein unplausibles BUSY-Signal beschädigt die funktionierende Wiedergabe nicht; fehlende UART-Antwort bleibt dagegen ein echter Diagnosefehler\n- Datenträgerwert wird nicht geraten: Klartext `Antwort erhalten` plus technischer Rohwert (z. B. `0x04`)\n\n'''
if "## 3.3.1" not in chg:
    chg = chg.replace("# Changelog\n\n", "# Changelog\n\n" + entry, 1)
write("CHANGELOG.md", chg)

readme = read("README.md")
readme = readme.replace("**Aktueller Stand:** `3.3.0`", "**Aktueller Stand:** `3.3.1`", 1)
if "### DY-SV17F-Diagnose in 3.3.1" not in readme:
    readme += '''\n### DY-SV17F-Diagnose in 3.3.1\n\nDie Soundwiedergabe blieb funktional unverändert. Die Diagnose unterscheidet jetzt sauber zwischen der letzten **UART-Protokollantwort** und dem rohen **BUSY-Pegel an GPIO39**. `Prüfen` bleibt lautlos. `Ton testen` kann zusätzlich einen kontrollierten End-to-End-Test durchführen und die BUSY-Polarität nur dann bestätigen, wenn UART und ein vollständiger BUSY-Zyklus zusammenpassen. BUSY ist reine Zusatzdiagnose und keine Voraussetzung für die Wiedergabe.\n'''
write("README.md", readme)

rn = read("Unterbrechungszaehler/RELEASE_NOTES.md")
if "# Release 3.3.1" not in rn[:200]:
    rn = '''# Release 3.3.1\n\n- DY-SV17F-Diagnose zeigt UART-Wiedergabestatus und BUSY-Rohpegel getrennt samt Messzeit\n- kein permanentes BUSY-Polling mehr; BUSY-Flanken nur während des manuellen Audiotests\n- `Prüfen` bleibt lautlos, `Ton testen` sammelt UART/BUSY-End-to-End-Diagnose\n- BUSY-Polarität wird nicht geraten und kann die funktionierende Wiedergabe nicht blockieren\n- Wiedergabepfad, Tracklogik, Lautstärke und UART-Pinbelegung bleiben unverändert\n\n''' + rn
write("Unterbrechungszaehler/RELEASE_NOTES.md", rn)

for rel in ("docs/de/README.md", "docs/en/README.md", "docs/swg/README.md"):
    text = read(rel)
    if rel.endswith("de/README.md") and "DY-SV17F-Diagnose 3.3.1" not in text:
        text += "\n## DY-SV17F-Diagnose 3.3.1\n\nUART-Wiedergabestatus und BUSY-Pegel werden getrennt und mit Messzeit dargestellt. `Prüfen` bleibt lautlos; `Ton testen` sammelt zusätzlich BUSY-Flanken und bestätigt ein vermutetes Ende per UART. BUSY bleibt reine Zusatzdiagnose und ist keine Voraussetzung für funktionierenden Sound.\n"
    elif rel.endswith("en/README.md") and "DY-SV17F diagnostics 3.3.1" not in text:
        text += "\n## DY-SV17F diagnostics 3.3.1\n\nUART playback status and the raw BUSY level are shown separately with measurement times. **Check** stays silent; **Test sound** additionally observes BUSY edges and confirms a suspected end through UART. BUSY remains optional diagnostics and is never required for normal playback.\n"
    elif rel.endswith("swg/README.md") and "DY-SV17F-Diagnose 3.3.1" not in text:
        text += "\n## DY-SV17F-Diagnose 3.3.1\n\nUART-Wiedergabstatus ond dr rohe BUSY-Pegel send jetzt sauber trennt ond hend ihre Messzeit dabei. `Prüfen` bleibt still; `Ton testen` schaut zusätzlich nach BUSY-Flanka ond fragt s vermutete End no amol per UART ab. BUSY isch bloß Diagnose ond net nötig, dass dr Ton funktioniert.\n"
    write(rel, text)

print("3.3.1 diagnostics patch applied")
