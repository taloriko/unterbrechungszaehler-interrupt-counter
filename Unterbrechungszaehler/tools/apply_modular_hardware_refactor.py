from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    (ROOT / rel).write_text(text, encoding="utf-8")


def replace_once(text, old, new, label):
    if old not in text:
        raise SystemExit(f"missing patch anchor: {label}")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# Central hardware configuration: keep timings/pins in one place.
# ---------------------------------------------------------------------------
hw = read("hardware_config.h")
old_audio = '''constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 1000;
constexpr uint8_t AUDIO_PROBE_MAX_ATTEMPTS = 2;
constexpr uint32_t AUDIO_PROBE_RETRY_DELAY_MS = 120;
constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;
// Playback itself is confirmed by the independent BUSY feedback line. This is
// intentionally separate from UART query timeouts: a lost status reply must not
// suppress otherwise valid audio. If BUSY never becomes active, resend the
// track once before reporting a playback warning.
constexpr uint32_t AUDIO_PLAY_BUSY_CONFIRM_MS = 450;
constexpr uint8_t AUDIO_PLAY_MAX_ATTEMPTS = 2;
constexpr uint32_t AUDIO_INTER_COMMAND_DELAY_MS = 120;
constexpr uint32_t AUDIO_BOOT_GRACE_MS = 1200;
constexpr bool AUDIO_BOOT_TONE_ENABLED = true;
constexpr uint16_t AUDIO_BOOT_TONE_TRACK = 1;
constexpr uint32_t AUDIO_BOOT_TONE_DELAY_MS = 350;
constexpr uint16_t AUDIO_TEST_TRACK = 1;
'''
new_audio = '''constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 900;
constexpr uint8_t AUDIO_PROBE_MAX_ATTEMPTS = 2;
// DY-SV17F commands are intentionally paced. Playback is higher priority than
// diagnostics, while volume is repeated idempotently because command 0x13 has
// no protocol response according to the UART guide.
constexpr uint32_t AUDIO_MIN_COMMAND_GAP_MS = 120;
constexpr uint32_t AUDIO_VOLUME_REPEAT_DELAY_MS = 150;
constexpr uint8_t AUDIO_VOLUME_SEND_REPEATS = 2;
constexpr uint32_t AUDIO_PLAY_BUSY_CONFIRM_MS = 450;
constexpr uint8_t AUDIO_PLAY_MAX_ATTEMPTS = 2;
constexpr uint32_t AUDIO_BOOT_GRACE_MS = 1200;
constexpr bool AUDIO_BOOT_TONE_ENABLED = true;
constexpr uint16_t AUDIO_BOOT_TONE_TRACK = 1;
constexpr uint32_t AUDIO_BOOT_TONE_DELAY_MS = 350;
constexpr uint32_t AUDIO_AUTO_PROBE_DELAY_MS = 5000;
constexpr uint16_t AUDIO_TEST_TRACK = 1;
'''
hw = replace_once(hw, old_audio, new_audio, "audio timing constants")
old_rf_freq = '''constexpr uint32_t RF433_FREQUENCY_HZ = 433920000UL;
constexpr uint32_t RF433_SOMFY_FREQUENCY_HZ = 433420000UL;  // Somfy RTS receive test
'''
new_rf_freq = '''constexpr uint32_t RF433_FREQUENCY_HZ = 433920000UL;
constexpr uint32_t RF433_SOMFY_FREQUENCY_HZ = 433420000UL;
// Raw OOK/RTS timing is captured by the ESP32 RMT peripheral, not by a GPIO
// CHANGE ISR. 1 MHz gives one-microsecond symbols; the hardware glitch filter
// rejects pulses far below every supported protocol timing.
constexpr uint32_t RF433_RMT_RESOLUTION_HZ = 1000000UL;
constexpr uint16_t RF433_RMT_MEM_BLOCK_SYMBOLS = 128;
constexpr uint16_t RF433_RMT_CAPTURE_SYMBOLS = 160;
constexpr uint32_t RF433_RMT_GLITCH_MIN_NS = 60000UL;
constexpr uint32_t RF433_RMT_IDLE_MAX_NS = 8000000UL;
'''
hw = replace_once(hw, old_rf_freq, new_rf_freq, "rf rmt constants")
write("hardware_config.h", hw)


# ---------------------------------------------------------------------------
# Audio: one cooperative state machine. Normal playback never depends on a
# query response. BUSY is the physical playback feedback. UART queries are
# low-priority diagnostics and can be pre-empted by a real interruption.
# ---------------------------------------------------------------------------
audio_h = r'''#pragma once

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
'''
write("audio_dy_sv17f.h", audio_h)

audio_cpp = r'''#include "audio_dy_sv17f.h"

#include <HardwareSerial.h>

#include "hardware_config.h"
#include "serial_log.h"
#include "status_registry.h"

namespace AudioDySv17f {
namespace {

HardwareSerial audioSerial(HardwareConfig::AUDIO_UART_PORT);
StatusRegistry::State moduleHealth = StatusRegistry::State::Unknown;
Diagnostics diag;
bool uartReady = false;
bool isDetected = false;
bool busyStateKnown = false;
bool busyState = false;
PlayState currentPlayState = PlayState::Unknown;
uint32_t checkedAtMs = 0;
const char *errorText = "";
uint32_t nextTxAllowedMs = 0;

uint8_t desiredVolumePercent = 100;
uint8_t volumeRepeatsPending = 0;
uint32_t volumeNotBeforeMs = 0;

bool bootTonePending = false;
uint32_t bootToneEarliestMs = 0;
bool autoProbePending = false;
uint32_t autoProbeEarliestMs = 0;

bool playbackConfirmActive = false;
bool playbackSawIdleSinceCommand = false;
uint16_t playbackTrack = 0;
uint8_t playbackAttempts = 0;
uint32_t playbackDeadlineMs = 0;
bool playbackBusyConfirmedEver = false;

enum class QueryKind : uint8_t {
  None,
  PlayState,
  OnlineDevice,
  CurrentDevice,
  TrackCount,
  CurrentTrack
};

constexpr QueryKind PROBE_SEQUENCE[] = {
    QueryKind::PlayState,
    QueryKind::OnlineDevice,
    QueryKind::CurrentDevice,
    QueryKind::TrackCount,
    QueryKind::CurrentTrack,
};
constexpr size_t PROBE_STEP_COUNT = sizeof(PROBE_SEQUENCE) / sizeof(PROBE_SEQUENCE[0]);

bool probeActive = false;
bool probeAutomatic = false;
size_t probeIndex = 0;
uint8_t probeSuccesses = 0;
uint8_t probeFailures = 0;
QueryKind waitingQuery = QueryKind::None;
uint8_t queryAttempts = 0;
uint32_t queryDeadlineMs = 0;

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
  return static_cast<uint8_t>(sum & 0xFFU);
}

bool txReady(uint32_t now) {
  return uartReady && due(now, nextTxAllowedMs);
}

void markTx(uint8_t command, uint32_t nextGapMs = HardwareConfig::AUDIO_MIN_COMMAND_GAP_MS) {
  const uint32_t now = millis();
  diag.lastCommand = command;
  diag.lastTxMs = now;
  ++diag.txFrames;
  nextTxAllowedMs = now + nextGapMs;
}

bool sendFrameNow(uint8_t command, const uint8_t *data = nullptr, uint8_t length = 0,
                  uint32_t nextGapMs = HardwareConfig::AUDIO_MIN_COMMAND_GAP_MS) {
  if (!uartReady) return false;
  uint8_t frame[12]{};
  const size_t total = static_cast<size_t>(length) + 4U;
  if (total > sizeof(frame)) return false;
  frame[0] = 0xAA;
  frame[1] = command;
  frame[2] = length;
  for (uint8_t i = 0; i < length; ++i) frame[3U + i] = data[i];
  frame[3U + length] = checksum(frame, 3U + length);
  const size_t written = audioSerial.write(frame, total);
  if (written != total) {
    SerialLog::warningf("AUDIO", "UART short write | cmd=0x%02X | %u/%u bytes",
                        command, static_cast<unsigned int>(written), static_cast<unsigned int>(total));
    return false;
  }
  markTx(command, nextGapMs);
  return true;
}

uint8_t moduleVolumeForPercent(uint8_t percent) {
  if (percent > 100U) percent = 100U;
  return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 30U + 50U) / 100U);
}

uint8_t commandForQuery(QueryKind kind) {
  switch (kind) {
    case QueryKind::PlayState: return 0x01;
    case QueryKind::OnlineDevice: return 0x09;
    case QueryKind::CurrentDevice: return 0x0A;
    case QueryKind::TrackCount: return 0x0C;
    case QueryKind::CurrentTrack: return 0x0D;
    case QueryKind::None:
    default: return 0;
  }
}

uint8_t expectedLength(QueryKind kind) {
  switch (kind) {
    case QueryKind::PlayState:
    case QueryKind::OnlineDevice:
    case QueryKind::CurrentDevice: return 1;
    case QueryKind::TrackCount:
    case QueryKind::CurrentTrack: return 2;
    case QueryKind::None:
    default: return 0;
  }
}

void resetParser() {
  rxLength = 0;
  rxExpected = 0;
  while (audioSerial.available() > 0) audioSerial.read();
}

void finishProbe() {
  probeActive = false;
  waitingQuery = QueryKind::None;
  queryAttempts = 0;
  checkedAtMs = millis();
  if (probeFailures == 0U && probeSuccesses == PROBE_STEP_COUNT) {
    isDetected = true;
    setHealth(StatusRegistry::State::Ok);
    SerialLog::successf("AUDIO", "Diagnostic probe OK | device=%s | files=%u | current=%u | RX=%lu",
                        deviceName(diag.currentDevice), static_cast<unsigned int>(diag.trackCount),
                        static_cast<unsigned int>(diag.currentTrack),
                        static_cast<unsigned long>(diag.rxFrames));
  } else if (probeSuccesses > 0U || playbackBusyConfirmedEver) {
    isDetected = true;
    setHealth(StatusRegistry::State::Warning, "UART diagnostics partial; playback feedback remains independent");
    SerialLog::warningf("AUDIO", "Diagnostic probe partial | success=%u | failed=%u | BUSY-confirmed=%s",
                        static_cast<unsigned int>(probeSuccesses), static_cast<unsigned int>(probeFailures),
                        playbackBusyConfirmedEver ? "yes" : "no");
  } else {
    isDetected = false;
    setHealth(StatusRegistry::State::NoResponse, "no UART diagnostic response and no BUSY-confirmed playback");
    SerialLog::error("AUDIO", "Diagnostic probe failed | no UART response and no BUSY-confirmed playback");
  }
  probeAutomatic = false;
}

void advanceProbe(bool success) {
  if (!probeActive) return;
  if (success) ++probeSuccesses;
  else ++probeFailures;
  ++probeIndex;
  queryAttempts = 0;
  waitingQuery = QueryKind::None;
  if (probeIndex >= PROBE_STEP_COUNT) finishProbe();
}

bool sendCurrentProbeQuery() {
  if (!probeActive || probeIndex >= PROBE_STEP_COUNT || waitingQuery != QueryKind::None) return false;
  const uint32_t now = millis();
  if (!txReady(now)) return false;
  const QueryKind kind = PROBE_SEQUENCE[probeIndex];
  const uint8_t command = commandForQuery(kind);
  if (!sendFrameNow(command)) return false;
  waitingQuery = kind;
  ++queryAttempts;
  queryDeadlineMs = millis() + HardwareConfig::AUDIO_RESPONSE_TIMEOUT_MS;
  return true;
}

void cancelProbeForPlayback() {
  if (!probeActive && waitingQuery == QueryKind::None) return;
  const bool retryAutomatic = probeAutomatic;
  probeActive = false;
  probeAutomatic = false;
  waitingQuery = QueryKind::None;
  queryAttempts = 0;
  resetParser();
  if (retryAutomatic) {
    autoProbePending = true;
    autoProbeEarliestMs = millis() + HardwareConfig::AUDIO_AUTO_PROBE_DELAY_MS;
  }
  SerialLog::info("AUDIO", "Low-priority diagnostic probe pre-empted by playback");
}

bool startProbe(bool automatic) {
  if (!enabled() || !uartReady || probeActive || playbackConfirmActive) return false;
  if (busyStateKnown && busyState && !automatic) {
    SerialLog::warning("AUDIO", "Manual diagnostic rejected while track is playing");
    return false;
  }
  resetParser();
  probeActive = true;
  probeAutomatic = automatic;
  probeIndex = 0;
  probeSuccesses = 0;
  probeFailures = 0;
  waitingQuery = QueryKind::None;
  queryAttempts = 0;
  setHealth(StatusRegistry::State::Checking);
  SerialLog::info("AUDIO", automatic ? "Background diagnostic started" : "Manual diagnostic started");
  return true;
}

void handleValidResponse(uint8_t command, const uint8_t *data, uint8_t length) {
  ++diag.rxFrames;
  diag.lastRxMs = millis();
  diag.uartResponseSeen = true;
  isDetected = true;

  if (waitingQuery == QueryKind::None || command != commandForQuery(waitingQuery)) {
    ++diag.unexpectedFrames;
    return;
  }
  if (length != expectedLength(waitingQuery)) {
    ++diag.unexpectedFrames;
    return;
  }

  switch (waitingQuery) {
    case QueryKind::PlayState:
      currentPlayState = data[0] == 0x01 ? PlayState::Playing :
                         data[0] == 0x02 ? PlayState::Paused : PlayState::Stopped;
      break;
    case QueryKind::OnlineDevice:
      diag.onlineDevice = data[0];
      diag.onlineDeviceKnown = true;
      break;
    case QueryKind::CurrentDevice:
      diag.currentDevice = data[0];
      diag.currentDeviceKnown = true;
      break;
    case QueryKind::TrackCount:
      diag.trackCount = static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8U) | data[1]);
      diag.trackCountKnown = true;
      break;
    case QueryKind::CurrentTrack:
      diag.currentTrack = static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8U) | data[1]);
      diag.currentTrackKnown = true;
      break;
    case QueryKind::None:
    default:
      break;
  }
  advanceProbe(true);
}

void feedParser(uint8_t value) {
  if (rxLength == 0U) {
    if (value != 0xAAU) return;
    rxBuffer[rxLength++] = value;
    rxExpected = 0;
    return;
  }
  if (rxLength >= sizeof(rxBuffer)) {
    rxLength = 0;
    rxExpected = 0;
    ++diag.unexpectedFrames;
    return;
  }
  rxBuffer[rxLength++] = value;
  if (rxLength == 3U) {
    rxExpected = static_cast<size_t>(rxBuffer[2]) + 4U;
    if (rxExpected < 4U || rxExpected > sizeof(rxBuffer)) {
      rxLength = 0;
      rxExpected = 0;
      ++diag.unexpectedFrames;
      return;
    }
  }
  if (rxExpected == 0U || rxLength < rxExpected) return;

  const uint8_t wanted = checksum(rxBuffer, rxExpected - 1U);
  const uint8_t got = rxBuffer[rxExpected - 1U];
  if (wanted == got) handleValidResponse(rxBuffer[1], &rxBuffer[3], rxBuffer[2]);
  else {
    ++diag.checksumErrors;
    SerialLog::warning("AUDIO", "Ignored DY-SV17F frame with invalid checksum");
  }
  rxLength = 0;
  rxExpected = 0;
}

void handleQueryTimeout() {
  if (waitingQuery == QueryKind::None || !due(millis(), queryDeadlineMs)) return;
  ++diag.queryTimeouts;
  const QueryKind timedOut = waitingQuery;
  waitingQuery = QueryKind::None;
  if (queryAttempts < HardwareConfig::AUDIO_PROBE_MAX_ATTEMPTS) {
    SerialLog::warningf("AUDIO", "Diagnostic query timeout | cmd=0x%02X | retry=%u/%u",
                        commandForQuery(timedOut), static_cast<unsigned int>(queryAttempts + 1U),
                        static_cast<unsigned int>(HardwareConfig::AUDIO_PROBE_MAX_ATTEMPTS));
    return;  // same probeIndex is retried after the global command gap
  }
  SerialLog::warningf("AUDIO", "Diagnostic query unavailable | cmd=0x%02X", commandForQuery(timedOut));
  advanceProbe(false);
}

void confirmPlayback() {
  playbackConfirmActive = false;
  playbackBusyConfirmedEver = true;
  isDetected = true;
  ++diag.busyConfirmedPlays;
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("AUDIO", "Playback confirmed by BUSY edge | track=%u | attempt=%u",
                      static_cast<unsigned int>(playbackTrack), static_cast<unsigned int>(playbackAttempts));
}

void updateBusyPin() {
  if (HardwareConfig::AUDIO_BUSY_PIN < 0) return;
  const bool next = digitalRead(HardwareConfig::AUDIO_BUSY_PIN) == LOW;
  if (!busyStateKnown) {
    busyStateKnown = true;
    busyState = next;
    diag.lastBusyChangeMs = millis();
    if (!busyState && currentPlayState == PlayState::Unknown) currentPlayState = PlayState::Stopped;
    return;
  }
  if (next == busyState) return;
  busyState = next;
  ++diag.busyEdges;
  diag.lastBusyChangeMs = millis();

  if (busyState) {
    currentPlayState = PlayState::Playing;
    isDetected = true;
    if (playbackConfirmActive && playbackSawIdleSinceCommand) confirmPlayback();
  } else {
    if (currentPlayState == PlayState::Playing) currentPlayState = PlayState::Stopped;
    if (playbackConfirmActive) playbackSawIdleSinceCommand = true;
  }
}

bool sendPlayNow(uint16_t track, bool retry) {
  const uint32_t now = millis();
  if (!txReady(now)) return false;
  const uint8_t data[2] = {static_cast<uint8_t>((track >> 8U) & 0xFFU), static_cast<uint8_t>(track & 0xFFU)};
  if (!sendFrameNow(0x07, data, sizeof(data))) return false;
  ++diag.playCommands;
  if (retry) ++diag.playRetries;
  diag.lastRequestedTrack = track;
  playbackTrack = track;
  playbackSawIdleSinceCommand = !busyStateKnown || !busyState;
  playbackConfirmActive = HardwareConfig::AUDIO_BUSY_PIN >= 0;
  playbackDeadlineMs = millis() + HardwareConfig::AUDIO_PLAY_BUSY_CONFIRM_MS;
  if (!playbackConfirmActive) {
    // No BUSY input means the play command is transport-only; do not invent a
    // confirmation. This project does have BUSY wired, so this is fallback only.
    setHealth(StatusRegistry::State::Warning, "play command sent without external BUSY feedback");
  } else {
    setHealth(StatusRegistry::State::Checking);
  }
  return true;
}

void handlePlaybackConfirmation() {
  if (!playbackConfirmActive || !due(millis(), playbackDeadlineMs)) return;
  if (playbackAttempts < HardwareConfig::AUDIO_PLAY_MAX_ATTEMPTS) {
    if (!txReady(millis())) return;
    ++playbackAttempts;
    if (sendPlayNow(playbackTrack, true)) {
      SerialLog::warningf("AUDIO", "No fresh BUSY edge after play command | retry %u/%u | track=%u",
                          static_cast<unsigned int>(playbackAttempts),
                          static_cast<unsigned int>(HardwareConfig::AUDIO_PLAY_MAX_ATTEMPTS),
                          static_cast<unsigned int>(playbackTrack));
    }
    return;
  }
  playbackConfirmActive = false;
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Warning, "play command not confirmed by a fresh BUSY transition");
  SerialLog::warningf("AUDIO", "Playback not confirmed by a fresh BUSY transition | track=%u",
                      static_cast<unsigned int>(playbackTrack));
}

void serviceVolume() {
  if (volumeRepeatsPending == 0U || probeActive || waitingQuery != QueryKind::None || playbackConfirmActive) return;
  const uint32_t now = millis();
  if (!due(now, volumeNotBeforeMs) || !txReady(now)) return;
  if (busyStateKnown && busyState) return;  // do not change control settings mid-track
  const uint8_t step = moduleVolumeForPercent(desiredVolumePercent);
  if (!sendFrameNow(0x13, &step, 1, HardwareConfig::AUDIO_VOLUME_REPEAT_DELAY_MS)) return;
  diag.desiredVolumePercent = desiredVolumePercent;
  diag.lastVolumeStep = step;
  ++diag.volumeCommands;
  --volumeRepeatsPending;
  SerialLog::infof("AUDIO", "Volume command sent | ui=%u%% | module=%u/30 | remaining-repeat=%u",
                   static_cast<unsigned int>(desiredVolumePercent), static_cast<unsigned int>(step),
                   static_cast<unsigned int>(volumeRepeatsPending));
}

void serviceBootAndBackgroundProbe() {
  const uint32_t now = millis();
  if (bootTonePending && due(now, bootToneEarliestMs) && volumeRepeatsPending == 0U &&
      !probeActive && !playbackConfirmActive && txReady(now)) {
    if (playTrack(HardwareConfig::AUDIO_BOOT_TONE_TRACK)) {
      bootTonePending = false;
      SerialLog::infof("AUDIO", "Boot tone command sent | track=%u", HardwareConfig::AUDIO_BOOT_TONE_TRACK);
    }
  }

  if (autoProbePending && !bootTonePending && !playbackConfirmActive && volumeRepeatsPending == 0U &&
      (!busyStateKnown || !busyState) && due(now, autoProbeEarliestMs)) {
    if (startProbe(true)) autoProbePending = false;
  }
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("audio", "status.audio", "audio", HardwareConfig::ENABLE_AUDIO_DY_SV17F);
  if (!enabled()) {
    setHealth(StatusRegistry::State::Disabled);
    StatusRegistry::setVisible("audio", false);
    return false;
  }

  // configureVolumePercent() is called before HardwareRegistry::begin(). Keep
  // that persisted project preference across transport initialization.
  diag = Diagnostics{};
  diag.desiredVolumePercent = desiredVolumePercent;
  diag.lastVolumeStep = moduleVolumeForPercent(desiredVolumePercent);
  if (HardwareConfig::AUDIO_BUSY_PIN >= 0) pinMode(HardwareConfig::AUDIO_BUSY_PIN, INPUT);
  audioSerial.begin(HardwareConfig::AUDIO_BAUD_RATE, SERIAL_8N1,
                    HardwareConfig::AUDIO_RX_PIN, HardwareConfig::AUDIO_TX_PIN);
  uartReady = true;
  nextTxAllowedMs = millis() + HardwareConfig::AUDIO_BOOT_GRACE_MS;
  volumeNotBeforeMs = nextTxAllowedMs;
  volumeRepeatsPending = HardwareConfig::AUDIO_VOLUME_SEND_REPEATS;
  bootTonePending = HardwareConfig::AUDIO_BOOT_TONE_ENABLED;
  bootToneEarliestMs = millis() + HardwareConfig::AUDIO_BOOT_GRACE_MS + HardwareConfig::AUDIO_BOOT_TONE_DELAY_MS;
  autoProbePending = true;
  autoProbeEarliestMs = millis() + HardwareConfig::AUDIO_AUTO_PROBE_DELAY_MS;
  updateBusyPin();
  setHealth(StatusRegistry::State::Checking);
  SerialLog::infof("AUDIO", "DY-SV17F transport ready | UART%u RX=%d TX=%d BUSY=%d | %lu 8N1",
                   HardwareConfig::AUDIO_UART_PORT, HardwareConfig::AUDIO_RX_PIN,
                   HardwareConfig::AUDIO_TX_PIN, HardwareConfig::AUDIO_BUSY_PIN,
                   static_cast<unsigned long>(HardwareConfig::AUDIO_BAUD_RATE));
  return true;
}

void update() {
  if (!enabled() || !uartReady) return;
  updateBusyPin();
  while (audioSerial.available() > 0) feedParser(static_cast<uint8_t>(audioSerial.read()));
  handleQueryTimeout();
  handlePlaybackConfirmation();

  if (probeActive) sendCurrentProbeQuery();
  else serviceVolume();
  serviceBootAndBackgroundProbe();
}

bool probe() { return startProbe(false); }

bool enabled() { return HardwareConfig::ENABLE_AUDIO_DY_SV17F; }
bool detected() { return isDetected; }
bool checking() { return probeActive || waitingQuery != QueryKind::None || playbackConfirmActive; }
StatusRegistry::State health() { return moduleHealth; }
uint32_t lastCheckMs() { return checkedAtMs; }
const char *lastError() { return errorText; }
HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::ExternalFeedback; }
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
uint8_t onlineDevices() { return diag.onlineDevice; }
uint8_t currentDevice() { return diag.currentDevice; }
uint16_t musicCount() { return diag.trackCount; }
uint16_t currentTrack() { return diag.currentTrack; }
const Diagnostics &diagnostics() { return diag; }

const char *deviceName(uint8_t device) {
  switch (device) {
    case 0x00: return "USB";
    case 0x01: return "SD";
    case 0x02: return "FLASH";
    case 0xFF: return "NO_DEVICE";
    default: return "UNKNOWN";
  }
}

void configureVolumePercent(uint8_t percent) {
  desiredVolumePercent = percent > 100U ? 100U : percent;
  diag.desiredVolumePercent = desiredVolumePercent;
  volumeRepeatsPending = HardwareConfig::AUDIO_VOLUME_SEND_REPEATS;
  if (uartReady) volumeNotBeforeMs = millis();
}

bool setVolumePercent(uint8_t percent) {
  if (percent > 100U) return false;
  configureVolumePercent(percent);
  return true;
}

uint8_t volumePercent() { return desiredVolumePercent; }

bool playTrack(uint16_t trackNumber) {
  if (!enabled() || !uartReady || trackNumber == 0U || playbackConfirmActive) return false;
  if (probeActive || waitingQuery != QueryKind::None) cancelProbeForPlayback();
  if (volumeRepeatsPending > 0U) return false;
  if (!txReady(millis())) return false;
  playbackAttempts = 1;
  if (!sendPlayNow(trackNumber, false)) return false;
  SerialLog::infof("AUDIO", "Play track requested | track=%u | BUSY-before=%s",
                   static_cast<unsigned int>(trackNumber),
                   busyStateKnown ? (busyState ? "active" : "idle") : "unknown");
  return true;
}

bool playTestTone() {
  SerialLog::infof("AUDIO", "Manual audio test requested | track=%u", HardwareConfig::AUDIO_TEST_TRACK);
  return playTrack(HardwareConfig::AUDIO_TEST_TRACK);
}

bool stop() {
  if (!enabled() || !uartReady || playbackConfirmActive) return false;
  if (probeActive || waitingQuery != QueryKind::None) cancelProbeForPlayback();
  if (!txReady(millis())) return false;
  return sendFrameNow(0x04);
}

bool pause() {
  if (!enabled() || !uartReady || playbackConfirmActive) return false;
  if (probeActive || waitingQuery != QueryKind::None) cancelProbeForPlayback();
  if (!txReady(millis())) return false;
  return sendFrameNow(0x03);
}

bool setVolume(uint8_t volume) {
  if (volume > 30U) return false;
  const uint8_t percent = static_cast<uint8_t>((static_cast<uint16_t>(volume) * 100U + 15U) / 30U);
  configureVolumePercent(percent);
  return true;
}

}  // namespace AudioDySv17f
'''
write("audio_dy_sv17f.cpp", audio_cpp)


# ---------------------------------------------------------------------------
# RF header diagnostics for the RMT backend.
# ---------------------------------------------------------------------------
rfh = read("rf433_cc1101.h")
old_info = '''  uint32_t activeFrequencyHz = 0;
  uint32_t decodedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint32_t overflowFrames = 0;
  Frame lastFrame{};
'''
new_info = '''  uint32_t activeFrequencyHz = 0;
  bool captureReady = false;
  bool carrierSense = false;
  uint16_t lastCaptureSymbols = 0;
  uint32_t captureFrames = 0;
  uint32_t captureErrors = 0;
  uint32_t decodedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint32_t overflowFrames = 0;
  Frame lastFrame{};
'''
rfh = replace_once(rfh, old_info, new_info, "rf info fields")
write("rf433_cc1101.h", rfh)


# ---------------------------------------------------------------------------
# RF implementation: move edge timing into ESP32 RMT RX. The callback only
# publishes a completed static buffer. All protocol decoding runs in loop().
# ---------------------------------------------------------------------------
rf = read("rf433_cc1101.cpp")
rf = replace_once(rf, '#include <SPI.h>\n#include <driver/gpio.h>\n#include <cstring>\n',
                  '#include <SPI.h>\n#include <driver/rmt_rx.h>\n#include <driver/rmt_types.h>\n#include <cstring>\n', "rf includes")
rf = replace_once(rf, '''constexpr uint16_t PULSE_BUFFER_SIZE = 160;
constexpr uint16_t MIN_FRAME_PULSES = 36;
constexpr uint32_t FRAME_GAP_US = 5000;
constexpr uint32_t FORCE_FRAME_GAP_US = 7000;
''', '''constexpr uint16_t PULSE_WORK_CAPACITY = 192;
constexpr uint16_t MIN_FRAME_PULSES = 36;
''', "rf old edge constants")

start = rf.index('volatile uint16_t pulseBuffers[2][PULSE_BUFFER_SIZE]{};')
end = rf.index('void setHealth(StatusRegistry::State state) {')
new_globals = '''rmt_channel_handle_t rmtChannel = nullptr;
rmt_symbol_word_t rmtSymbols[HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS]{};
volatile bool rmtFrameReady = false;
volatile size_t rmtReadySymbols = 0;
bool rmtEnabled = false;
bool rmtArmed = false;

Frame pendingCandidate;
uint8_t pendingRepeats = 0;
uint32_t pendingSeenMs = 0;
Frame emittedFrame;
bool emittedAvailable = false;
uint32_t lastEmitMs = 0;
StatusRegistry::State currentHealth = StatusRegistry::State::Disabled;
uint32_t checkedAtMs = 0;
bool receiveTestActiveFlag = false;
uint32_t receiveTestStartedMs = 0;
const char *receiveTestResultText = "idle";
Frame receiveTestFrame;

enum class CaptureMode : uint8_t { FixedOok = 0, SomfyRts = 1 };
CaptureMode captureMode = CaptureMode::FixedOok;
Protocol operatingProtocolValue = Protocol::FixedOok;

'''
rf = rf[:start] + new_globals + rf[end:]

# Replace the complete old GPIO edge ISR/capture block with RMT lifecycle.
block_start = rf.index('void IRAM_ATTR resetSomfyCapture')
block_end = rf.index('bool similarBucket(uint8_t a, uint8_t b) {')
rmt_block = r'''bool IRAM_ATTR onRmtReceiveDone(rmt_channel_handle_t, const rmt_rx_done_event_data_t *edata, void *) {
  if (!edata) return false;
  rmtReadySymbols = edata->num_symbols > HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS
                        ? HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS
                        : edata->num_symbols;
  rmtFrameReady = true;
  rmtArmed = false;
  return false;  // no task is woken; normal loop consumes the static buffer
}

bool armRmtReceive() {
  if (!rmtChannel || !rmtEnabled || rmtArmed || rmtFrameReady) return false;
  rmt_receive_config_t cfg{};
  cfg.signal_range_min_ns = HardwareConfig::RF433_RMT_GLITCH_MIN_NS;
  cfg.signal_range_max_ns = HardwareConfig::RF433_RMT_IDLE_MAX_NS;
  const esp_err_t err = rmt_receive(rmtChannel, rmtSymbols, sizeof(rmtSymbols), &cfg);
  if (err != ESP_OK) {
    ++currentInfo.captureErrors;
    currentInfo.captureReady = false;
    return false;
  }
  rmtArmed = true;
  currentInfo.captureReady = true;
  return true;
}

bool initRmtCapture() {
  rmt_rx_channel_config_t cfg{};
  cfg.gpio_num = static_cast<gpio_num_t>(HardwareConfig::RF433_GDO0_PIN);
  cfg.clk_src = RMT_CLK_SRC_DEFAULT;
  cfg.resolution_hz = HardwareConfig::RF433_RMT_RESOLUTION_HZ;
  cfg.mem_block_symbols = HardwareConfig::RF433_RMT_MEM_BLOCK_SYMBOLS;
  cfg.intr_priority = 0;
  cfg.flags.invert_in = false;
  cfg.flags.with_dma = false;
  if (rmt_new_rx_channel(&cfg, &rmtChannel) != ESP_OK || !rmtChannel) return false;

  rmt_rx_event_callbacks_t callbacks{};
  callbacks.on_recv_done = onRmtReceiveDone;
  if (rmt_rx_register_event_callbacks(rmtChannel, &callbacks, nullptr) != ESP_OK) return false;
  if (rmt_enable(rmtChannel) != ESP_OK) return false;
  rmtEnabled = true;
  return armRmtReceive();
}

void pauseRmtCapture() {
  if (rmtChannel && rmtEnabled) rmt_disable(rmtChannel);
  rmtEnabled = false;
  rmtArmed = false;
  rmtFrameReady = false;
  rmtReadySymbols = 0;
  currentInfo.captureReady = false;
}

bool resumeRmtCapture() {
  if (!rmtChannel) return false;
  if (!rmtEnabled) {
    if (rmt_enable(rmtChannel) != ESP_OK) {
      ++currentInfo.captureErrors;
      return false;
    }
    rmtEnabled = true;
  }
  return armRmtReceive();
}

bool applyCaptureMode(CaptureMode mode) {
  pauseRmtCapture();
  if (!strobe(SIDLE)) return false;

  // 26 MHz crystal: 433.92 MHz = 0x10B071, Somfy RTS 433.42 MHz = 0x10AB85.
  const bool somfy = mode == CaptureMode::SomfyRts;
  if (!writeRegister(0x0D, 0x10) ||
      !writeRegister(0x0E, somfy ? 0xAB : 0xB0) ||
      !writeRegister(0x0F, somfy ? 0x85 : 0x71)) {
    return false;
  }
  strobe(SFRX);
  captureMode = mode;
  if (!strobe(SRX)) return false;
  currentInfo.activeFrequencyHz = somfy ? HardwareConfig::RF433_SOMFY_FREQUENCY_HZ
                                        : HardwareConfig::RF433_FREQUENCY_HZ;
  return resumeRmtCapture();
}

'''
rf = rf[:block_start] + rmt_block + rf[block_end:]

# Replace Somfy ISR-ready processor with loop-side RMT symbol decoder.
somfy_start = rf.index('void processSomfyReady() {')
somfy_end = rf.index('void processCandidate(const Frame &candidate) {')
new_somfy = r'''struct SomfyDecodeState {
  bool receiving = false;
  bool waitingHalf = false;
  uint8_t syncCount = 0;
  uint8_t bitCount = 0;
  uint8_t previousBit = 0;
  uint8_t payload[7]{};
};

bool appendSomfyBit(SomfyDecodeState &state) {
  if (state.bitCount >= 56U) return true;
  if (state.previousBit) {
    state.payload[state.bitCount / 8U] |= static_cast<uint8_t>(1U << (7U - (state.bitCount % 8U)));
  }
  ++state.bitCount;
  return state.bitCount >= 56U;
}

bool feedSomfyDuration(SomfyDecodeState &state, uint32_t duration) {
  if (duration < SOMFY_GLITCH_MIN_US) return false;
  if (!state.receiving) {
    if (duration >= SOMFY_HW_SYNC_MIN_US && duration <= SOMFY_HW_SYNC_MAX_US) {
      if (state.syncCount < 31U) ++state.syncCount;
      return false;
    }
    if (duration >= SOMFY_SW_SYNC_MIN_US && duration <= SOMFY_SW_SYNC_MAX_US && state.syncCount >= 4U) {
      state.receiving = true;
      state.waitingHalf = false;
      state.bitCount = 0;
      state.previousBit = 0;
      for (uint8_t &byte : state.payload) byte = 0;
      return false;
    }
    state.syncCount = 0;
    return false;
  }

  if (duration >= SOMFY_SYMBOL_MIN_US && duration <= SOMFY_SYMBOL_MAX_US && !state.waitingHalf) {
    state.previousBit ^= 1U;
    return appendSomfyBit(state);
  }
  if (duration >= SOMFY_HALF_MIN_US && duration <= SOMFY_HALF_MAX_US) {
    if (state.waitingHalf) {
      state.waitingHalf = false;
      return appendSomfyBit(state);
    }
    state.waitingHalf = true;
    return false;
  }

  state.receiving = false;
  state.waitingHalf = false;
  state.syncCount = 0;
  state.bitCount = 0;
  state.previousBit = 0;
  for (uint8_t &byte : state.payload) byte = 0;
  return false;
}

bool decodeSomfySymbols(const rmt_symbol_word_t *symbols, size_t count, Frame &out, uint8_t &syncCount) {
  SomfyDecodeState state;
  for (size_t i = 0; i < count; ++i) {
    const uint32_t durations[2] = {symbols[i].duration0, symbols[i].duration1};
    for (uint8_t part = 0; part < 2; ++part) {
      const uint32_t duration = durations[part];
      if (duration == 0U) continue;
      if (feedSomfyDuration(state, duration)) {
        syncCount = state.syncCount;
        return decodeSomfyPayload(state.payload, out);
      }
    }
  }
  return false;
}

void processSomfyCandidate(const Frame &candidate, uint8_t syncCount) {
  const uint32_t nowMs = millis();
  if (sameFrame(candidate, currentInfo.lastFrame) && static_cast<uint32_t>(nowMs - lastEmitMs) < PRESS_DEDUPE_MS) {
    return;
  }

  const bool diagnostic = receiveTestActiveFlag;
  if (diagnostic) {
    receiveTestActiveFlag = false;
    receiveTestResultText = "somfy_received";
    receiveTestFrame = candidate;
    checkedAtMs = nowMs;
    currentInfo.error = "";
    setHealth(StatusRegistry::State::Ok);
    SerialLog::successf("RF433", "Somfy RTS test passed | sync=%u | address=0x%06lX | rolling=%u | command=%s",
                        static_cast<unsigned int>(syncCount), static_cast<unsigned long>(candidate.code),
                        static_cast<unsigned int>(candidate.rollingCode), somfyCommandName(candidate.command));
  }

  emittedFrame = candidate;
  emittedFrame.diagnostic = diagnostic;
  emittedAvailable = true;
  currentInfo.lastFrame = emittedFrame;
  ++currentInfo.decodedFrames;
  lastEmitMs = nowMs;
}

'''
rf = rf[:somfy_start] + new_somfy + rf[somfy_end:]

# Replace old pulse-buffer processor with RMT static-buffer processor.
ready_start = rf.index('void processReadyFrame() {')
ready_end = rf.index('\n}\n\n}  // namespace', ready_start) + 2
new_ready = r'''void processRmtCapture() {
  if (!rmtFrameReady) {
    currentInfo.carrierSense = digitalRead(HardwareConfig::RF433_GDO2_PIN) != 0;
    if (!rmtArmed && rmtEnabled) armRmtReceive();
    return;
  }

  rmt_symbol_word_t local[HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS]{};
  const size_t count = rmtReadySymbols > HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS
                           ? HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS
                           : rmtReadySymbols;
  for (size_t i = 0; i < count; ++i) local[i] = rmtSymbols[i];
  rmtFrameReady = false;
  rmtReadySymbols = 0;
  ++currentInfo.captureFrames;
  currentInfo.lastCaptureSymbols = static_cast<uint16_t>(count);
  if (count >= HardwareConfig::RF433_RMT_CAPTURE_SYMBOLS) ++currentInfo.overflowFrames;
  currentInfo.carrierSense = digitalRead(HardwareConfig::RF433_GDO2_PIN) != 0;

  // Rearm before decoding the private copy. RF reception therefore remains
  // mostly hardware-autonomous while the loop performs bounded parsing.
  if (!armRmtReceive()) ++currentInfo.captureErrors;
  if (count == 0U) return;

  if (captureMode == CaptureMode::SomfyRts) {
    Frame candidate;
    uint8_t syncCount = 0;
    if (!decodeSomfySymbols(local, count, candidate, syncCount)) {
      ++currentInfo.rejectedFrames;
      return;
    }
    processSomfyCandidate(candidate, syncCount);
    return;
  }

  uint16_t timings[PULSE_WORK_CAPACITY]{};
  uint16_t timingCount = 0;
  for (size_t i = 0; i < count && timingCount < PULSE_WORK_CAPACITY; ++i) {
    const uint32_t durations[2] = {local[i].duration0, local[i].duration1};
    for (uint8_t part = 0; part < 2 && timingCount < PULSE_WORK_CAPACITY; ++part) {
      const uint32_t duration = durations[part];
      if (duration >= 70U && duration <= 4600U) timings[timingCount++] = static_cast<uint16_t>(duration);
    }
  }
  if (timingCount < MIN_FRAME_PULSES) {
    ++currentInfo.rejectedFrames;
    return;
  }
  Frame candidate;
  if (!decodeFrame(timings, timingCount, candidate)) {
    ++currentInfo.rejectedFrames;
    return;
  }
  processCandidate(candidate);
}'''
rf = rf[:ready_start] + new_ready + rf[ready_end:]

# Boot: RMT channel owns GDO0 timing. No GPIO CHANGE interrupt.
old_boot_tail = '''  captureMode = CaptureMode::FixedOok;
  operatingProtocolValue = Protocol::FixedOok;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
  lastEdgeUs = micros();
  attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN), onDataEdge, CHANGE);
  currentInfo.ready = true;
'''
new_boot_tail = '''  captureMode = CaptureMode::FixedOok;
  operatingProtocolValue = Protocol::FixedOok;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
  if (!initRmtCapture()) {
    failReceiver("cc1101_rmt_init_failed", StatusRegistry::State::Error);
    SerialLog::error("RF433", "ESP32 RMT RX channel could not be initialized");
    return false;
  }
  currentInfo.ready = true;
'''
rf = replace_once(rf, old_boot_tail, new_boot_tail, "rf boot capture")
rf = rf.replace('    detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));\n', '')
rf = replace_once(rf,
                  '  SerialLog::successf("RF433", "CC1101 ready | 433.92 MHz OOK async | part=0x%02X | version=0x%02X | GDO0=%d GDO2=%d",',
                  '  SerialLog::successf("RF433", "CC1101 ready | 433.92 MHz OOK async -> ESP32 RMT RX | part=0x%02X | version=0x%02X | GDO0=%d GDO2=%d",',
                  "rf ready log")
rf = replace_once(rf,
                  '  if (captureMode == CaptureMode::SomfyRts) processSomfyReady();\n  else processReadyFrame();\n',
                  '  processRmtCapture();\n', "rf update processor")
rf = replace_once(rf,
                  'bool pollFrame(Frame &frameOut) {\n  update();\n  if (!emittedAvailable) return false;',
                  'bool pollFrame(Frame &frameOut) {\n  if (!emittedAvailable) return false;', "rf poll pure")
rf = replace_once(rf,
                  'const Info &info() {\n  currentInfo.overflowFrames = isrOverflowFrames;\n  return currentInfo;\n}',
                  'const Info &info() {\n  currentInfo.captureReady = rmtEnabled && (rmtArmed || rmtFrameReady);\n  currentInfo.carrierSense = currentInfo.ready && digitalRead(HardwareConfig::RF433_GDO2_PIN) != 0;\n  return currentInfo;\n}', "rf info")
write("rf433_cc1101.cpp", rf)


# ---------------------------------------------------------------------------
# Lifecycle ownership: HardwareRegistry owns hardware drivers. Rf433Service is
# project-level mapping only and never initializes/updates the driver itself.
# ---------------------------------------------------------------------------
svc = read("rf433_service.cpp")
svc = replace_once(svc,
'''  SourceRegistry::begin();
  if (!Rf433Cc1101::begin()) return false;
  if (!Rf433Cc1101::setOperatingProtocol(driverProtocol(ProjectPreferences::radioMode()))) {
''',
'''  SourceRegistry::begin();
  if (!Rf433Cc1101::info().ready) return false;
  if (!Rf433Cc1101::setOperatingProtocol(driverProtocol(ProjectPreferences::radioMode()))) {
''', "rf service begin ownership")
svc = replace_once(svc,
'''void update() {
  SourceRegistry::update();
  Rf433Cc1101::update();

''',
'''void update() {
  SourceRegistry::update();

''', "rf service update ownership")
write("rf433_service.cpp", svc)

reg = read("hardware_registry.cpp")
reg = replace_once(reg,
'''void update() {
  if (!pinMapValid) return;
  GpioModule::update();
  AudioDySv17f::update();
}
''',
'''void update() {
  if (!pinMapValid) return;
  // Urgent local input and UART servicing run before bounded RF decoding.
  // Each concrete driver is updated exactly here; project services only
  // consume the cached/decoded results afterwards.
  GpioModule::update();
  AudioDySv17f::update();
  Rf433Cc1101::update();
}
''', "hardware registry update ownership")

old_audio_info = '''    if (AudioDySv17f::detected()) {
      appendInfoString(out, first, "hardware.info.playState", String(AudioDySv17f::playStateName()), "stateKey");
      appendInfoString(out, first, "hardware.info.onlineDevices", hexByte(AudioDySv17f::onlineDevices()));
      appendInfoUInt(out, first, "hardware.info.fileCount", AudioDySv17f::musicCount());
    }
    if (AudioDySv17f::busyKnown()) appendInfoBool(out, first, "hardware.info.busy", AudioDySv17f::busy());
    appendInfoUInt(out, first, "hardware.info.testTrack", HardwareConfig::AUDIO_TEST_TRACK);
'''
new_audio_info = '''    const auto &audio = AudioDySv17f::diagnostics();
    appendInfoString(out, first, "hardware.info.playState", String(AudioDySv17f::playStateName()), "stateKey");
    if (audio.onlineDeviceKnown) appendInfoString(out, first, "hardware.info.onlineDevices", AudioDySv17f::deviceName(audio.onlineDevice));
    if (audio.currentDeviceKnown) appendInfoString(out, first, "hardware.info.currentDevice", AudioDySv17f::deviceName(audio.currentDevice));
    if (audio.trackCountKnown) appendInfoUInt(out, first, "hardware.info.fileCount", audio.trackCount);
    if (audio.currentTrackKnown) appendInfoUInt(out, first, "hardware.info.currentTrack", audio.currentTrack);
    if (AudioDySv17f::busyKnown()) appendInfoBool(out, first, "hardware.info.busy", AudioDySv17f::busy());
    appendInfoUInt(out, first, "hardware.info.volume", audio.desiredVolumePercent, "percent");
    appendInfoUInt(out, first, "hardware.info.volumeStep", audio.lastVolumeStep);
    appendInfoUInt(out, first, "hardware.info.uartResponses", audio.rxFrames);
    appendInfoUInt(out, first, "hardware.info.uartTimeouts", audio.queryTimeouts);
    appendInfoUInt(out, first, "hardware.info.checksumErrors", audio.checksumErrors);
    appendInfoUInt(out, first, "hardware.info.playCommands", audio.playCommands);
    appendInfoUInt(out, first, "hardware.info.busyConfirmedPlays", audio.busyConfirmedPlays);
    appendInfoUInt(out, first, "hardware.info.playRetries", audio.playRetries);
    appendInfoUInt(out, first, "hardware.info.busyEdges", audio.busyEdges);
    appendInfoUInt(out, first, "hardware.info.testTrack", HardwareConfig::AUDIO_TEST_TRACK);
'''
reg = replace_once(reg, old_audio_info, new_audio_info, "audio diagnostics json")
old_rf_info = '''    appendInfoUInt(out, first, "hardware.info.decodedFrames", rf.decodedFrames);
    appendInfoUInt(out, first, "hardware.info.rejectedFrames", rf.rejectedFrames);
    appendInfoUInt(out, first, "hardware.info.overflowFrames", rf.overflowFrames);
'''
new_rf_info = '''    appendInfoString(out, first, "hardware.info.captureBackend", "ESP32 RMT RX");
    appendInfoBool(out, first, "hardware.info.captureReady", rf.captureReady);
    appendInfoBool(out, first, "hardware.info.carrierSense", rf.carrierSense);
    appendInfoUInt(out, first, "hardware.info.captureFrames", rf.captureFrames);
    appendInfoUInt(out, first, "hardware.info.lastCaptureSymbols", rf.lastCaptureSymbols);
    appendInfoUInt(out, first, "hardware.info.captureErrors", rf.captureErrors);
    appendInfoUInt(out, first, "hardware.info.decodedFrames", rf.decodedFrames);
    appendInfoUInt(out, first, "hardware.info.rejectedFrames", rf.rejectedFrames);
    appendInfoUInt(out, first, "hardware.info.overflowFrames", rf.overflowFrames);
'''
reg = replace_once(reg, old_rf_info, new_rf_info, "rf rmt diagnostics json")
write("hardware_registry.cpp", reg)


# ---------------------------------------------------------------------------
# UI labels: dynamic post-pack assignment keeps all seven language variants
# complete without duplicating the already large base dictionaries.
# ---------------------------------------------------------------------------
app = read("ui-src/app.js")
anchor = '  const LANGUAGE_LABELS = {\n'
if anchor not in app:
    raise SystemExit("missing LANGUAGE_LABELS anchor")
labels = r'''  const HARDWARE_DIAG_LABELS = {
    de: {
      'hardware.info.currentDevice': 'Aktiver Datenträger', 'hardware.info.currentTrack': 'Aktueller Track',
      'hardware.info.volume': 'Soll-Lautstärke', 'hardware.info.volumeStep': 'Gesendete Lautstärkestufe (0–30)',
      'hardware.info.uartResponses': 'UART-Antworten', 'hardware.info.uartTimeouts': 'UART-Timeouts',
      'hardware.info.checksumErrors': 'UART-Prüfsummenfehler', 'hardware.info.playCommands': 'Play-Kommandos',
      'hardware.info.busyConfirmedPlays': 'Per BUSY bestätigte Starts', 'hardware.info.playRetries': 'Play-Wiederholungen',
      'hardware.info.busyEdges': 'BUSY-Flanken', 'hardware.info.captureBackend': 'Pulserfassung',
      'hardware.info.captureReady': 'RMT-Empfang bereit', 'hardware.info.carrierSense': 'Trägersignal aktiv',
      'hardware.info.captureFrames': 'RMT-Aufnahmen', 'hardware.info.lastCaptureSymbols': 'Symbole letzte Aufnahme',
      'hardware.info.captureErrors': 'RMT-Fehler'
    },
    en: {
      'hardware.info.currentDevice': 'Active storage device', 'hardware.info.currentTrack': 'Current track',
      'hardware.info.volume': 'Requested volume', 'hardware.info.volumeStep': 'Sent volume step (0–30)',
      'hardware.info.uartResponses': 'UART responses', 'hardware.info.uartTimeouts': 'UART timeouts',
      'hardware.info.checksumErrors': 'UART checksum errors', 'hardware.info.playCommands': 'Play commands',
      'hardware.info.busyConfirmedPlays': 'Starts confirmed by BUSY', 'hardware.info.playRetries': 'Play retries',
      'hardware.info.busyEdges': 'BUSY edges', 'hardware.info.captureBackend': 'Pulse capture',
      'hardware.info.captureReady': 'RMT receiver ready', 'hardware.info.carrierSense': 'Carrier sense active',
      'hardware.info.captureFrames': 'RMT captures', 'hardware.info.lastCaptureSymbols': 'Symbols in last capture',
      'hardware.info.captureErrors': 'RMT errors'
    },
    it: {
      'hardware.info.currentDevice': 'Supporto attivo', 'hardware.info.currentTrack': 'Traccia attuale',
      'hardware.info.volume': 'Volume richiesto', 'hardware.info.volumeStep': 'Livello volume inviato (0–30)',
      'hardware.info.uartResponses': 'Risposte UART', 'hardware.info.uartTimeouts': 'Timeout UART',
      'hardware.info.checksumErrors': 'Errori checksum UART', 'hardware.info.playCommands': 'Comandi play',
      'hardware.info.busyConfirmedPlays': 'Avvii confermati da BUSY', 'hardware.info.playRetries': 'Ripetizioni play',
      'hardware.info.busyEdges': 'Fronti BUSY', 'hardware.info.captureBackend': 'Acquisizione impulsi',
      'hardware.info.captureReady': 'Ricezione RMT pronta', 'hardware.info.carrierSense': 'Portante attiva',
      'hardware.info.captureFrames': 'Acquisizioni RMT', 'hardware.info.lastCaptureSymbols': 'Simboli ultima acquisizione',
      'hardware.info.captureErrors': 'Errori RMT'
    },
    fr: {
      'hardware.info.currentDevice': 'Support actif', 'hardware.info.currentTrack': 'Piste actuelle',
      'hardware.info.volume': 'Volume demandé', 'hardware.info.volumeStep': 'Niveau de volume envoyé (0–30)',
      'hardware.info.uartResponses': 'Réponses UART', 'hardware.info.uartTimeouts': 'Timeouts UART',
      'hardware.info.checksumErrors': 'Erreurs checksum UART', 'hardware.info.playCommands': 'Commandes lecture',
      'hardware.info.busyConfirmedPlays': 'Démarrages confirmés par BUSY', 'hardware.info.playRetries': 'Répétitions lecture',
      'hardware.info.busyEdges': 'Fronts BUSY', 'hardware.info.captureBackend': 'Capture des impulsions',
      'hardware.info.captureReady': 'Réception RMT prête', 'hardware.info.carrierSense': 'Porteuse active',
      'hardware.info.captureFrames': 'Captures RMT', 'hardware.info.lastCaptureSymbols': 'Symboles dernière capture',
      'hardware.info.captureErrors': 'Erreurs RMT'
    },
    swg: {
      'hardware.info.currentDevice': 'Aktiver Datenträger', 'hardware.info.currentTrack': 'Aktueller Track',
      'hardware.info.volume': 'Soll-Lautstärk', 'hardware.info.volumeStep': 'Gsendete Lautstärkstuf (0–30)',
      'hardware.info.uartResponses': 'UART-Antworta', 'hardware.info.uartTimeouts': 'UART-Timeouts',
      'hardware.info.checksumErrors': 'UART-Prüfsummafehler', 'hardware.info.playCommands': 'Play-Kommandos',
      'hardware.info.busyConfirmedPlays': 'Per BUSY bestätigte Starts', 'hardware.info.playRetries': 'Play-Wiederholunga',
      'hardware.info.busyEdges': 'BUSY-Flanka', 'hardware.info.captureBackend': 'Pulserfassig',
      'hardware.info.captureReady': 'RMT-Empfang bereit', 'hardware.info.carrierSense': 'Trägersignal aktiv',
      'hardware.info.captureFrames': 'RMT-Aufnahma', 'hardware.info.lastCaptureSymbols': 'Symbol letzte Aufnahm',
      'hardware.info.captureErrors': 'RMT-Fehler'
    }
  };
  HARDWARE_DIAG_LABELS['swg-alb'] = HARDWARE_DIAG_LABELS.swg;
  HARDWARE_DIAG_LABELS['swg-ob'] = HARDWARE_DIAG_LABELS.swg;
  Object.entries(HARDWARE_DIAG_LABELS).forEach(([code, labels]) => Object.assign(I18N[code], labels));

'''
app = app.replace(anchor, labels + anchor, 1)
write("ui-src/app.js", app)


# ---------------------------------------------------------------------------
# Architecture/test documentation for the draft. Keep baseline 3.2 historical.
# ---------------------------------------------------------------------------
doc = r'''# Hardware-Laufzeitarchitektur – Draft 3.3.0-dev433

Dieser Draft überarbeitet **nicht** die Fachlogik des Unterbrechungszählers. Er zieht nur die optionale Hardware wieder auf die ursprünglichen Basisregeln zurück: kleine unabhängige Treiber, zentrale Lifecycle-Orchestrierung, keine Projektbedeutung im Hardwarecode, keine unnötigen Interruptlasten und keine erfundenen Bestätigungen.

## Verbindliche Verantwortungsgrenzen

```text
HardwareRegistry
  ├─ GpioModule          GPIO / lokaler Zustand
  ├─ RtcDs3231           I2C / RTC
  ├─ DisplaySh1106       I2C / OLED
  ├─ AudioDySv17f        UART2 + BUSY
  └─ Rf433Cc1101         SPI + RMT RX + Funkdecoder

Rf433Service              SourceRegistry + Projektzuordnung
InterruptionService       Unterbrechungsereignis / Feedback / Speicherung
```

`HardwareRegistry` ist die **einzige** Stelle, die konkrete Hardwaretreiber startet und zyklisch bedient. `Rf433Service` initialisiert oder tickt den CC1101-Treiber nicht mehr; es konsumiert nur fertige Frames und ordnet sie stabilen Source-IDs zu.

## GPIO

DI1 bleibt ein Human-Button mit minimalem Edge-Latch. Die ISR setzt nur ein Flag. Entprellung, Callback und Projektarbeit laufen im normalen Loop. Deaktivierte generische Pins werden nicht konfiguriert und stehen optionalen Modulen zur Verfügung.

## DS3231

Der RTC-Treiber bleibt bewusst unverändert: gemeinsamer 400-kHz-I2C-Bus, BCD-/Kalenderprüfung, OSF-Auswertung, Temperatur und Schreibverifikation. Teilreads werden nicht als gültige Zeit ausgegeben.

## SH1106

Der Displaytreiber bleibt bewusst unverändert: fester 1024-Byte-Framebuffer, kleine I2C-Chunks, Controllerkommandos für Ein/Aus, Kontrast und 180°-Rotation. Ein Displayfehler beeinflusst keine Ereigniserfassung.

## DY-SV17F

UART bleibt 9600 8N1 auf GPIO18/19. CON3/BUSY auf GPIO39 ist nach der Mode-Select-Phase das unabhängige physische Playback-Feedback.

Die Laufzeit wurde vereinfacht:

- `Play specified music (0x07)` erwartet laut Protokoll **keine** UART-Antwort.
- Ein normaler Ton wird deshalb nicht mehr mit einer zusätzlichen Statusabfrage gekoppelt.
- Ein Wiedergabestart gilt nur dann als BUSY-bestätigt, wenn nach dem Play-Kommando eine **frische** BUSY-Sequenz beobachtet wird. Ein bereits vorher LOW stehendes BUSY bestätigt kein neues Kommando.
- Ein nicht bestätigter Play-Befehl wird höchstens einmal wiederholt.
- Lautstärke `0x13` ist command-only. 0–100 % wird zentral auf die dokumentierten 31 Stufen 0–30 abgebildet. Weil das Modul darauf keine Antwort sendet, wird dieselbe Einstellung zweimal mit Abstand gesendet; die UI nennt sie ausdrücklich Soll-/Sendewert, nicht bestätigten Istwert.
- Diagnoseabfragen `0x01`, `0x09`, `0x0A`, `0x0C`, `0x0D` laufen sequenziell und niedrig priorisiert. Echte Wiedergabe darf eine Diagnose abbrechen.
- Fehlgeschlagene Diagnoseabfragen löschen keine zuvor gültig gelesenen Werte.
- Nach dem Boot läuft einmal eine niedrige Prioritätsdiagnose, damit Trackanzahl/Datenträger für Rotation und Hardwarekarte verfügbar werden.

Die Hardwarekarte zeigt BUSY, Wiedergabestatus, Online-/aktiven Datenträger, Dateizahl, aktuellen Track, Soll-Lautstärke, gesendete Modulstufe sowie UART-/Playback-Zähler.

## CC1101 / RF1100SE

Der CC1101 bleibt im asynchronen OOK-Modus, weil Universal-Festcodes und Somfy RTS unterschiedliche proprietäre Rohpulse benötigen. **Die Rohflanken werden aber nicht mehr per GPIO-CHANGE-ISR in Software vermessen.**

Stattdessen besitzt der ESP32 einen RMT-RX-Kanal auf GDO0:

- 1 MHz Auflösung = 1 µs
- statischer 160-Symbol-Puffer
- 60-µs-Hardware-Glitchfilter
- 8-ms-Idlegrenze beendet eine Aufnahme
- Callback veröffentlicht nur `frame ready + symbol count`
- Universal-/Somfy-Decoding läuft vollständig im normalen Loop
- nach Kopie in einen kleinen lokalen Arbeitsbuffer wird RMT sofort wieder scharfgeschaltet

Damit sinkt die Interruptlast von „eine ISR pro RF-Flanke“ auf ungefähr „ein RX-done-Callback pro Aufnahme“. Das schützt insbesondere UART2, WLAN und den restlichen kooperativen Loop vor zufälliger 433-MHz-Rauschlast.

Die Hardwarekarte zeigt zusätzlich RMT-Bereitschaft, Carrier Sense, Anzahl Aufnahmen, letzte Symbolzahl, RMT-Fehler, Decodererfolge/-verwerfungen und Überläufe. Der bestehende 10-s-Empfangstest bleibt erhalten.

## Bus-/Prioritätsmodell

```text
Loop-Priorität:
1. HardwareRegistry: DI
2. HardwareRegistry: Audio UART/BUSY
3. HardwareRegistry: RF RMT-Auswertung
4. Rf433Service: fertige Frames -> Source-ID
5. InterruptionService: Feedback + Persistenzpipeline
6. WLAN/Web/Zeit/OTA
```

I2C, SPI, UART2 und RMT besitzen getrennte Verantwortungsbereiche. Kein Modul darf ein anderes Modul direkt initialisieren oder dessen Fehlerstatus überschreiben.

## Diagnosesemantik

Ein Diagnosewert ist nur dann „bestätigt“, wenn seine reale Rückmeldung dazu passt:

- I2C/SPI: Bus-/Registerantwort
- Audio-Wiedergabe: externe BUSY-Flanke
- Audio-Kommandos ohne Response (z. B. Volume): nur „gesendet“, nie „bestätigt“
- RF-Konfiguration: SPI-Readback
- RF-Empfang: RMT-Aufnahme + gültiger Protokolldecoder

Ein optionales Modul darf ausfallen, ohne Raw-Event, PendingQueue oder andere Hardwaremodule zu beschädigen.
'''
write("HARDWARE_RUNTIME_DRAFT.md", doc)

rf_test = read("RF433_TEST.md")
if "## Kombitest Funk + Audio" not in rf_test:
    rf_test += r'''

## Kombitest Funk + Audio

Dieser Test ist nach der RMT-Umstellung besonders wichtig:

1. CC1101 angeschlossen lassen und passenden Modus wählen.
2. Unter Hardware prüfen: `Pulserfassung = ESP32 RMT RX`, `RMT-Empfang bereit = Ja`.
3. DY-SV17F auf 0 % stellen, mindestens eine Sekunde warten und `Ton testen` drücken. BUSY darf aktiv werden, hörbar sollte bei wirksamem Volume-Kommando nichts sein.
4. Danach 25 %, 50 % und 100 % testen. Die Karte zeigt Soll-Lautstärke und die gesendete Modulstufe 0…30.
5. 20 normale Töne mit Funkempfänger angeschlossen auslösen. `Per BUSY bestätigte Starts` muss entsprechend steigen; `Play-Wiederholungen` sollte normalerweise 0 bleiben.
6. Parallel mehrfach den Funkbutton drücken. Audio und Funk müssen unabhängig bleiben.
7. `Prüfen` beim Audio startet ausschließlich die UART-Diagnose. Fehlende optionale Antworten dürfen ein reales BUSY-bestätigtes Playback nicht als verschwunden darstellen.

Bei einem Fehler Screenshot der Audio- **und** Funkkarte zusammen sichern. Relevant sind besonders UART-Timeouts, Prüfsummenfehler, Play-Wiederholungen, BUSY-Flanken, RMT-Aufnahmen und RMT-Fehler.
'''
write("RF433_TEST.md", rf_test)


# ---------------------------------------------------------------------------
# Release checks: replace the old audio-specific regression assertion with
# invariants for the modular RMT/BUSY architecture.
# ---------------------------------------------------------------------------
check = read("tools/release_check.py")
old_audio_check = '    check("AUDIO_PROBE_MAX_ATTEMPTS = 2" in hardware and "RetryProbePlay" in audio_driver and "BUSY confirms active playback" in audio_driver, "DY-SV17F nonblocking query retry and BUSY fallback")\n'
new_arch_checks = (
    '    check("AUDIO_MIN_COMMAND_GAP_MS = 120" in hardware and "AUDIO_VOLUME_SEND_REPEATS = 2" in hardware and "HardwareTypes::FeedbackType::ExternalFeedback" in audio_driver, "DY-SV17F paced command path with external BUSY feedback")\n'
    '    check("case QueryKind::CurrentDevice: return 0x0A;" in audio_driver and "case QueryKind::CurrentTrack: return 0x0D;" in audio_driver, "DY-SV17F full low-priority diagnostics")\n'
    '    check('#include <driver/rmt_rx.h>' in rf_driver and 'attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN)' not in rf_driver, "CC1101 raw timing uses ESP32 RMT instead of per-edge ISR")\n'
    '    check("Rf433Cc1101::update();" in hardware_registry and "Rf433Cc1101::update();" not in (ROOT / "rf433_service.cpp").read_text(encoding="utf-8"), "HardwareRegistry exclusively services RF driver")\n'
    '    check("if (!Rf433Cc1101::begin())" not in (ROOT / "rf433_service.cpp").read_text(encoding="utf-8"), "project RF service does not initialize hardware driver")\n'
    '    check("HARDWARE_DIAG_LABELS" in JS, "expanded hardware diagnostics translated")\n'
)
if old_audio_check not in check:
    raise SystemExit("old audio release-check assertion not found")
check = check.replace(old_audio_check, new_arch_checks, 1)
write("tools/release_check.py", check)

print("Modular hardware refactor applied")
