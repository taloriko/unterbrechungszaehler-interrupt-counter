#include "audio_dy_sv17f.h"

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
