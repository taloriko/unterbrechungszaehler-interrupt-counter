#include "audio_dy_sv17f.h"

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
