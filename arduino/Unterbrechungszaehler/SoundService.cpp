#include "SoundService.h"

#include <Preferences.h>

#include "Config.h"

namespace {
Preferences prefs;

uint8_t checksum(const uint8_t* data, uint8_t length) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < length; i++) sum += data[i];
  return static_cast<uint8_t>(sum & 0xFF);
}
}

void SoundService::begin() {
  loadSettings();

  // Der UART wird beim Boot kurz aktiv geprueft. Danach gibt es im Leerlauf
  // bewusst keine zyklischen Statusabfragen. Erst bei einer Wiedergabe wird
  // wieder gepollt, damit Start und Abschluss bestaetigt werden koennen.
  pinMode(UicConfig::SOUND_RX_PIN, INPUT_PULLUP);
  serial_.begin(UicConfig::SOUND_BAUD, SERIAL_8N1, UicConfig::SOUND_RX_PIN, UicConfig::SOUND_TX_PIN);
  while (serial_.available()) serial_.read();

  hardwareState_ = SoundHardwareState::Probing;
  confirmationCount_ = 0;
  bootProbeActive_ = true;
  bootProbeStartedAt_ = millis();
  lastQueryAt_ = 0;
  Serial.printf("[SOUND] UART RX=%u TX=%u gestartet | DY-SV17F Boot-Pruefung\n",
                UicConfig::SOUND_RX_PIN, UicConfig::SOUND_TX_PIN);
}

void SoundService::tick() {
  parseSerial();
  const uint32_t now = millis();

  if (queryPending_ && now - lastQueryAt_ > UicConfig::SOUND_RESPONSE_TIMEOUT_MS) {
    queryPending_ = false;
  }

  if (bootProbeActive_) {
    if (present()) {
      bootProbeActive_ = false;
      Serial.println("[SOUND] Boot-Pruefung abgeschlossen");
    } else if (now - bootProbeStartedAt_ >= UicConfig::SOUND_BOOT_PROBE_WINDOW_MS) {
      bootProbeActive_ = false;
      hardwareState_ = SoundHardwareState::Lost;
      confirmationCount_ = 0;
      queryPending_ = false;
      Serial.println("[SOUND] DY-SV17F beim Boot nicht erkannt");
    } else if (!queryPending_ && now - lastQueryAt_ >= UicConfig::SOUND_BOOT_PROBE_INTERVAL_MS) {
      queryState();
    }
  }

  if (playback_ == Playback::AwaitingStart &&
      now - playbackStartedAt_ > UicConfig::SOUND_START_TIMEOUT_MS) {
    failPlayback(true, "start_timeout");
  } else if (playback_ == Playback::Playing &&
             now - playbackStartedAt_ > UicConfig::SOUND_PLAYBACK_TIMEOUT_MS) {
    failPlayback(true, "playback_timeout");
  }

  // Nur eine aktive Wiedergabe braucht laufende Statusabfragen.
  if (busy() && !queryPending_ && now - lastQueryAt_ >= UicConfig::SOUND_QUERY_INTERVAL_MS) {
    queryState();
  }
}

bool SoundService::requestPlay() {
  return requestPlay(track_);
}

bool SoundService::requestPlay(uint16_t track) {
  if (!enabled_ || track == 0 || busy()) return false;

  // Auch wenn das Modul beim Boot nicht erkannt wurde, darf ein spaeteres
  // Ereignis einen neuen Versuch ausloesen. UART-Senden ist dabei harmlos;
  // die anschliessenden Statusabfragen entscheiden, ob die Wiedergabe lief.
  if (!present()) {
    hardwareState_ = SoundHardwareState::Probing;
    confirmationCount_ = 0;
  }

  while (serial_.available()) serial_.read();
  rxLen_ = 0;
  queryPending_ = false;

  setVolume();
  setSingleStopMode();
  playSpecified(track);

  sentCount_++;
  playback_ = Playback::AwaitingStart;
  playState_ = 0xFF;
  playbackStartedAt_ = millis();
  lastPlaybackError_ = "-";
  lastQueryAt_ = 0;
  return true;
}

bool SoundService::test() {
  if (track_ == 0 || busy()) return false;
  return requestPlay(track_);
}

bool SoundService::setSettings(bool enabled, uint8_t volume, uint16_t track) {
  if (volume > 30 || track == 0) return false;
  enabled_ = enabled;
  volume_ = volume;
  track_ = track;
  return saveSettings();
}

void SoundService::sendFrame(uint8_t command, const uint8_t* payload, uint8_t payloadLength) {
  uint8_t frame[20] = {0};
  const uint8_t length = static_cast<uint8_t>(4 + payloadLength);
  frame[0] = 0xAA;
  frame[1] = command;
  frame[2] = payloadLength;
  for (uint8_t i = 0; i < payloadLength; i++) frame[3 + i] = payload[i];
  frame[length - 1] = checksum(frame, length - 1);
  serial_.write(frame, length);
  serial_.flush();
}

void SoundService::queryState() {
  // Altlasten werden vor einer neuen Anfrage verworfen. Akzeptiert wird nur ein
  // vollstaendiges Statusframe innerhalb des Antwortfensters dieser Anfrage.
  while (serial_.available()) serial_.read();
  rxLen_ = 0;
  sendFrame(0x01);
  lastQueryAt_ = millis();
  queryPending_ = true;
}

void SoundService::setVolume() {
  const uint8_t data[1] = {volume_};
  sendFrame(0x13, data, 1);
}

void SoundService::setSingleStopMode() {
  const uint8_t data[1] = {0x02};
  sendFrame(0x18, data, 1);
}

void SoundService::playSpecified(uint16_t track) {
  // DY-SV17F UART-Protokoll: 0x07 = "Play specified music".
  const uint8_t data[2] = {
    static_cast<uint8_t>(track >> 8),
    static_cast<uint8_t>(track & 0xFF)
  };
  sendFrame(0x07, data, sizeof(data));
}

void SoundService::parseSerial() {
  while (serial_.available()) {
    const uint8_t b = static_cast<uint8_t>(serial_.read());
    if (rxLen_ == 0 && b != 0xAA) continue;
    if (rxLen_ >= sizeof(rx_)) rxLen_ = 0;
    rx_[rxLen_++] = b;

    if (rxLen_ >= 3) {
      const uint8_t expected = static_cast<uint8_t>(4 + rx_[2]);
      if (expected > sizeof(rx_)) {
        rxLen_ = 0;
        errorCount_++;
        continue;
      }
      if (rxLen_ == expected) {
        if (checksum(rx_, expected - 1) == rx_[expected - 1]) handleFrame(rx_, expected);
        else errorCount_++;
        rxLen_ = 0;
      }
    }
  }
}

void SoundService::handleFrame(const uint8_t* frame, uint8_t length) {
  if (!queryPending_ || millis() - lastQueryAt_ > UicConfig::SOUND_RESPONSE_TIMEOUT_MS) return;
  if (length != 5 || frame[0] != 0xAA || frame[1] != 0x01 || frame[2] != 0x01) return;

  const uint8_t state = frame[3];
  if (state > 0x02) {
    errorCount_++;
    return;
  }

  queryPending_ = false;
  playState_ = state;
  lastResponseAt_ = millis();
  responseCount_++;
  registerValidStatus(state);
}

void SoundService::registerValidStatus(uint8_t state) {
  if (!present()) {
    if (confirmationCount_ < 0xFF) confirmationCount_++;
    if (confirmationCount_ >= UicConfig::SOUND_REQUIRED_CONFIRMATIONS) {
      hardwareState_ = SoundHardwareState::Ready;
      Serial.printf("[SOUND] DY-SV17F nach %u Statusantworten bestaetigt\n",
                    static_cast<unsigned>(confirmationCount_));
    }

    // Falls die dritte Bestaetigung bereits PLAY meldet, kann die Wiedergabe
    // direkt als gestartet gelten; wir brauchen keine zusaetzliche Abfrage.
    if (present() && playback_ == Playback::AwaitingStart && state == 0x01) {
      playback_ = Playback::Playing;
      Serial.println("[SOUND] Wiedergabe durch PLAY-Status bestaetigt");
    }
    return;
  }

  confirmationCount_ = UicConfig::SOUND_REQUIRED_CONFIRMATIONS;

  if (playback_ == Playback::AwaitingStart) {
    if (state == 0x01) {
      playback_ = Playback::Playing;
      Serial.println("[SOUND] Wiedergabe durch PLAY-Status bestaetigt");
    }
    return;
  }

  if (playback_ == Playback::Playing && state == 0x00) {
    playback_ = Playback::Idle;
    completedCount_++;
    lastPlaybackError_ = "-";
    Serial.printf("[SOUND] Wiedergabe abgeschlossen | gesendet=%lu erfolgreich=%lu fehler=%lu\n",
                  static_cast<unsigned long>(sentCount_),
                  static_cast<unsigned long>(completedCount_),
                  static_cast<unsigned long>(failedCount_));
  }
}

void SoundService::failPlayback(bool timeout, const char* reason) {
  if (!busy()) return;
  playback_ = Playback::Idle;
  failedCount_++;
  if (timeout) timeoutCount_++;
  lastPlaybackError_ = reason ? reason : "playback_error";
  queryPending_ = false;
  if (!present()) {
    hardwareState_ = SoundHardwareState::Lost;
    confirmationCount_ = 0;
  }
  Serial.printf("[SOUND] Wiedergabe fehlgeschlagen: %s | fehler=%lu timeout=%lu\n",
                lastPlaybackError_,
                static_cast<unsigned long>(failedCount_),
                static_cast<unsigned long>(timeoutCount_));
}

ModuleState SoundService::moduleState() const {
  if (!enabled_) return ModuleState::Disabled;
  if (busy()) return ModuleState::Busy;
  if (!present()) return hardwareState_ == SoundHardwareState::Lost ? ModuleState::NotDetected : ModuleState::Initializing;
  return ModuleState::Ready;
}

const char* SoundService::statusDetail() const {
  if (!enabled_) return "sound_disabled";
  if (busy()) return playback_ == Playback::AwaitingStart ? "waiting_for_play" : "playing";
  if (!present()) return hardwareState_ == SoundHardwareState::Lost ? "hardware_missing" : "probing";
  if (lastPlaybackError_ && lastPlaybackError_[0] != '-') return lastPlaybackError_;
  return "ready_idle";
}

void SoundService::loadSettings() {
  if (!prefs.begin("uic-sound", true)) return;
  enabled_ = prefs.getBool("enabled", false);
  volume_ = prefs.getUChar("volume", 20);
  track_ = prefs.getUShort("track", 1);
  prefs.end();
  if (volume_ > 30) volume_ = 20;
  if (track_ == 0) track_ = 1;
}

bool SoundService::saveSettings() {
  if (!prefs.begin("uic-sound", false)) return false;
  const bool a = prefs.putBool("enabled", enabled_) == 1;
  const bool b = prefs.putUChar("volume", volume_) == 1;
  const bool c = prefs.putUShort("track", track_) == 2;
  prefs.end();
  return a && b && c;
}
