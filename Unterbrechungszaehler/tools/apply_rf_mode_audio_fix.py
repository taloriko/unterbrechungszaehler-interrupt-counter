from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return (ROOT / name).read_text(encoding="utf-8")


def write(name, text):
    (ROOT / name).write_text(text, encoding="utf-8")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# Persistent project preference: exactly one normal RF operating mode.
# ---------------------------------------------------------------------------
name = "project_preferences.h"
s = read(name)
s = replace_once(s,
'''enum class SoundMode : uint8_t {
  Fixed = 0,
  Rotate = 1
};
''',
'''enum class SoundMode : uint8_t {
  Fixed = 0,
  Rotate = 1
};

enum class RadioMode : uint8_t {
  Universal433 = 0,
  SomfyRts = 1
};
''', "preferences radio enum")
s = replace_once(s,
'''bool setSoundMode(SoundMode mode);
bool parseSoundMode(const char *value, SoundMode &mode);

const char *language();
''',
'''bool setSoundMode(SoundMode mode);
bool parseSoundMode(const char *value, SoundMode &mode);

RadioMode radioMode();
const char *radioModeName();
bool setRadioMode(RadioMode mode);
bool parseRadioMode(const char *value, RadioMode &mode);

const char *language();
''', "preferences radio api")
write(name, s)

name = "project_config.h"
s = read(name)
s = replace_once(s,
'''constexpr uint8_t SOUND_VOLUME_DEFAULT_PERCENT = 100;
constexpr bool DISPLAY_ENABLED_DEFAULT = true;
''',
'''constexpr uint8_t SOUND_VOLUME_DEFAULT_PERCENT = 100;
constexpr ProjectPreferences::RadioMode RF_MODE_DEFAULT = ProjectPreferences::RadioMode::Universal433;
constexpr bool DISPLAY_ENABLED_DEFAULT = true;
''', "radio mode default")
write(name, s)

name = "project_preferences.cpp"
s = read(name)
s = replace_once(s,
'''SoundMode soundModeValue = ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
char languageValue[12]{};
''',
'''SoundMode soundModeValue = ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
RadioMode radioModeValue = ProjectConfig::RF_MODE_DEFAULT;
char languageValue[12]{};
''', "radio mode state")
s = replace_once(s,
'''SoundMode sanitizedSoundMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(SoundMode::Rotate)
             ? static_cast<SoundMode>(raw)
             : ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
}

DisplayMode sanitizedMode(uint8_t raw) {
''',
'''SoundMode sanitizedSoundMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(SoundMode::Rotate)
             ? static_cast<SoundMode>(raw)
             : ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;
}

RadioMode sanitizedRadioMode(uint8_t raw) {
  return raw <= static_cast<uint8_t>(RadioMode::SomfyRts)
             ? static_cast<RadioMode>(raw)
             : ProjectConfig::RF_MODE_DEFAULT;
}

DisplayMode sanitizedMode(uint8_t raw) {
''', "radio mode sanitize")
s = replace_once(s,
'''  soundModeValue = sanitizedSoundMode(prefs.getUChar("sndmode", static_cast<uint8_t>(ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT)));

  const String storedLanguage = prefs.getString("lang", "");
''',
'''  soundModeValue = sanitizedSoundMode(prefs.getUChar("sndmode", static_cast<uint8_t>(ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT)));
  radioModeValue = sanitizedRadioMode(prefs.getUChar("rfmode", static_cast<uint8_t>(ProjectConfig::RF_MODE_DEFAULT)));

  const String storedLanguage = prefs.getString("lang", "");
''', "radio mode load")
s = replace_once(s,
'''  SerialLog::infof("PROJECT", "Display settings | language=%s | brightness=%u%% | dim-after=%u min | dim-brightness=%u%%",
                   languageValue, static_cast<unsigned int>(brightness), static_cast<unsigned int>(dimAfterMinutes),
                   static_cast<unsigned int>(dimBrightness));
}
''',
'''  SerialLog::infof("PROJECT", "Display settings | language=%s | brightness=%u%% | dim-after=%u min | dim-brightness=%u%%",
                   languageValue, static_cast<unsigned int>(brightness), static_cast<unsigned int>(dimAfterMinutes),
                   static_cast<unsigned int>(dimBrightness));
  SerialLog::infof("PROJECT", "RF settings | mode=%s", radioModeName());
}
''', "radio mode boot log")
s = replace_once(s,
'''bool setSoundMode(SoundMode value) {
  if (soundModeValue == value) return true;
  if (!persistUChar("sndmode", static_cast<uint8_t>(value))) return false;
  soundModeValue = value;
  SerialLog::infof("PROJECT", "Interruption sound mode changed | %s", soundModeName());
  return true;
}

const char *language() { return languageValue[0] ? languageValue : AppConfig::FALLBACK_LANGUAGE; }
''',
'''bool setSoundMode(SoundMode value) {
  if (soundModeValue == value) return true;
  if (!persistUChar("sndmode", static_cast<uint8_t>(value))) return false;
  soundModeValue = value;
  SerialLog::infof("PROJECT", "Interruption sound mode changed | %s", soundModeName());
  return true;
}

RadioMode radioMode() { return radioModeValue; }

const char *radioModeName() {
  return radioModeValue == RadioMode::SomfyRts ? "somfy" : "universal";
}

bool parseRadioMode(const char *value, RadioMode &parsed) {
  if (!value) return false;
  if (strcmp(value, "universal") == 0) parsed = RadioMode::Universal433;
  else if (strcmp(value, "somfy") == 0) parsed = RadioMode::SomfyRts;
  else return false;
  return true;
}

bool setRadioMode(RadioMode value) {
  if (radioModeValue == value) return true;
  if (!persistUChar("rfmode", static_cast<uint8_t>(value))) return false;
  radioModeValue = value;
  SerialLog::infof("PROJECT", "RF operating mode changed | %s", radioModeName());
  return true;
}

const char *language() { return languageValue[0] ? languageValue : AppConfig::FALLBACK_LANGUAGE; }
''', "radio mode functions")
write(name, s)

# ---------------------------------------------------------------------------
# DY-SV17F: tolerate one missed UART answer without hiding real failures.
# ---------------------------------------------------------------------------
name = "hardware_config.h"
s = read(name)
s = replace_once(s,
'''constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 1000;
constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;
''',
'''constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 1000;
constexpr uint8_t AUDIO_PROBE_MAX_ATTEMPTS = 2;
constexpr uint32_t AUDIO_PROBE_RETRY_DELAY_MS = 120;
constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;
''', "audio retry constants")
write(name, s)

name = "audio_dy_sv17f.cpp"
s = read(name)
s = replace_once(s,
'''bool volumePending = true;

enum class WaitKind''',
'''bool volumePending = true;
uint8_t probePlayAttempts = 0;

enum class WaitKind''', "audio retry state")
s = replace_once(s,
'''enum class DeferredAction : uint8_t { None, StartProbe, QueryDevices, QueryCount, VerifyPlay, BootTone };''',
'''enum class DeferredAction : uint8_t { None, StartProbe, RetryProbePlay, QueryDevices, QueryCount, VerifyPlay, BootTone };''', "audio retry deferred enum")
s = replace_once(s,
'''void sendQuery(WaitKind kind, uint8_t command) {
  sendFrame(command);
  startWait(kind, command);
}

bool applyDesiredVolume();
''',
'''void sendQuery(WaitKind kind, uint8_t command) {
  sendFrame(command);
  startWait(kind, command);
}

void sendProbePlayQuery() {
  if (probePlayAttempts < 0xFFU) ++probePlayAttempts;
  sendQuery(WaitKind::ProbePlay, 0x01);
}

bool applyDesiredVolume();
''', "audio retry send helper")
s = replace_once(s,
'''  if (timedOut == WaitKind::ProbePlay) {
    isDetected = false;
    currentPlayState = PlayState::Unknown;
    finishProbe(StatusRegistry::State::NoResponse, "play-state query timeout");
    return;
  }
''',
'''  if (timedOut == WaitKind::ProbePlay) {
    if (probePlayAttempts < HardwareConfig::AUDIO_PROBE_MAX_ATTEMPTS) {
      deferredAction = DeferredAction::RetryProbePlay;
      deferredAtMs = millis() + HardwareConfig::AUDIO_PROBE_RETRY_DELAY_MS;
      SerialLog::warningf("AUDIO", "Play-state query timeout | retry %u/%u scheduled",
                          static_cast<unsigned int>(probePlayAttempts + 1U),
                          static_cast<unsigned int>(HardwareConfig::AUDIO_PROBE_MAX_ATTEMPTS));
      return;
    }

    // BUSY is an independent hardware feedback line. If UART replies were lost
    // but the module is demonstrably playing, report a warning instead of the
    // misleading hard NO RESPONSE state. An idle BUSY line still requires a
    // valid UART reply and therefore remains a real transport failure.
    updateBusyPin();
    if (busyStateKnown && busyState) {
      isDetected = true;
      currentPlayState = PlayState::Playing;
      finishProbe(StatusRegistry::State::Warning, "UART response missing; BUSY confirms active playback");
    } else {
      isDetected = false;
      currentPlayState = PlayState::Unknown;
      finishProbe(StatusRegistry::State::NoResponse, "play-state query timeout after retry");
    }
    return;
  }
''', "audio probe timeout")
s = replace_once(s,
'''  probeHadSecondaryFailure = false;
  setHealth(StatusRegistry::State::Checking);
  sendQuery(WaitKind::ProbePlay, 0x01);
''',
'''  probeHadSecondaryFailure = false;
  probePlayAttempts = 0;
  setHealth(StatusRegistry::State::Checking);
  sendProbePlayQuery();
''', "audio probe start")
s = replace_once(s,
'''    if (action == DeferredAction::StartProbe) {
      if (!probeActive && waitingFor == WaitKind::None) startProbe(true);
    } else if (action == DeferredAction::QueryDevices) {
''',
'''    if (action == DeferredAction::StartProbe) {
      if (!probeActive && waitingFor == WaitKind::None) startProbe(true);
    } else if (action == DeferredAction::RetryProbePlay) {
      if (probeActive && waitingFor == WaitKind::None) sendProbePlayQuery();
    } else if (action == DeferredAction::QueryDevices) {
''', "audio retry dispatcher")
s = replace_once(s,
'''         deferredAction == DeferredAction::StartProbe ||
         deferredAction == DeferredAction::QueryDevices ||
''',
'''         deferredAction == DeferredAction::StartProbe ||
         deferredAction == DeferredAction::RetryProbePlay ||
         deferredAction == DeferredAction::QueryDevices ||
''', "audio retry checking")
write(name, s)

# ---------------------------------------------------------------------------
# Source registry: reuse the existing reserved byte for the RF protocol.
# Size remains exactly 32 B/entry and 332 B total.
# ---------------------------------------------------------------------------
name = "source_registry.h"
s = read(name)
s = replace_once(s,
'''constexpr size_t SOURCE_NAME_MAX_BYTES = 23;

struct Entry {
''',
'''constexpr size_t SOURCE_NAME_MAX_BYTES = 23;

enum class RadioProtocol : uint8_t {
  FixedOok = 0,
  SomfyRts = 1
};

struct Entry {
''', "registry protocol enum")
s = replace_once(s,
'''  bool bound = false;
  uint8_t bitCount = 0;
''',
'''  bool bound = false;
  RadioProtocol protocol = RadioProtocol::FixedOok;
  uint8_t bitCount = 0;
''', "registry entry protocol")
s = replace_once(s,
'''bool consumeFrame(uint32_t code,
                  uint8_t bitCount,
''',
'''bool consumeFrame(RadioProtocol protocol,
                  uint32_t code,
                  uint8_t bitCount,
''', "registry consume signature")
s = replace_once(s,
'''bool renameSource(uint8_t sourceId, const char *name);
bool unbindSource(uint8_t sourceId);
''',
'''const char *radioProtocolName(RadioProtocol protocol);
bool renameSource(uint8_t sourceId, const char *name);
bool unbindSource(uint8_t sourceId);
''', "registry protocol name")
write(name, s)

name = "source_registry.cpp"
s = read(name)
s = replace_once(s,
'''  uint8_t pulseBucket = 0;
  uint8_t reserved = 0;
  uint32_t code = 0;
''',
'''  uint8_t pulseBucket = 0;
  uint8_t protocol = 0;  // v1 previously reserved this byte; zero means legacy Fixed OOK.
  uint32_t code = 0;
''', "stored protocol byte")
s = replace_once(s,
'''    entry.bound = (stored.flags & FLAG_BOUND) != 0;
    entry.bitCount = stored.bitCount;
''',
'''    entry.bound = (stored.flags & FLAG_BOUND) != 0;
    entry.protocol = stored.protocol == static_cast<uint8_t>(RadioProtocol::SomfyRts)
                         ? RadioProtocol::SomfyRts
                         : RadioProtocol::FixedOok;
    entry.bitCount = stored.bitCount;
''', "decode stored protocol")
old = '''bool consumeFrame(uint32_t code,
                  uint8_t bitCount,
                  uint8_t pulseBucket,
                  uint8_t &sourceIdOut,
                  bool &learnedOut) {
  begin();
  sourceIdOut = SOURCE_ID_UNKNOWN;
  learnedOut = false;
  if (bitCount < 16 || bitCount > 32 || code == 0) return false;

  if (learn.active) {
    uint8_t target = learn.targetSourceId;
    if (target == SOURCE_ID_UNKNOWN) target = firstUnassignedSource();
    if (target == SOURCE_ID_UNKNOWN) {
      learn.error = "source_id_exhausted";
      learn.active = false;
      return false;
    }

    for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
      const StoredEntry &existing = registry.entries[i];
      const uint8_t existingId = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
      if ((existing.flags & FLAG_BOUND) == 0 || existingId == target) continue;
      if (existing.code == code && existing.bitCount == bitCount && pulseBucketMatches(existing.pulseBucket, pulseBucket)) {
        learn.error = "already_bound";
        return false;
      }
    }

    const int index = radioIndex(target);
    StoredEntry before = registry.entries[index];
    StoredEntry &stored = registry.entries[index];
    stored.flags = FLAG_ASSIGNED | FLAG_BOUND;
    stored.bitCount = bitCount;
    stored.pulseBucket = pulseBucket;
    stored.code = code;
    if (learn.pendingName[0]) {
      copyName(stored.name, learn.pendingName);
    } else if (!stored.name[0]) {
      char generated[24];
      snprintf(generated, sizeof(generated), "Button %u", static_cast<unsigned int>(target - SOURCE_ID_RADIO_FIRST + 1U));
      copyName(stored.name, generated);
    }
    if (!save()) {
      registry.entries[index] = before;
      decodeEntries();
      learn.error = "persist_failed";
      return false;
    }

    learn.active = false;
    learn.error = "none";
    sourceIdOut = target;
    learnedOut = true;
    SerialLog::successf("RF433", "Button learned | source=%u | name=%s | bits=%u | code=0x%08lX",
                        static_cast<unsigned int>(target), sourceName(target), static_cast<unsigned int>(bitCount),
                        static_cast<unsigned long>(code));
    return true;
  }

  for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
    const StoredEntry &stored = registry.entries[i];
    if ((stored.flags & FLAG_BOUND) == 0) continue;
    if (stored.code == code && stored.bitCount == bitCount && pulseBucketMatches(stored.pulseBucket, pulseBucket)) {
      sourceIdOut = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
      return true;
    }
  }
  return false;
}
'''
new = '''bool consumeFrame(RadioProtocol protocol,
                  uint32_t code,
                  uint8_t bitCount,
                  uint8_t pulseBucket,
                  uint8_t &sourceIdOut,
                  bool &learnedOut) {
  begin();
  sourceIdOut = SOURCE_ID_UNKNOWN;
  learnedOut = false;
  if (code == 0) return false;
  if (protocol == RadioProtocol::FixedOok && (bitCount < 16 || bitCount > 32)) return false;
  if (protocol == RadioProtocol::SomfyRts && bitCount != 56) return false;

  const uint8_t protocolByte = static_cast<uint8_t>(protocol);
  auto matches = [&](const StoredEntry &stored) {
    if (stored.protocol != protocolByte || stored.code != code) return false;
    if (protocol == RadioProtocol::SomfyRts) return true;  // stable 24-bit address is the identity.
    return stored.bitCount == bitCount && pulseBucketMatches(stored.pulseBucket, pulseBucket);
  };

  if (learn.active) {
    uint8_t target = learn.targetSourceId;
    if (target == SOURCE_ID_UNKNOWN) target = firstUnassignedSource();
    if (target == SOURCE_ID_UNKNOWN) {
      learn.error = "source_id_exhausted";
      learn.active = false;
      return false;
    }

    for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
      const StoredEntry &existing = registry.entries[i];
      const uint8_t existingId = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
      if ((existing.flags & FLAG_BOUND) == 0 || existingId == target) continue;
      if (matches(existing)) {
        learn.error = "already_bound";
        return false;
      }
    }

    const int index = radioIndex(target);
    StoredEntry before = registry.entries[index];
    StoredEntry &stored = registry.entries[index];
    stored.flags = FLAG_ASSIGNED | FLAG_BOUND;
    stored.protocol = protocolByte;
    stored.bitCount = bitCount;
    stored.pulseBucket = protocol == RadioProtocol::SomfyRts ? 0U : pulseBucket;
    stored.code = code;
    if (learn.pendingName[0]) {
      copyName(stored.name, learn.pendingName);
    } else if (!stored.name[0]) {
      char generated[24];
      snprintf(generated, sizeof(generated), "Button %u", static_cast<unsigned int>(target - SOURCE_ID_RADIO_FIRST + 1U));
      copyName(stored.name, generated);
    }
    if (!save()) {
      registry.entries[index] = before;
      decodeEntries();
      learn.error = "persist_failed";
      return false;
    }

    learn.active = false;
    learn.error = "none";
    sourceIdOut = target;
    learnedOut = true;
    SerialLog::successf("RF433", "Button learned | source=%u | name=%s | protocol=%s | bits=%u | code=0x%08lX",
                        static_cast<unsigned int>(target), sourceName(target), radioProtocolName(protocol),
                        static_cast<unsigned int>(bitCount), static_cast<unsigned long>(code));
    return true;
  }

  for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
    const StoredEntry &stored = registry.entries[i];
    if ((stored.flags & FLAG_BOUND) == 0) continue;
    if (matches(stored)) {
      sourceIdOut = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
      return true;
    }
  }
  return false;
}

const char *radioProtocolName(RadioProtocol protocol) {
  return protocol == RadioProtocol::SomfyRts ? "somfy" : "universal";
}
'''
s = replace_once(s, old, new, "registry consume implementation")
s = replace_once(s,
'''  registry.entries[index].flags &= static_cast<uint8_t>(~FLAG_BOUND);
  registry.entries[index].bitCount = 0;
''',
'''  registry.entries[index].flags &= static_cast<uint8_t>(~FLAG_BOUND);
  registry.entries[index].protocol = static_cast<uint8_t>(RadioProtocol::FixedOok);
  registry.entries[index].bitCount = 0;
''', "registry unbind protocol")
write(name, s)

# ---------------------------------------------------------------------------
# CC1101: selected normal mode, selected-mode receive test, Somfy normal frames.
# ---------------------------------------------------------------------------
name = "rf433_cc1101.h"
s = read(name)
s = replace_once(s,
'''bool startReceiveTest();
void cancelReceiveTest();
''',
'''bool setOperatingProtocol(Protocol protocol);
Protocol operatingProtocol();

bool startReceiveTest();
void cancelReceiveTest();
''', "rf driver operating api")
s = s.replace('const char *error = "none";', 'const char *error = "";')
write(name, s)

name = "rf433_cc1101.cpp"
s = read(name)
s = s.replace('constexpr uint32_t RECEIVE_TEST_SWITCH_MS = 1500;\n', '')
s = replace_once(s,
'''volatile CaptureMode captureMode = CaptureMode::FixedOok;
bool receiveTestSomfyPhase = false;
uint32_t receiveTestPhaseStartedMs = 0;
''',
'''volatile CaptureMode captureMode = CaptureMode::FixedOok;
Protocol operatingProtocolValue = Protocol::FixedOok;
''', "rf operating state")
s = replace_once(s,
'''bool verifyFixedConfiguration() {
  return readConfigRegister(0x02) == 0x0D &&
         readConfigRegister(0x08) == 0x32 &&
         readConfigRegister(0x0D) == 0x10 &&
         readConfigRegister(0x0E) == 0xB0 &&
         readConfigRegister(0x0F) == 0x71 &&
         readConfigRegister(0x12) == 0x30;
}
''',
'''bool verifyConfiguration(CaptureMode mode) {
  const bool somfy = mode == CaptureMode::SomfyRts;
  return readConfigRegister(0x02) == 0x0D &&
         readConfigRegister(0x08) == 0x32 &&
         readConfigRegister(0x0D) == 0x10 &&
         readConfigRegister(0x0E) == (somfy ? 0xAB : 0xB0) &&
         readConfigRegister(0x0F) == (somfy ? 0x85 : 0x71) &&
         readConfigRegister(0x12) == 0x30;
}
''', "rf verify config")
s = replace_once(s,
'''bool sameFrame(const Frame &a, const Frame &b) {
  return a.code == b.code && a.bitCount == b.bitCount && similarBucket(a.pulseBucket, b.pulseBucket);
}
''',
'''bool sameFrame(const Frame &a, const Frame &b) {
  if (a.protocol != b.protocol || a.code != b.code) return false;
  if (a.protocol == Protocol::SomfyRts) {
    return a.rollingCode == b.rollingCode && a.command == b.command;
  }
  return a.bitCount == b.bitCount && similarBucket(a.pulseBucket, b.pulseBucket);
}
''', "rf same frame")
old = '''void processSomfyReady() {
  if (!somfyReady) return;
  uint8_t encoded[7]{};
  uint8_t syncCount = 0;
  noInterrupts();
  if (somfyReady) {
    for (uint8_t i = 0; i < 7; ++i) encoded[i] = somfyReadyPayload[i];
    syncCount = somfyReadySyncCount;
    somfyReady = false;
  }
  interrupts();

  Frame candidate;
  if (!decodeSomfyPayload(encoded, candidate)) {
    ++currentInfo.rejectedFrames;
    return;
  }

  receiveTestFrame = candidate;
  receiveTestActiveFlag = false;
  receiveTestResultText = "somfy_received";
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("RF433", "Somfy RTS test passed | sync=%u | address=0x%06lX | rolling=%u | command=%s",
                      static_cast<unsigned int>(syncCount),
                      static_cast<unsigned long>(candidate.code),
                      static_cast<unsigned int>(candidate.rollingCode),
                      somfyCommandName(candidate.command));
  if (!applyCaptureMode(CaptureMode::FixedOok)) {
    failReceiver("cc1101_restore_fixed_failed", StatusRegistry::State::Error);
  } else {
    currentInfo.configVerified = verifyFixedConfiguration();
  }
}
'''
new = '''void processSomfyReady() {
  if (!somfyReady) return;
  uint8_t encoded[7]{};
  uint8_t syncCount = 0;
  noInterrupts();
  if (somfyReady) {
    for (uint8_t i = 0; i < 7; ++i) encoded[i] = somfyReadyPayload[i];
    syncCount = somfyReadySyncCount;
    somfyReady = false;
  }
  interrupts();

  Frame candidate;
  if (!decodeSomfyPayload(encoded, candidate)) {
    ++currentInfo.rejectedFrames;
    return;
  }

  const uint32_t nowMs = millis();
  if (sameFrame(candidate, currentInfo.lastFrame) && static_cast<uint32_t>(nowMs - lastEmitMs) < PRESS_DEDUPE_MS) {
    return;  // repeated RTS telegram of the same physical press
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
                        static_cast<unsigned int>(syncCount),
                        static_cast<unsigned long>(candidate.code),
                        static_cast<unsigned int>(candidate.rollingCode),
                        somfyCommandName(candidate.command));
  }

  emittedFrame = candidate;
  emittedFrame.diagnostic = diagnostic;
  emittedAvailable = true;
  currentInfo.lastFrame = emittedFrame;
  ++currentInfo.decodedFrames;
  lastEmitMs = nowMs;
}
'''
s = replace_once(s, old, new, "rf somfy production processing")
s = replace_once(s,
'''    setHealth(StatusRegistry::State::Ok);
    SerialLog::successf("RF433", "Receive test passed | bits=%u | code=0x%08lX",
''',
'''    currentInfo.error = "";
    setHealth(StatusRegistry::State::Ok);
    SerialLog::successf("RF433", "Receive test passed | bits=%u | code=0x%08lX",
''', "rf fixed diagnostic health")
s = s.replace('currentInfo.configVerified = verifyFixedConfiguration();', 'currentInfo.configVerified = verifyConfiguration(CaptureMode::FixedOok);', 1)
s = replace_once(s,
'''  captureMode = CaptureMode::FixedOok;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
''',
'''  captureMode = CaptureMode::FixedOok;
  operatingProtocolValue = Protocol::FixedOok;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
''', "rf begin operating mode")
s = s.replace('  currentInfo.error = "none";\n', '  currentInfo.error = "";\n')
old_probe = '''bool probe() {
  if (!enabled() || receiveTestActiveFlag) return false;
  checkedAtMs = millis();
  if (!currentInfo.ready) {
    begin();
    return true;
  }

  setHealth(StatusRegistry::State::Checking);
  const uint8_t part = readStatusRegister(PARTNUM);
  const uint8_t version = readStatusRegister(VERSION);
  if (part == 0xFFU || version == 0xFFU) {
    detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));
    failReceiver("cc1101_probe_no_response", StatusRegistry::State::NoResponse);
    SerialLog::error("RF433", "Manual probe: CC1101 did not answer on SPI");
    return true;
  }
  if (!strobe(SRX)) {
    detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));
    failReceiver("cc1101_probe_rx_failed", StatusRegistry::State::Error);
    return true;
  }
  if (!verifyFixedConfiguration()) {
    failReceiver("cc1101_probe_readback_failed", StatusRegistry::State::Error);
    SerialLog::error("RF433", "Manual probe: configuration register readback failed");
    return true;
  }

  currentInfo.configVerified = true;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
  currentInfo.partNumber = part;
  currentInfo.version = version;
  currentInfo.error = "";
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("RF433", "Manual probe: OK | part=0x%02X | version=0x%02X", part, version);
  return true;
}
'''
new_probe = '''bool probe() {
  if (!enabled() || receiveTestActiveFlag) return false;
  checkedAtMs = millis();
  if (!currentInfo.ready) {
    begin();
    return true;
  }

  setHealth(StatusRegistry::State::Checking);
  const uint8_t part = readStatusRegister(PARTNUM);
  const uint8_t version = readStatusRegister(VERSION);
  if (part == 0xFFU || version == 0xFFU) {
    detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));
    failReceiver("cc1101_probe_no_response", StatusRegistry::State::NoResponse);
    SerialLog::error("RF433", "Manual probe: CC1101 did not answer on SPI");
    return true;
  }
  if (!strobe(SRX)) {
    detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));
    failReceiver("cc1101_probe_rx_failed", StatusRegistry::State::Error);
    return true;
  }
  const CaptureMode activeMode = captureMode;
  if (!verifyConfiguration(activeMode)) {
    failReceiver("cc1101_probe_readback_failed", StatusRegistry::State::Error);
    SerialLog::error("RF433", "Manual probe: configuration register readback failed");
    return true;
  }

  currentInfo.configVerified = true;
  currentInfo.activeFrequencyHz = activeMode == CaptureMode::SomfyRts
                                      ? HardwareConfig::RF433_SOMFY_FREQUENCY_HZ
                                      : HardwareConfig::RF433_FREQUENCY_HZ;
  currentInfo.partNumber = part;
  currentInfo.version = version;
  currentInfo.error = "";
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("RF433", "Manual probe: OK | part=0x%02X | version=0x%02X | mode=%s",
                      part, version, protocolName(operatingProtocolValue));
  return true;
}
'''
s = replace_once(s, old_probe, new_probe, "rf probe current mode")
old_tail = '''bool startReceiveTest() {
  if (!enabled() || !currentInfo.ready || receiveTestActiveFlag) return false;
  receiveTestActiveFlag = true;
  receiveTestStartedMs = millis();
  receiveTestPhaseStartedMs = receiveTestStartedMs;
  receiveTestSomfyPhase = true;
  receiveTestResultText = "waiting";
  receiveTestFrame = Frame{};
  checkedAtMs = receiveTestStartedMs;
  setHealth(StatusRegistry::State::Checking);

  // Start on Somfy's 433.42 MHz because a normal 433.92 MHz fixed-code sender
  // is already covered by the production receive path. The test alternates both.
  if (!applyCaptureMode(CaptureMode::SomfyRts)) {
    receiveTestActiveFlag = false;
    failReceiver("cc1101_test_retune_failed", StatusRegistry::State::Error);
    return false;
  }
  SerialLog::infof("RF433", "Auto receive test started | window=%lu ms | scans 433.92 fixed OOK + 433.42 Somfy RTS | press repeatedly",
                   static_cast<unsigned long>(RECEIVE_TEST_MS));
  return true;
}

void cancelReceiveTest() {
  if (!receiveTestActiveFlag) return;
  receiveTestActiveFlag = false;
  receiveTestResultText = "cancelled";
  checkedAtMs = millis();
  if (captureMode != CaptureMode::FixedOok && !applyCaptureMode(CaptureMode::FixedOok)) {
    failReceiver("cc1101_restore_fixed_failed", StatusRegistry::State::Error);
    return;
  }
  currentInfo.configVerified = verifyFixedConfiguration();
  setHealth(currentInfo.ready ? StatusRegistry::State::Ok : StatusRegistry::State::Error);
  SerialLog::info("RF433", "Receive test cancelled");
}

bool receiveTestActive() { return receiveTestActiveFlag; }
const char *receiveTestResult() { return receiveTestResultText; }
uint32_t receiveTestRemainingMs() {
  if (!receiveTestActiveFlag) return 0;
  const uint32_t elapsedMs = static_cast<uint32_t>(millis() - receiveTestStartedMs);
  return elapsedMs < RECEIVE_TEST_MS ? RECEIVE_TEST_MS - elapsedMs : 0;
}
const Frame &lastTestFrame() { return receiveTestFrame; }

void update() {
  if (!currentInfo.ready) return;
  if (captureMode == CaptureMode::SomfyRts) processSomfyReady();
  else processReadyFrame();

  if (!receiveTestActiveFlag) return;
  const uint32_t nowMs = millis();
  if (static_cast<uint32_t>(nowMs - receiveTestStartedMs) >= RECEIVE_TEST_MS) {
    receiveTestActiveFlag = false;
    receiveTestResultText = "timeout";
    checkedAtMs = nowMs;
    if (captureMode != CaptureMode::FixedOok && !applyCaptureMode(CaptureMode::FixedOok)) {
      failReceiver("cc1101_restore_fixed_failed", StatusRegistry::State::Error);
      return;
    }
    currentInfo.configVerified = verifyFixedConfiguration();
    setHealth(StatusRegistry::State::Ok);
    SerialLog::warning("RF433", "Auto receive test finished without valid fixed-code or Somfy RTS frame");
    return;
  }

  if (static_cast<uint32_t>(nowMs - receiveTestPhaseStartedMs) < RECEIVE_TEST_SWITCH_MS) return;
  const CaptureMode nextMode = receiveTestSomfyPhase ? CaptureMode::FixedOok : CaptureMode::SomfyRts;
  if (!applyCaptureMode(nextMode)) {
    receiveTestActiveFlag = false;
    failReceiver("cc1101_test_retune_failed", StatusRegistry::State::Error);
    return;
  }
  receiveTestSomfyPhase = nextMode == CaptureMode::SomfyRts;
  receiveTestPhaseStartedMs = nowMs;
}
'''
new_tail = '''bool setOperatingProtocol(Protocol protocol) {
  if (!enabled() || !currentInfo.ready) return false;
  if (receiveTestActiveFlag) cancelReceiveTest();

  const CaptureMode nextMode = protocol == Protocol::SomfyRts ? CaptureMode::SomfyRts : CaptureMode::FixedOok;
  const CaptureMode previousMode = captureMode;
  const Protocol previousProtocol = operatingProtocolValue;
  if (nextMode == previousMode) {
    operatingProtocolValue = protocol;
    currentInfo.activeFrequencyHz = nextMode == CaptureMode::SomfyRts
                                        ? HardwareConfig::RF433_SOMFY_FREQUENCY_HZ
                                        : HardwareConfig::RF433_FREQUENCY_HZ;
    currentInfo.configVerified = verifyConfiguration(nextMode);
    if (!currentInfo.configVerified) return false;
    currentInfo.error = "";
    setHealth(StatusRegistry::State::Ok);
    return true;
  }

  if (!applyCaptureMode(nextMode) || !verifyConfiguration(nextMode)) {
    const bool restored = applyCaptureMode(previousMode) && verifyConfiguration(previousMode);
    operatingProtocolValue = previousProtocol;
    currentInfo.configVerified = restored;
    currentInfo.error = restored ? "cc1101_mode_switch_failed" : "cc1101_mode_restore_failed";
    checkedAtMs = millis();
    setHealth(restored ? StatusRegistry::State::Warning : StatusRegistry::State::Error);
    return false;
  }

  operatingProtocolValue = protocol;
  currentInfo.configVerified = true;
  currentInfo.error = "";
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("RF433", "Operating mode active | protocol=%s | frequency=%lu Hz",
                      protocolName(protocol), static_cast<unsigned long>(currentInfo.activeFrequencyHz));
  return true;
}

Protocol operatingProtocol() { return operatingProtocolValue; }

bool startReceiveTest() {
  if (!enabled() || !currentInfo.ready || receiveTestActiveFlag) return false;
  receiveTestActiveFlag = true;
  receiveTestStartedMs = millis();
  receiveTestResultText = "waiting";
  receiveTestFrame = Frame{};
  checkedAtMs = receiveTestStartedMs;
  setHealth(StatusRegistry::State::Checking);
  SerialLog::infof("RF433", "Receive test started | window=%lu ms | mode=%s | frequency=%lu Hz | press repeatedly",
                   static_cast<unsigned long>(RECEIVE_TEST_MS), protocolName(operatingProtocolValue),
                   static_cast<unsigned long>(currentInfo.activeFrequencyHz));
  return true;
}

void cancelReceiveTest() {
  if (!receiveTestActiveFlag) return;
  receiveTestActiveFlag = false;
  receiveTestResultText = "cancelled";
  checkedAtMs = millis();
  currentInfo.error = "";
  setHealth(currentInfo.ready ? StatusRegistry::State::Ok : StatusRegistry::State::Error);
  SerialLog::info("RF433", "Receive test cancelled");
}

bool receiveTestActive() { return receiveTestActiveFlag; }
const char *receiveTestResult() { return receiveTestResultText; }
uint32_t receiveTestRemainingMs() {
  if (!receiveTestActiveFlag) return 0;
  const uint32_t elapsedMs = static_cast<uint32_t>(millis() - receiveTestStartedMs);
  return elapsedMs < RECEIVE_TEST_MS ? RECEIVE_TEST_MS - elapsedMs : 0;
}
const Frame &lastTestFrame() { return receiveTestFrame; }

void update() {
  if (!currentInfo.ready) return;
  if (captureMode == CaptureMode::SomfyRts) processSomfyReady();
  else processReadyFrame();

  if (!receiveTestActiveFlag) return;
  const uint32_t nowMs = millis();
  if (static_cast<uint32_t>(nowMs - receiveTestStartedMs) < RECEIVE_TEST_MS) return;

  receiveTestActiveFlag = false;
  receiveTestResultText = "timeout";
  checkedAtMs = nowMs;
  currentInfo.error = "";
  setHealth(StatusRegistry::State::Ok);
  SerialLog::warningf("RF433", "Receive test finished without valid frame | mode=%s",
                      protocolName(operatingProtocolValue));
}
'''
s = replace_once(s, old_tail, new_tail, "rf selected mode test")
# The first begin verification was intentionally fixed; remaining stale calls are bugs.
if "verifyFixedConfiguration" in s or "receiveTestPhaseStartedMs" in s or "receiveTestSomfyPhase" in s:
    raise SystemExit("stale RF auto-scan symbols remain")
write(name, s)

# ---------------------------------------------------------------------------
# Project RF service: map selected protocol to stable source IDs and persist mode
# only after successful hardware retune.
# ---------------------------------------------------------------------------
write("rf433_service.h", '''#pragma once

#include <Arduino.h>

#include "project_preferences.h"

namespace Rf433Service {

bool begin();
void update();
bool ready();

bool setOperatingMode(ProjectPreferences::RadioMode mode);

bool startLearn(const char *name, uint8_t targetSourceId = 0);
void cancelLearn();
bool renameSource(uint8_t sourceId, const char *name);
bool unbindSource(uint8_t sourceId);

}  // namespace Rf433Service
''')

write("rf433_service.cpp", '''#include "rf433_service.h"

#include "interruption_service.h"
#include "interruption_types.h"
#include "rf433_cc1101.h"
#include "serial_log.h"
#include "source_registry.h"

namespace Rf433Service {
namespace {
bool started = false;

Rf433Cc1101::Protocol driverProtocol(ProjectPreferences::RadioMode mode) {
  return mode == ProjectPreferences::RadioMode::SomfyRts
             ? Rf433Cc1101::Protocol::SomfyRts
             : Rf433Cc1101::Protocol::FixedOok;
}

SourceRegistry::RadioProtocol registryProtocol(Rf433Cc1101::Protocol protocol) {
  return protocol == Rf433Cc1101::Protocol::SomfyRts
             ? SourceRegistry::RadioProtocol::SomfyRts
             : SourceRegistry::RadioProtocol::FixedOok;
}
}  // namespace

bool begin() {
  if (started) return Rf433Cc1101::info().ready;
  started = true;
  SourceRegistry::begin();
  if (!Rf433Cc1101::begin()) return false;
  if (!Rf433Cc1101::setOperatingProtocol(driverProtocol(ProjectPreferences::radioMode()))) {
    SerialLog::error("RF433", "Persisted RF mode could not be applied");
    return false;
  }
  return true;
}

void update() {
  SourceRegistry::update();
  Rf433Cc1101::update();

  Rf433Cc1101::Frame frame;
  while (Rf433Cc1101::pollFrame(frame)) {
    if (frame.diagnostic) {
      SerialLog::infof("RF433", "Diagnostic frame consumed without interruption | protocol=%s | bits=%u | code=0x%08lX",
                       Rf433Cc1101::protocolName(frame.protocol), static_cast<unsigned int>(frame.bitCount),
                       static_cast<unsigned long>(frame.code));
      continue;
    }

    uint8_t sourceId = SourceRegistry::SOURCE_ID_UNKNOWN;
    bool learned = false;
    const SourceRegistry::RadioProtocol protocol = registryProtocol(frame.protocol);
    const bool known = SourceRegistry::consumeFrame(protocol, frame.code, frame.bitCount, frame.pulseBucket, sourceId, learned);
    if (learned) {
      // The learning press is configuration, not an interruption event.
      continue;
    }
    if (!known || sourceId < SourceRegistry::SOURCE_ID_RADIO_FIRST) {
      SerialLog::infof("RF433", "Unassigned radio frame | protocol=%s | bits=%u | code=0x%08lX",
                       SourceRegistry::radioProtocolName(protocol), static_cast<unsigned int>(frame.bitCount),
                       static_cast<unsigned long>(frame.code));
      continue;
    }

    SerialLog::infof("RF433", "Matched button | source=%u | name=%s | protocol=%s | code=0x%08lX",
                     static_cast<unsigned int>(sourceId), SourceRegistry::sourceName(sourceId),
                     SourceRegistry::radioProtocolName(protocol), static_cast<unsigned long>(frame.code));
    InterruptionService::capture(InterruptionTypes::EventSource::Radio, sourceId);
  }
}

bool ready() { return Rf433Cc1101::info().ready; }

bool setOperatingMode(ProjectPreferences::RadioMode mode) {
  if (!ready() || SourceRegistry::learnState().active) return false;
  const ProjectPreferences::RadioMode previous = ProjectPreferences::radioMode();
  if (mode == previous) return true;

  if (Rf433Cc1101::receiveTestActive()) Rf433Cc1101::cancelReceiveTest();
  if (!Rf433Cc1101::setOperatingProtocol(driverProtocol(mode))) return false;

  // Hardware first, NVS second. If persistence fails, restore the previously
  // active receiver mode so runtime and stored configuration cannot diverge.
  if (!ProjectPreferences::setRadioMode(mode)) {
    Rf433Cc1101::setOperatingProtocol(driverProtocol(previous));
    return false;
  }
  return true;
}

bool startLearn(const char *name, uint8_t targetSourceId) {
  if (!ready()) return false;
  if (Rf433Cc1101::receiveTestActive()) Rf433Cc1101::cancelReceiveTest();
  return SourceRegistry::startLearn(name, targetSourceId);
}

void cancelLearn() { SourceRegistry::cancelLearn(); }

bool renameSource(uint8_t sourceId, const char *name) {
  return SourceRegistry::renameSource(sourceId, name);
}

bool unbindSource(uint8_t sourceId) {
  return SourceRegistry::unbindSource(sourceId);
}

}  // namespace Rf433Service
''')

# ---------------------------------------------------------------------------
# JSON/API and hardware UI data.
# ---------------------------------------------------------------------------
name = "interruption_api.cpp"
s = read(name)
s = replace_once(s,
'''  fieldString(out, "soundMode", ProjectPreferences::soundModeName());
  fieldUInt(out, "soundTrackCount", AudioDySv17f::musicCount());
''',
'''  fieldString(out, "soundMode", ProjectPreferences::soundModeName());
  fieldUInt(out, "soundTrackCount", AudioDySv17f::musicCount());
  fieldString(out, "rfMode", ProjectPreferences::radioModeName());
''', "preferences json rf mode")
write(name, s)

name = "web_server.cpp"
s = read(name)
s = replace_once(s,
'''  const bool hasSoundMode = server.hasArg("soundMode");
  const bool hasLanguage = server.hasArg("language");
''',
'''  const bool hasSoundMode = server.hasArg("soundMode");
  const bool hasRfMode = server.hasArg("rfMode");
  const bool hasLanguage = server.hasArg("language");
''', "web rf mode flag")
s = replace_once(s,
'''                         static_cast<uint8_t>(hasTrack) + static_cast<uint8_t>(hasSoundMode) +
                         static_cast<uint8_t>(hasLanguage) + static_cast<uint8_t>(hasDisplayEnabled) +
''',
'''                         static_cast<uint8_t>(hasTrack) + static_cast<uint8_t>(hasSoundMode) +
                         static_cast<uint8_t>(hasRfMode) + static_cast<uint8_t>(hasLanguage) + static_cast<uint8_t>(hasDisplayEnabled) +
''', "web rf mode field count")
s = replace_once(s,
'''    ok = ProjectPreferences::setSoundMode(value);
  } else if (hasLanguage) {
''',
'''    ok = ProjectPreferences::setSoundMode(value);
  } else if (hasRfMode) {
    ProjectPreferences::RadioMode value = ProjectPreferences::RadioMode::Universal433;
    if (!ProjectPreferences::parseRadioMode(server.arg("rfMode").c_str(), value)) {
      server.send(400, "application/json; charset=utf-8", "{\\"ok\\":false,\\"error\\":\\"invalid_rf_mode\\"}");
      return;
    }
    ok = Rf433Service::setOperatingMode(value);
  } else if (hasLanguage) {
''', "web rf mode handler")
write(name, s)

name = "rf433_api.cpp"
s = read(name)
s = replace_once(s,
'''#include "json_utils.h"
#include "rf433_cc1101.h"
''',
'''#include "json_utils.h"
#include "project_preferences.h"
#include "rf433_cc1101.h"
''', "rf api preferences include")
s = replace_once(s,
'''  if (entry && entry->assigned) {
    fieldUInt(out, "bitCount", entry->bitCount);
''',
'''  if (entry && entry->assigned) {
    fieldString(out, "protocol", SourceRegistry::radioProtocolName(entry->protocol));
    fieldUInt(out, "bitCount", entry->bitCount);
''', "rf source protocol json")
s = replace_once(s,
'''  fieldBool(out, "ready", radio.ready);
  fieldString(out, "error", radio.error);
  fieldUInt(out, "frequencyHz", 433920000UL);
''',
'''  fieldBool(out, "ready", radio.ready);
  fieldString(out, "error", radio.error);
  fieldString(out, "mode", ProjectPreferences::radioModeName());
  fieldUInt(out, "frequencyHz", radio.activeFrequencyHz);
''', "rf api actual mode")
s = replace_once(s,
'''  fieldUInt(out, "lastCode", radio.lastFrame.code);
  fieldUInt(out, "lastBits", radio.lastFrame.bitCount);
  fieldUInt(out, "lastPulseBucket", radio.lastFrame.pulseBucket, false);
''',
'''  fieldUInt(out, "lastCode", radio.lastFrame.code);
  fieldString(out, "lastProtocol", radio.lastFrame.protocol == Rf433Cc1101::Protocol::SomfyRts ? "somfy" : "universal");
  fieldUInt(out, "lastBits", radio.lastFrame.bitCount);
  fieldUInt(out, "lastRollingCode", radio.lastFrame.rollingCode);
  fieldUInt(out, "lastCommand", radio.lastFrame.command);
  fieldUInt(out, "lastPulseBucket", radio.lastFrame.pulseBucket, false);
''', "rf api last protocol")
write(name, s)

name = "hardware_registry.cpp"
s = read(name)
s = replace_once(s,
'''#include "json_utils.h"
#include "rtc_ds3231.h"
''',
'''#include "json_utils.h"
#include "project_preferences.h"
#include "rtc_ds3231.h"
''', "hardware preferences include")
s = replace_once(s,
'''  DisplaySh1106::begin();
  AudioDySv17f::begin();
  Rf433Cc1101::begin();
''',
'''  DisplaySh1106::begin();
  // Configure the custom SPI routing before UART2. This leaves GPIO18/19 under
  // the final ownership of the DY-SV17F UART after every optional bus is set up.
  Rf433Cc1101::begin();
  AudioDySv17f::begin();
''', "hardware RF before audio")
s = replace_once(s,
'''    appendInfoString(out, first, "hardware.info.model", "CC1101 / RF1100SE");
    appendInfoString(out, first, "hardware.info.transport", "SPI / OOK: 433.92 MHz + Somfy RTS 433.42 MHz Test");
''',
'''    appendInfoString(out, first, "hardware.info.model", "CC1101 / RF1100SE");
    appendInfoString(out, first, "hardware.info.transport", "SPI / OOK");
    appendInfoString(out, first, "hardware.info.rfMode", ProjectPreferences::radioModeName(), "rfMode");
''', "hardware selected RF mode")
write(name, s)

# ---------------------------------------------------------------------------
# UI: mode selection lives with source management on Device/Hardware, not Home.
# ---------------------------------------------------------------------------
name = "ui-src/app.js"
s = read(name)
anchor = '''  Object.assign(I18N.de, RF433_I18N.de);
  Object.assign(I18N.en, RF433_I18N.en);
'''
insert = '''  Object.assign(RF433_I18N.de, {
    'hardware.info.rfMode': 'Betriebsart', 'rf433.mode': 'Funkmodus',
    'rf433.mode.universal': 'Universal 433', 'rf433.mode.somfy': 'Somfy RTS',
    'rf433.mode.hint': 'Es ist immer genau ein Empfangsmodus aktiv. Vorhandene Bindungen des anderen Modus bleiben gespeichert.',
    'rf433.protocol': 'Protokoll', 'action.rf433Test': 'Empfang testen (10 s)',
    'rf433.test.waiting': 'Aktiven Funkmodus testen – Taste mehrfach drücken …',
    'rf433.test.timeout': 'Im aktiven Funkmodus wurde kein gültiges Telegramm empfangen.'
  });
  Object.assign(RF433_I18N.en, {
    'hardware.info.rfMode': 'Operating mode', 'rf433.mode': 'Radio mode',
    'rf433.mode.universal': 'Universal 433', 'rf433.mode.somfy': 'Somfy RTS',
    'rf433.mode.hint': 'Exactly one receive mode is active. Existing bindings from the other mode remain stored.',
    'rf433.protocol': 'Protocol', 'action.rf433Test': 'Test reception (10 s)',
    'rf433.test.waiting': 'Testing the active radio mode – press repeatedly …',
    'rf433.test.timeout': 'No valid telegram was received in the active radio mode.'
  });
  Object.assign(RF433_I18N.it, {
    'hardware.info.rfMode': 'Modalità operativa', 'rf433.mode': 'Modalità radio',
    'rf433.mode.universal': 'Universal 433', 'rf433.mode.somfy': 'Somfy RTS',
    'rf433.mode.hint': 'È attiva una sola modalità di ricezione. Le associazioni dell’altra modalità restano memorizzate.',
    'rf433.protocol': 'Protocollo', 'action.rf433Test': 'Test ricezione (10 s)',
    'rf433.test.waiting': 'Test della modalità radio attiva – premere più volte …',
    'rf433.test.timeout': 'Nessun telegramma valido ricevuto nella modalità radio attiva.'
  });
  Object.assign(RF433_I18N.fr, {
    'hardware.info.rfMode': 'Mode de fonctionnement', 'rf433.mode': 'Mode radio',
    'rf433.mode.universal': 'Universal 433', 'rf433.mode.somfy': 'Somfy RTS',
    'rf433.mode.hint': 'Un seul mode de réception est actif. Les liaisons de l’autre mode restent enregistrées.',
    'rf433.protocol': 'Protocole', 'action.rf433Test': 'Tester réception (10 s)',
    'rf433.test.waiting': 'Test du mode radio actif – appuyer plusieurs fois …',
    'rf433.test.timeout': 'Aucun télégramme valide reçu dans le mode radio actif.'
  });
  Object.assign(RF433_I18N.swg, {
    'hardware.info.rfMode': 'Betriebsart', 'rf433.mode': 'Funkmodus',
    'rf433.mode.universal': 'Universal 433', 'rf433.mode.somfy': 'Somfy RTS',
    'rf433.mode.hint': 'Es isch immer bloß oi Empfangsmodus aktiv. D Bindunga vom andere Modus bleibet gspeichert.',
    'rf433.protocol': 'Protokoll', 'action.rf433Test': 'Empfang testa (10 s)',
    'rf433.test.waiting': 'Aktiva Funkmodus testa – Knopf mehrafach drucka …',
    'rf433.test.timeout': 'Em aktiva Funkmodus isch koi gültigs Telegramm komma.'
  });

  Object.assign(I18N.de, RF433_I18N.de);
  Object.assign(I18N.en, RF433_I18N.en);
'''
s = replace_once(s, anchor, insert, "RF mode translations")
s = replace_once(s,
'''projectSettings: { soundEnabled: true, soundVolume: 100, soundMode: 'rotate', soundTrack: 2, soundTrackCount: 0, language: 'en',''',
'''projectSettings: { soundEnabled: true, soundVolume: 100, soundMode: 'rotate', soundTrack: 2, soundTrackCount: 0, rfMode: 'universal', language: 'en',''', "UI RF mode state")
old = '''    const head = el('div', 'rf433-head');
    const capacity = el('strong', 'rf433-capacity');
    const refreshButton = el('button', 'button'); refreshButton.type = 'button'; refreshButton.dataset.rfAction = 'refresh'; refreshButton.textContent = t('rf433.refresh');
    head.append(capacity, refreshButton);
    const learnInfo = el('div', 'rf433-learn-info');
    const lastFrame = el('div', 'form-note rf433-last-frame');
    const list = el('div', 'rf433-source-list');
    const message = el('div', 'project-setting-message'); message.setAttribute('aria-live', 'polite'); message.dataset.rfMessage = '1';
    root.append(head, learnInfo, lastFrame, list, message);

    const update = () => {
      const data = state.rf433 || {};
      const rf = data.rf || {};
      const learn = data.learn || {};
      capacity.textContent = t('rf433.capacity').replace('{n}', String(Number(data.assignedRadio || 0)));
'''
new = '''    const head = el('div', 'rf433-head');
    const capacity = el('strong', 'rf433-capacity');
    const refreshButton = el('button', 'button'); refreshButton.type = 'button'; refreshButton.dataset.rfAction = 'refresh'; refreshButton.textContent = t('rf433.refresh');
    head.append(capacity, refreshButton);
    const modeRow = el('label', 'project-setting-row');
    const modeLabel = el('span', 'project-setting-label'); modeLabel.textContent = t('rf433.mode');
    const modeSelect = el('select', 'project-setting-input'); modeSelect.dataset.projectSetting = 'rfMode';
    for (const value of ['universal', 'somfy']) {
      const option = el('option'); option.value = value; option.textContent = t(`rf433.mode.${value}`); modeSelect.append(option);
    }
    modeRow.append(modeLabel, modeSelect);
    const modeHint = el('div', 'form-note'); modeHint.textContent = t('rf433.mode.hint');
    const learnInfo = el('div', 'rf433-learn-info');
    const lastFrame = el('div', 'form-note rf433-last-frame');
    const list = el('div', 'rf433-source-list');
    const message = el('div', 'project-setting-message'); message.setAttribute('aria-live', 'polite'); message.dataset.rfMessage = '1';
    root.append(head, modeRow, modeHint, learnInfo, lastFrame, list, message);

    const update = () => {
      const data = state.rf433 || {};
      const rf = data.rf || {};
      const learn = data.learn || {};
      const selectedMode = state.projectSettings?.rfMode || rf.mode || 'universal';
      if (document.activeElement !== modeSelect) modeSelect.value = selectedMode;
      modeSelect.disabled = !!learn.active || !rf.ready;
      capacity.textContent = t('rf433.capacity').replace('{n}', String(Number(data.assignedRadio || 0)));
'''
s = replace_once(s, old, new, "RF source mode selector")
s = replace_once(s,
'''      if (Number(rf.lastCode || 0) > 0) {
        lastFrame.textContent = `${t('rf433.lastFrame')}: 0x${Number(rf.lastCode).toString(16).toUpperCase().padStart(8, '0')} · ${Number(rf.lastBits || 0)} bit`;
      } else lastFrame.textContent = '';
''',
'''      if (Number(rf.lastCode || 0) > 0) {
        const protocol = rf.lastProtocol || 'universal';
        const codeText = `0x${Number(rf.lastCode).toString(16).toUpperCase().padStart(protocol === 'somfy' ? 6 : 8, '0')}`;
        lastFrame.textContent = `${t('rf433.lastFrame')}: ${t(`rf433.mode.${protocol}`)} · ${codeText}${protocol === 'somfy' ? '' : ` · ${Number(rf.lastBits || 0)} bit`}`;
      } else lastFrame.textContent = '';
''', "RF last frame mode")
s = replace_once(s,
'''        const bound = el('span'); bound.textContent = t(source.bound ? 'rf433.bound' : 'rf433.unbound');
        meta.append(id, bound);
''',
'''        const bound = el('span'); bound.textContent = t(source.bound ? 'rf433.bound' : 'rf433.unbound');
        const protocol = el('span'); protocol.textContent = `${t('rf433.protocol')}: ${t(`rf433.mode.${source.protocol || 'universal'}`)}`;
        meta.append(id, bound, protocol);
''', "RF source protocol label")
s = replace_once(s,
'''        if (data.summary) applyInterruptionSummary(data.summary);
        const message = document.querySelector('[data-project-setting-message]');
        if (message) { message.textContent = ''; message.dataset.state = 'ok'; }
        return true;
''',
'''        if (data.summary) applyInterruptionSummary(data.summary);
        if (field === 'rfMode') {
          await this.loadRfSources(true);
          if (state.activeView === 'device') await this.refreshHardwareState();
        }
        const message = document.querySelector(field === 'rfMode' ? '[data-rf-message]' : '[data-project-setting-message]');
        if (message) { message.textContent = ''; message.dataset.state = 'ok'; }
        return true;
''', "RF mode refresh after save")
s = replace_once(s,
'''        const message = document.querySelector('[data-project-setting-message]');
        if (message) { message.textContent = t('project.preferenceError'); message.dataset.state = 'error'; }
''',
'''        const message = document.querySelector(field === 'rfMode' ? '[data-rf-message]' : '[data-project-setting-message]');
        if (message) { message.textContent = t('project.preferenceError'); message.dataset.state = 'error'; }
''', "RF mode preference error")
write(name, s)

# ---------------------------------------------------------------------------
# Hardware card and release checks.
# ---------------------------------------------------------------------------
name = "tools/release_check.py"
s = read(name)
s = replace_once(s,
'''    check("RF433_SOMFY_FREQUENCY_HZ = 433420000UL" in hardware, "Somfy RTS 433.42 MHz test frequency")
    rf_driver = (ROOT / "rf433_cc1101.cpp").read_text(encoding="utf-8")
    check("Protocol::SomfyRts" in rf_driver and "decodeSomfyPayload" in rf_driver and "somfy_received" in rf_driver, "Somfy RTS receive-only diagnostic decoder")
    check("verifyFixedConfiguration" in rf_driver and "configVerified" in (ROOT / "rf433_cc1101.h").read_text(encoding="utf-8"), "CC1101 register readback verification")
''',
'''    check("RF433_SOMFY_FREQUENCY_HZ = 433420000UL" in hardware, "Somfy RTS 433.42 MHz frequency")
    rf_driver = (ROOT / "rf433_cc1101.cpp").read_text(encoding="utf-8")
    source_registry_cpp = (ROOT / "source_registry.cpp").read_text(encoding="utf-8")
    preferences_cpp = (ROOT / "project_preferences.cpp").read_text(encoding="utf-8")
    check("RF_MODE_DEFAULT = ProjectPreferences::RadioMode::Universal433" in project and '\"rfmode\"' in preferences_cpp, "persistent exclusive RF operating mode")
    check("Protocol::SomfyRts" in rf_driver and "decodeSomfyPayload" in rf_driver and "setOperatingProtocol" in rf_driver, "Somfy RTS normal receive path")
    check("RadioProtocol::SomfyRts" in source_registry_cpp and "uint8_t protocol = 0" in source_registry_cpp and "sizeof(StoredEntry) == 32" in source_registry_cpp, "RF protocol reuses reserved registry byte without growth")
    check("verifyConfiguration" in rf_driver and "configVerified" in (ROOT / "rf433_cc1101.h").read_text(encoding="utf-8"), "CC1101 selected-mode register readback verification")
''', "release RF mode checks")
s = replace_once(s,
'''    check("AUDIO_RX_PIN = 18" in hardware and "AUDIO_TX_PIN = 19" in hardware and "AUDIO_BUSY_PIN = 39" in hardware, "DY-SV17F pin map")
''',
'''    check("AUDIO_RX_PIN = 18" in hardware and "AUDIO_TX_PIN = 19" in hardware and "AUDIO_BUSY_PIN = 39" in hardware, "DY-SV17F pin map")
    audio_driver = (ROOT / "audio_dy_sv17f.cpp").read_text(encoding="utf-8")
    hardware_registry = (ROOT / "hardware_registry.cpp").read_text(encoding="utf-8")
    check("AUDIO_PROBE_MAX_ATTEMPTS = 2" in hardware and "RetryProbePlay" in audio_driver and "BUSY confirms active playback" in audio_driver, "DY-SV17F nonblocking query retry and BUSY fallback")
    check(hardware_registry.find("Rf433Cc1101::begin();") < hardware_registry.find("AudioDySv17f::begin();"), "CC1101 SPI initialized before final UART2 pin routing")
    check("rfMode" in JS and "'rf433.mode.somfy'" in JS and "'rf433.mode.universal'" in JS, "RF mode selector UI")
''', "release audio RF coexistence checks")
write(name, s)

# Add a concise hardware-test section without rewriting existing documentation.
name = "RF433_TEST.md"
s = read(name)
marker = "## Funkmodus Universal 433 / Somfy RTS"
if marker not in s:
    s += '''\n\n## Funkmodus Universal 433 / Somfy RTS\n\nUnter **Gerät → Hardware → Angelernte Funkbuttons** wird genau ein normaler Empfangsmodus gewählt:\n\n- **Universal 433**: 433,92 MHz, typische ASK/OOK-Festcode-Taster.\n- **Somfy RTS**: 433,42 MHz, klassische 56-Bit-RTS-Sender. Die stabile 24-Bit-Senderadresse ist die Bindungs-ID; Rolling Code und Befehl werden nicht als Identität gespeichert.\n\nBeim Umschalten bleiben bestehende Bindungen des jeweils anderen Protokolls erhalten. Anlernen und Unterbrechungserfassung verwenden ausschließlich den aktuell gewählten Modus. Der Hardware-Empfangstest testet ebenfalls nur diesen Modus und speichert sein Testtelegramm nicht als Unterbrechung.\n\n### DY-SV17F-Koexistenztest\n\nNach Einbau des CC1101 Audio mehrfach mit **Prüfen** und **Ton testen** testen. Der Audiotreiber wiederholt eine ausgebliebene Play-State-UART-Abfrage einmal nicht blockierend. Ist danach keine UART-Antwort vorhanden, BUSY aber aktiv, wird dies als Warnung mit bestätigter Wiedergabe statt fälschlich als vollständige Nichterreichbarkeit dargestellt. Ein inaktiver BUSY-Pin ersetzt keine UART-Antwort.\n'''
write(name, s)

print("RF mode + audio coexistence patch applied")
