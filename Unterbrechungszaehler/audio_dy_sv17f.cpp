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
bool busyStateKnown = false;
bool busyState = false;
uint32_t checkedAtMs = 0;
const char *errorText = "";
PlayState currentPlayState = PlayState::Unknown;
uint8_t devicesOnline = 0;
uint16_t tracks = 0;

enum class WaitKind : uint8_t { None, ProbePlay, ProbeDevices, ProbeCount, VerifyPlay };
enum class VerifyExpectation : uint8_t { Any, Playing, Stopped };
WaitKind waitingFor = WaitKind::None;
VerifyExpectation verifyExpectation = VerifyExpectation::Any;
uint8_t expectedCommand = 0;
uint32_t responseDeadlineMs = 0;

enum class DeferredAction : uint8_t { None, StartProbe, QueryDevices, QueryCount, VerifyPlay, BootTone };
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
  const size_t total = static_cast<size_t>(length) + 4;
  if (total > sizeof(frame)) return;
  frame[0] = 0xAA;
  frame[1] = command;
  frame[2] = length;
  for (uint8_t i = 0; i < length; ++i) frame[3 + i] = data[i];
  frame[3 + length] = checksum(frame, 3 + length);
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

void finishProbe(StatusRegistry::State state, const char *message = "") {
  const bool shouldPlayBootTone = probeWasBoot && isDetected && HardwareConfig::AUDIO_BOOT_TONE_ENABLED;
  probeWasBoot = false;
  probeActive = false;
  waitingFor = WaitKind::None;
  checkedAtMs = millis();
  setHealth(state, message);
  if (state == StatusRegistry::State::Ok) {
    SerialLog::successf("AUDIO", "DY-SV17F: OK | play=%s | online=0x%02X | files=%u | BUSY=%s",
                        playStateName(), devicesOnline, tracks,
                        busyStateKnown ? (busyState ? "active" : "idle") : "n/a");
  } else if (state == StatusRegistry::State::Warning) {
    SerialLog::warningf("AUDIO", "DY-SV17F: WARNING | communication confirmed | some optional information missing | play=%s",
                        playStateName());
  } else {
    SerialLog::error("AUDIO", "DY-SV17F: NO RESPONSE | UART play-state query timed out");
  }

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

bool commandPathIdle() {
  return !probeActive && waitingFor == WaitKind::None && deferredAction == DeferredAction::None;
}

void updateBusyPin();

void handleFrame(uint8_t command, const uint8_t *data, uint8_t length) {
  if (command != expectedCommand || waitingFor == WaitKind::None) return;

  switch (waitingFor) {
    case WaitKind::ProbePlay:
      if (length != 1) return;
      isDetected = true;
      currentPlayState = data[0] == 0x01 ? PlayState::Playing : data[0] == 0x02 ? PlayState::Paused : PlayState::Stopped;
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

    case WaitKind::VerifyPlay:
      if (length == 1) {
        isDetected = true;
        currentPlayState = data[0] == 0x01 ? PlayState::Playing : data[0] == 0x02 ? PlayState::Paused : PlayState::Stopped;
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
      }
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
    rxExpected = static_cast<size_t>(rxBuffer[2]) + 4;
    if (rxExpected > sizeof(rxBuffer) || rxExpected < 4) {
      rxLength = 0;
      rxExpected = 0;
      return;
    }
  }

  if (rxExpected == 0 || rxLength < rxExpected) return;
  const uint8_t expectedChecksum = checksum(rxBuffer, rxExpected - 1);
  const uint8_t actualChecksum = rxBuffer[rxExpected - 1];
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
    updateBusyPin();
    const VerifyExpectation expectation = verifyExpectation;
    verifyExpectation = VerifyExpectation::Any;
    if (expectation == VerifyExpectation::Playing && busyStateKnown && busyState) {
      currentPlayState = PlayState::Playing;
      setHealth(StatusRegistry::State::Warning, "UART response missing; BUSY confirms playback");
      SerialLog::warning("AUDIO", "UART verification timed out, but BUSY is active | playback confirmed by BUSY");
    } else if (expectation == VerifyExpectation::Stopped && busyStateKnown && !busyState) {
      currentPlayState = PlayState::Stopped;
      setHealth(StatusRegistry::State::Warning, "UART response missing; BUSY confirms idle/stopped");
      SerialLog::warning("AUDIO", "UART verification timed out, but BUSY is idle | stop confirmed by BUSY");
    } else {
      setHealth(StatusRegistry::State::NoResponse, "command verification timeout");
      SerialLog::error("AUDIO", "Command verification failed | no matching UART/BUSY feedback");
    }
  }
}

void updateBusyPin() {
  if (HardwareConfig::AUDIO_BUSY_PIN < 0) return;
  busyStateKnown = true;
  busyState = digitalRead(HardwareConfig::AUDIO_BUSY_PIN) == LOW;
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
  setHealth(StatusRegistry::State::Checking);
  sendQuery(WaitKind::ProbePlay, 0x01);
  SerialLog::info("AUDIO", bootProbe ? "DY-SV17F boot health check started | querying play state"
                                    : "DY-SV17F health check started | querying play state");
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

  if (HardwareConfig::AUDIO_BUSY_PIN >= 0) pinMode(HardwareConfig::AUDIO_BUSY_PIN, INPUT);
  audioSerial.begin(HardwareConfig::AUDIO_BAUD_RATE, SERIAL_8N1,
                    HardwareConfig::AUDIO_RX_PIN, HardwareConfig::AUDIO_TX_PIN);
  SerialLog::infof("AUDIO", "DY-SV17F UART ready | UART%u | RX=%d | TX=%d | BUSY=%d | %lu baud",
                   HardwareConfig::AUDIO_UART_PORT, HardwareConfig::AUDIO_RX_PIN,
                   HardwareConfig::AUDIO_TX_PIN, HardwareConfig::AUDIO_BUSY_PIN,
                   static_cast<unsigned long>(HardwareConfig::AUDIO_BAUD_RATE));
  updateBusyPin();
  setHealth(StatusRegistry::State::Checking);
  deferredAction = DeferredAction::StartProbe;
  deferredAtMs = millis() + HardwareConfig::AUDIO_BOOT_GRACE_MS;
  SerialLog::infof("AUDIO", "DY-SV17F boot grace | first query in %lu ms",
                   static_cast<unsigned long>(HardwareConfig::AUDIO_BOOT_GRACE_MS));
  return true;
}

void update() {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F) return;
  updateBusyPin();

  while (audioSerial.available() > 0) feedParser(static_cast<uint8_t>(audioSerial.read()));
  handleTimeout();

  const uint32_t now = millis();
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
      if (!probeActive && waitingFor == WaitKind::None) sendQuery(WaitKind::VerifyPlay, 0x01);
    } else if (action == DeferredAction::BootTone) {
      if (!probeActive && waitingFor == WaitKind::None) {
        SerialLog::infof("AUDIO", "Boot tone | track=%u", HardwareConfig::AUDIO_BOOT_TONE_TRACK);
        playTrack(HardwareConfig::AUDIO_BOOT_TONE_TRACK);
      }
    }
  }
}

bool probe() {
  return startProbe(false);
}

bool enabled() { return HardwareConfig::ENABLE_AUDIO_DY_SV17F; }
bool detected() { return isDetected; }
bool checking() {
  return probeActive || waitingFor != WaitKind::None ||
         deferredAction == DeferredAction::StartProbe ||
         deferredAction == DeferredAction::QueryDevices ||
         deferredAction == DeferredAction::QueryCount ||
         deferredAction == DeferredAction::VerifyPlay;
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

bool busyKnown() { return busyStateKnown; }
bool busy() { return busyState; }
uint8_t onlineDevices() { return devicesOnline; }
uint16_t musicCount() { return tracks; }

bool playTrack(uint16_t trackNumber) {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || trackNumber == 0 || !commandPathIdle()) return false;

  // Do NOT send command 0x0B here. The DY-SV17F documentation states that
  // switching the drive also starts the first track, which makes command
  // verification ambiguous and can produce an unwanted sound. Track 0x07
  // directly addresses the currently selected/internal FLASH sequence.
  const uint8_t data[2] = {static_cast<uint8_t>((trackNumber >> 8) & 0xFF),
                           static_cast<uint8_t>(trackNumber & 0xFF)};
  sendFrame(0x07, data, sizeof(data));
  SerialLog::infof("AUDIO", "Play track command sent | track=%u", trackNumber);
  scheduleVerify(HardwareConfig::AUDIO_COMMAND_VERIFY_DELAY_MS, VerifyExpectation::Playing);
  return true;
}

bool playTestTone() {
  SerialLog::infof("AUDIO", "Manual audio test requested | track=%u", HardwareConfig::AUDIO_TEST_TRACK);
  return playTrack(HardwareConfig::AUDIO_TEST_TRACK);
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
  // The documented volume command has no return value, so no missing response
  // is interpreted as an error. Health remains based on real protocol queries.
  SerialLog::infof("AUDIO", "Volume command sent | volume=%u | no protocol response expected", volume);
  return true;
}

}  // namespace AudioDySv17f
