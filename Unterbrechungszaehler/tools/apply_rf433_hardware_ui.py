#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return (ROOT / name).read_text(encoding="utf-8")


def write(name, text):
    (ROOT / name).write_text(text, encoding="utf-8")


def replace_once(name, old, new):
    text = read(name)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{name}: expected one occurrence, found {count}: {old[:80]!r}")
    write(name, text.replace(old, new, 1))


def replace_all(name, old, new, minimum=1):
    text = read(name)
    count = text.count(old)
    if count < minimum:
        raise RuntimeError(f"{name}: expected at least {minimum} occurrences, found {count}: {old[:80]!r}")
    write(name, text.replace(old, new))


def replace_block(name, start, end, new_block):
    text = read(name)
    i = text.find(start)
    if i < 0:
        raise RuntimeError(f"{name}: start marker not found: {start!r}")
    j = text.find(end, i)
    if j < 0:
        raise RuntimeError(f"{name}: end marker not found: {end!r}")
    write(name, text[:i] + new_block + text[j:])


# ---------------------------------------------------------------------------
# Wi-Fi: advertise the project-specific ASCII firmware name as STA hostname.
# ---------------------------------------------------------------------------
replace_once(
    "wifi_module.cpp",
    "bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {\n  return static_cast<uint32_t>(now - since) >= interval;\n}\n",
    "bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {\n  return static_cast<uint32_t>(now - since) >= interval;\n}\n\nbool applyProjectHostname() {\n  // DHCP hostnames must stay ASCII-safe. FIRMWARE_NAME is the network-safe\n  // project name (PROJECT_NAME may contain UTF-8 such as ä). Set it before\n  // WiFi.begin() so routers such as FRITZ!Box show a useful device name.\n  const bool ok = WiFi.setHostname(AppConfig::FIRMWARE_NAME);\n  if (ok) SerialLog::infof(\"WIFI\", \"Station hostname=%s\", AppConfig::FIRMWARE_NAME);\n  else SerialLog::warningf(\"WIFI\", \"Could not set station hostname=%s\", AppConfig::FIRMWARE_NAME);\n  return ok;\n}\n"
)
replace_once(
    "wifi_module.cpp",
    "  WiFi.mode(credentialsAvailable ? WIFI_AP_STA : WIFI_AP);\n  const bool started = WiFi.softAP(apName, AppConfig::FALLBACK_AP_PASSWORD);",
    "  WiFi.mode(credentialsAvailable ? WIFI_AP_STA : WIFI_AP);\n  if (credentialsAvailable) applyProjectHostname();\n  const bool started = WiFi.softAP(apName, AppConfig::FALLBACK_AP_PASSWORD);"
)
replace_once(
    "wifi_module.cpp",
    "  WiFi.mode(WIFI_STA);\n  const String currentIp = WiFi.localIP().toString();",
    "  WiFi.mode(WIFI_STA);\n  applyProjectHostname();\n  const String currentIp = WiFi.localIP().toString();"
)
replace_once(
    "wifi_module.cpp",
    "  WiFi.mode(WIFI_STA);\n  WiFi.setAutoReconnect(true);",
    "  WiFi.mode(WIFI_STA);\n  applyProjectHostname();\n  WiFi.setAutoReconnect(true);"
)

# ---------------------------------------------------------------------------
# RF driver: expose the same health/probe/test surface as other hardware.
# ---------------------------------------------------------------------------
replace_once(
    "rf433_cc1101.h",
    "#include <Arduino.h>\n",
    "#include <Arduino.h>\n\n#include \"hardware_types.h\"\n"
)
replace_once(
    "rf433_cc1101.h",
    "  uint8_t repeats = 0;\n};",
    "  uint8_t repeats = 0;\n  bool diagnostic = false;\n};"
)
replace_once(
    "rf433_cc1101.h",
    "bool begin();\nvoid update();\nbool pollFrame(Frame &frameOut);\nconst Info &info();",
    "bool begin();\nbool probe();\nvoid update();\nbool pollFrame(Frame &frameOut);\nconst Info &info();\n\nbool enabled();\nStatusRegistry::State health();\nuint32_t lastCheckMs();\nconst char *lastError();\nHardwareTypes::FeedbackType feedbackType();\n\nbool startReceiveTest();\nvoid cancelReceiveTest();\nbool receiveTestActive();\nconst char *receiveTestResult();\nuint32_t receiveTestRemainingMs();\nconst Frame &lastTestFrame();"
)

replace_once(
    "rf433_cc1101.cpp",
    "constexpr uint32_t PRESS_DEDUPE_MS = 550;\n",
    "constexpr uint32_t PRESS_DEDUPE_MS = 550;\nconstexpr uint32_t RECEIVE_TEST_MS = 5000;\n"
)
replace_once(
    "rf433_cc1101.cpp",
    "uint32_t lastEmitMs = 0;\n\nbool selectChip() {",
    "uint32_t lastEmitMs = 0;\nStatusRegistry::State currentHealth = StatusRegistry::State::Disabled;\nuint32_t checkedAtMs = 0;\nbool receiveTestActiveFlag = false;\nuint32_t receiveTestStartedMs = 0;\nconst char *receiveTestResultText = \"idle\";\nFrame receiveTestFrame;\n\nvoid setHealth(StatusRegistry::State state) {\n  currentHealth = state;\n  StatusRegistry::setState(\"rf433\", state);\n}\n\nvoid failReceiver(const char *error, StatusRegistry::State state) {\n  currentInfo.ready = false;\n  currentInfo.error = error ? error : \"rf433_error\";\n  checkedAtMs = millis();\n  setHealth(state);\n}\n\nbool selectChip() {"
)
replace_once(
    "rf433_cc1101.cpp",
    "  emittedFrame = candidate;\n  emittedFrame.repeats = pendingRepeats;\n  emittedAvailable = true;\n  currentInfo.lastFrame = emittedFrame;\n  ++currentInfo.decodedFrames;\n  lastEmitMs = nowMs;",
    "  const bool diagnostic = receiveTestActiveFlag;\n  if (diagnostic) {\n    receiveTestActiveFlag = false;\n    receiveTestResultText = \"received\";\n    receiveTestFrame = candidate;\n    checkedAtMs = nowMs;\n    setHealth(StatusRegistry::State::Ok);\n    SerialLog::successf(\"RF433\", \"Receive test passed | bits=%u | code=0x%08lX\",\n                        static_cast<unsigned int>(candidate.bitCount),\n                        static_cast<unsigned long>(candidate.code));\n  }\n\n  emittedFrame = candidate;\n  emittedFrame.repeats = pendingRepeats;\n  emittedFrame.diagnostic = diagnostic;\n  emittedAvailable = true;\n  currentInfo.lastFrame = emittedFrame;\n  ++currentInfo.decodedFrames;\n  lastEmitMs = nowMs;"
)

replace_block(
    "rf433_cc1101.cpp",
    "bool begin() {",
    "bool pollFrame(Frame &frameOut) {",
    r'''bool begin() {
  if (!enabled()) {
    currentHealth = StatusRegistry::State::Disabled;
    return false;
  }
  if (currentInfo.ready) return true;

  currentInfo = Info{};
  currentInfo.initialized = true;
  currentInfo.error = "initializing";
  checkedAtMs = millis();
  receiveTestActiveFlag = false;
  receiveTestResultText = "idle";
  StatusRegistry::registerProvider("rf433", "status.rf433", "hardware", true);
  setHealth(StatusRegistry::State::Checking);

  pinMode(HardwareConfig::RF433_CS_PIN, OUTPUT);
  digitalWrite(HardwareConfig::RF433_CS_PIN, HIGH);
  pinMode(HardwareConfig::RF433_GDO0_PIN, INPUT);
  pinMode(HardwareConfig::RF433_GDO2_PIN, INPUT);
  SPI.begin(HardwareConfig::RF433_SCK_PIN,
            HardwareConfig::RF433_MISO_PIN,
            HardwareConfig::RF433_MOSI_PIN,
            HardwareConfig::RF433_CS_PIN);
  delay(1);

  if (!strobe(SRES)) {
    failReceiver("cc1101_not_ready", StatusRegistry::State::NoResponse);
    SerialLog::error("RF433", "CC1101 reset failed (MISO never became ready)");
    return false;
  }
  delay(1);

  currentInfo.partNumber = readStatusRegister(PARTNUM);
  currentInfo.version = readStatusRegister(VERSION);
  if (currentInfo.partNumber == 0xFFU || currentInfo.version == 0xFFU) {
    failReceiver("cc1101_spi_read_failed", StatusRegistry::State::NoResponse);
    SerialLog::error("RF433", "CC1101 SPI status read failed");
    return false;
  }

  strobe(SIDLE);
  for (const RegisterSetting &setting : SETTINGS) {
    if (!writeRegister(setting.address, setting.value)) {
      failReceiver("cc1101_config_failed", StatusRegistry::State::Error);
      return false;
    }
  }
  strobe(SFRX);
  if (!strobe(SRX)) {
    failReceiver("cc1101_rx_failed", StatusRegistry::State::Error);
    return false;
  }

  lastEdgeUs = micros();
  attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN), onDataEdge, CHANGE);
  currentInfo.ready = true;
  currentInfo.error = "none";
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("RF433", "CC1101 ready | 433.92 MHz OOK async | part=0x%02X | version=0x%02X | GDO0=%d GDO2=%d",
                      currentInfo.partNumber, currentInfo.version,
                      HardwareConfig::RF433_GDO0_PIN, HardwareConfig::RF433_GDO2_PIN);
  return true;
}

bool probe() {
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

  currentInfo.partNumber = part;
  currentInfo.version = version;
  currentInfo.error = "none";
  checkedAtMs = millis();
  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("RF433", "Manual probe: OK | part=0x%02X | version=0x%02X", part, version);
  return true;
}

bool enabled() { return HardwareConfig::ENABLE_RF433_CC1101; }
StatusRegistry::State health() { return currentHealth; }
uint32_t lastCheckMs() { return checkedAtMs; }
const char *lastError() { return currentInfo.error; }
HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::ProtocolResponse; }

bool startReceiveTest() {
  if (!enabled() || !currentInfo.ready || receiveTestActiveFlag) return false;
  receiveTestActiveFlag = true;
  receiveTestStartedMs = millis();
  receiveTestResultText = "waiting";
  receiveTestFrame = Frame{};
  checkedAtMs = receiveTestStartedMs;
  setHealth(StatusRegistry::State::Checking);
  SerialLog::infof("RF433", "Receive test started | window=%lu ms | press any compatible 433 MHz button",
                   static_cast<unsigned long>(RECEIVE_TEST_MS));
  return true;
}

void cancelReceiveTest() {
  if (!receiveTestActiveFlag) return;
  receiveTestActiveFlag = false;
  receiveTestResultText = "cancelled";
  checkedAtMs = millis();
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
  processReadyFrame();
  if (receiveTestActiveFlag && static_cast<uint32_t>(millis() - receiveTestStartedMs) >= RECEIVE_TEST_MS) {
    receiveTestActiveFlag = false;
    receiveTestResultText = "timeout";
    checkedAtMs = millis();
    setHealth(StatusRegistry::State::Ok);
    SerialLog::warning("RF433", "Receive test finished without a valid fixed-code frame");
  }
}

'''
)

# Diagnostic receive frames must never become interruption events. Starting a
# new learn operation explicitly cancels an in-progress receive test.
replace_once(
    "rf433_service.cpp",
    "  while (Rf433Cc1101::pollFrame(frame)) {\n    uint8_t sourceId = SourceRegistry::SOURCE_ID_UNKNOWN;",
    "  while (Rf433Cc1101::pollFrame(frame)) {\n    if (frame.diagnostic) {\n      SerialLog::infof(\"RF433\", \"Diagnostic frame consumed without interruption | bits=%u | code=0x%08lX\",\n                       static_cast<unsigned int>(frame.bitCount), static_cast<unsigned long>(frame.code));\n      continue;\n    }\n    uint8_t sourceId = SourceRegistry::SOURCE_ID_UNKNOWN;"
)
replace_once(
    "rf433_service.cpp",
    "bool startLearn(const char *name, uint8_t targetSourceId) {\n  if (!ready()) return false;\n  return SourceRegistry::startLearn(name, targetSourceId);\n}",
    "bool startLearn(const char *name, uint8_t targetSourceId) {\n  if (!ready()) return false;\n  if (Rf433Cc1101::receiveTestActive()) Rf433Cc1101::cancelReceiveTest();\n  return SourceRegistry::startLearn(name, targetSourceId);\n}"
)

# ---------------------------------------------------------------------------
# HardwareRegistry integration: pin validation, status, checks and RX test.
# ---------------------------------------------------------------------------
replace_once(
    "hardware_registry.cpp",
    "#include \"rtc_ds3231.h\"\n#include \"serial_log.h\"",
    "#include \"rtc_ds3231.h\"\n#include \"rf433_cc1101.h\"\n#include \"source_registry.h\"\n#include \"serial_log.h\""
)
replace_once(
    "hardware_registry.cpp",
    "  if (HardwareConfig::ENABLE_AUDIO_DY_SV17F) {\n    add(HardwareConfig::AUDIO_RX_PIN, \"Audio RX\", false);\n    add(HardwareConfig::AUDIO_TX_PIN, \"Audio TX\", true);\n    add(HardwareConfig::AUDIO_BUSY_PIN, \"Audio BUSY\", false);\n  }",
    "  if (HardwareConfig::ENABLE_AUDIO_DY_SV17F) {\n    add(HardwareConfig::AUDIO_RX_PIN, \"Audio RX\", false);\n    add(HardwareConfig::AUDIO_TX_PIN, \"Audio TX\", true);\n    add(HardwareConfig::AUDIO_BUSY_PIN, \"Audio BUSY\", false);\n  }\n  if (HardwareConfig::ENABLE_RF433_CC1101) {\n    add(HardwareConfig::RF433_SCK_PIN, \"RF433 SCK\", true);\n    add(HardwareConfig::RF433_MISO_PIN, \"RF433 MISO\", false);\n    add(HardwareConfig::RF433_MOSI_PIN, \"RF433 MOSI\", true);\n    add(HardwareConfig::RF433_CS_PIN, \"RF433 CS\", true);\n    add(HardwareConfig::RF433_GDO0_PIN, \"RF433 GDO0\", false);\n    add(HardwareConfig::RF433_GDO2_PIN, \"RF433 GDO2\", false);\n  }"
)
replace_once(
    "hardware_registry.cpp",
    "  if (HardwareConfig::ENABLE_AUDIO_DY_SV17F) {\n    StatusRegistry::registerProvider(\"audio\", \"status.audio\", \"audio\", true);\n    StatusRegistry::setState(\"audio\", StatusRegistry::State::Error);\n  }\n}",
    "  if (HardwareConfig::ENABLE_AUDIO_DY_SV17F) {\n    StatusRegistry::registerProvider(\"audio\", \"status.audio\", \"audio\", true);\n    StatusRegistry::setState(\"audio\", StatusRegistry::State::Error);\n  }\n  if (HardwareConfig::ENABLE_RF433_CC1101) {\n    StatusRegistry::registerProvider(\"rf433\", \"status.rf433\", \"hardware\", true);\n    StatusRegistry::setState(\"rf433\", StatusRegistry::State::Error);\n  }\n}"
)
replace_once(
    "hardware_registry.cpp",
    "String hexByte(uint8_t value) {\n  char buffer[5];\n  snprintf(buffer, sizeof(buffer), \"0x%02X\", value);\n  return String(buffer);\n}\n",
    "String hexByte(uint8_t value) {\n  char buffer[5];\n  snprintf(buffer, sizeof(buffer), \"0x%02X\", value);\n  return String(buffer);\n}\n\nString hexDword(uint32_t value) {\n  char buffer[11];\n  snprintf(buffer, sizeof(buffer), \"0x%08lX\", static_cast<unsigned long>(value));\n  return String(buffer);\n}\n"
)
replace_once(
    "hardware_registry.cpp",
    "  DisplaySh1106::begin();\n  AudioDySv17f::begin();\n  SerialLog::info(\"HARDWARE\", \"Boot hardware checks dispatched | asynchronous modules finish in loop()\");",
    "  DisplaySh1106::begin();\n  AudioDySv17f::begin();\n  Rf433Cc1101::begin();\n  SerialLog::info(\"HARDWARE\", \"Boot hardware checks dispatched | asynchronous modules finish in loop()\");"
)
replace_once(
    "hardware_registry.cpp",
    "  DisplaySh1106::probe();\n  AudioDySv17f::probe();",
    "  DisplaySh1106::probe();\n  AudioDySv17f::probe();\n  Rf433Cc1101::probe();"
)
replace_once(
    "hardware_registry.cpp",
    "  if (std::strcmp(moduleId, \"audio\") == 0) return AudioDySv17f::enabled();\n  return false;",
    "  if (std::strcmp(moduleId, \"audio\") == 0) return AudioDySv17f::enabled();\n  if (std::strcmp(moduleId, \"rf433\") == 0) return Rf433Cc1101::enabled();\n  return false;"
)
replace_once(
    "hardware_registry.cpp",
    "  if (std::strcmp(moduleId, \"audio\") == 0 && AudioDySv17f::enabled()) return AudioDySv17f::probe();\n  return false;",
    "  if (std::strcmp(moduleId, \"audio\") == 0 && AudioDySv17f::enabled()) return AudioDySv17f::probe();\n  if (std::strcmp(moduleId, \"rf433\") == 0 && Rf433Cc1101::enabled()) return Rf433Cc1101::probe();\n  return false;"
)
replace_once(
    "hardware_registry.cpp",
    "  if (std::strcmp(moduleId, \"audio\") == 0 && std::strcmp(actionId, \"test\") == 0 && AudioDySv17f::enabled()) {\n    SerialLog::info(\"HARDWARE\", \"Manual action | audio test tone\");\n    return AudioDySv17f::playTestTone();\n  }\n  return false;",
    "  if (std::strcmp(moduleId, \"audio\") == 0 && std::strcmp(actionId, \"test\") == 0 && AudioDySv17f::enabled()) {\n    SerialLog::info(\"HARDWARE\", \"Manual action | audio test tone\");\n    return AudioDySv17f::playTestTone();\n  }\n  if (std::strcmp(moduleId, \"rf433\") == 0 && std::strcmp(actionId, \"test\") == 0 && Rf433Cc1101::enabled()) {\n    if (SourceRegistry::learnState().active) return false;\n    SerialLog::info(\"HARDWARE\", \"Manual action | RF433 receive test\");\n    return Rf433Cc1101::startReceiveTest();\n  }\n  return false;"
)
replace_once(
    "hardware_registry.cpp",
    "         DisplaySh1106::health() == StatusRegistry::State::Checking ||\n         AudioDySv17f::checking();",
    "         DisplaySh1106::health() == StatusRegistry::State::Checking ||\n         AudioDySv17f::checking() ||\n         Rf433Cc1101::health() == StatusRegistry::State::Checking ||\n         Rf433Cc1101::receiveTestActive();"
)
replace_once(
    "hardware_registry.cpp",
    "  out += ']';\n  out += '}';",
    "  if (Rf433Cc1101::enabled()) {\n    beginModule(out, firstModule, \"rf433\", \"hardware.rf433\", \"hardware\", true, effectiveHealth(Rf433Cc1101::health()),\n                Rf433Cc1101::feedbackType(), effectiveCheckedAt(Rf433Cc1101::lastCheckMs()), effectiveError(Rf433Cc1101::lastError()));\n    bool first = true;\n    const auto &rf = Rf433Cc1101::info();\n    appendInfoString(out, first, \"hardware.info.model\", \"CC1101 / RF1100SE\");\n    appendInfoString(out, first, \"hardware.info.transport\", \"SPI / 433.92 MHz OOK\");\n    appendInfoString(out, first, \"hardware.info.pins\",\n                     String(\"SCK GPIO\") + String(static_cast<int>(HardwareConfig::RF433_SCK_PIN)) +\n                     \", MISO GPIO\" + String(static_cast<int>(HardwareConfig::RF433_MISO_PIN)) +\n                     \", MOSI GPIO\" + String(static_cast<int>(HardwareConfig::RF433_MOSI_PIN)) +\n                     \", CS GPIO\" + String(static_cast<int>(HardwareConfig::RF433_CS_PIN)) +\n                     \", GDO0 GPIO\" + String(static_cast<int>(HardwareConfig::RF433_GDO0_PIN)) +\n                     \", GDO2 GPIO\" + String(static_cast<int>(HardwareConfig::RF433_GDO2_PIN)));\n    appendInfoUInt(out, first, \"hardware.info.frequency\", HardwareConfig::RF433_FREQUENCY_HZ);\n    appendInfoBool(out, first, \"hardware.info.initialized\", rf.initialized);\n    if (rf.partNumber != 0xFFU) appendInfoString(out, first, \"hardware.info.partNumber\", hexByte(rf.partNumber));\n    if (rf.version != 0xFFU) appendInfoString(out, first, \"hardware.info.chipVersion\", hexByte(rf.version));\n    appendInfoUInt(out, first, \"hardware.info.decodedFrames\", rf.decodedFrames);\n    appendInfoUInt(out, first, \"hardware.info.rejectedFrames\", rf.rejectedFrames);\n    appendInfoUInt(out, first, \"hardware.info.overflowFrames\", rf.overflowFrames);\n    appendInfoString(out, first, \"hardware.info.rfTestResult\", Rf433Cc1101::receiveTestResult(), \"rfTest\");\n    const auto &testFrame = Rf433Cc1101::lastTestFrame();\n    if (testFrame.code != 0U) {\n      appendInfoString(out, first, \"hardware.info.rfTestFrame\",\n                       hexDword(testFrame.code) + \" / \" + String(testFrame.bitCount) + \" bit\");\n    }\n    endModule(out, \"test\", \"action.rf433Test\", \"hardware\");\n  }\n\n  out += ']';\n  out += '}';"
)

# ---------------------------------------------------------------------------
# Keep the source API cheap in the UI: retained raw counts are opt-in only.
# ---------------------------------------------------------------------------
replace_once("rf433_api.h", "String buildSourcesJson();", "String buildSourcesJson(bool includeRetainedCounts = true);")
replace_once("rf433_api.cpp", "String buildSourcesJson() {", "String buildSourcesJson(bool includeRetainedCounts) {")
replace_once("rf433_api.cpp", "  if (rawReady) {", "  if (rawReady && includeRetainedCounts) {")
replace_once(
    "rf433_api.cpp",
    "  fieldUInt(out, \"boundRadio\", static_cast<uint32_t>(SourceRegistry::boundRadioCount()));\n",
    "  fieldUInt(out, \"boundRadio\", static_cast<uint32_t>(SourceRegistry::boundRadioCount()));\n  fieldBool(out, \"retainedCountsIncluded\", includeRetainedCounts);\n"
)
replace_all(
    "web_server.cpp",
    "sendJson(Rf433Api::buildSourcesJson());",
    "sendJson(Rf433Api::buildSourcesJson(server.arg(\"compact\") != \"1\"));",
    minimum=5
)

# ---------------------------------------------------------------------------
# UI: Home = learn + count only; Device/Hardware = receiver + button list.
# ---------------------------------------------------------------------------
app = read("ui-src/app.js")
app = app.replace("const order = { wifi: 10, time: 15, gpio: 20, rtc: 30, display: 40, audio: 50, data: 55 };",
                  "const order = { wifi: 10, time: 15, gpio: 20, rtc: 30, display: 40, rf433: 45, audio: 50, data: 55 };")
app = app.replace("    stateKey(value) { return t(`hardware.play.${value || 'unknown'}`); },",
                  "    stateKey(value) { return t(`hardware.play.${value || 'unknown'}`); },\n    rfTest(value) { return t(`rf433.test.${value || 'idle'}`); },")
app = app.replace(
    "          { id: 'rf433-sources', titleKey: 'card.rf433', descriptionKey: 'card.rf433.desc', icon: 'hardware', width: 'full', components: [\n            { type: 'rfSources' }\n          ] }",
    "          { id: 'rf433-learn', titleKey: 'card.rf433.learn', descriptionKey: 'card.rf433.learn.desc', icon: 'hardware', width: 'full', components: [\n            { type: 'rfLearn' }\n          ] }"
)
app = app.replace(
    "          { id: 'ota', titleKey: 'card.ota', descriptionKey: 'card.ota.desc', icon: 'upload', width: 'full', components: [",
    "          { id: 'rf433-sources', titleKey: 'card.rf433.sources', descriptionKey: 'card.rf433.sources.desc', icon: 'hardware', width: 'full', components: [\n            { type: 'rfSources' }\n          ] },\n          { id: 'ota', titleKey: 'card.ota', descriptionKey: 'card.ota.desc', icon: 'upload', width: 'full', components: [",
    1
)
write("ui-src/app.js", app)

RF_I18N = r'''  const RF433_I18N = {
    de: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Funk',
      'card.rf433.learn': 'Funkbutton anlernen', 'card.rf433.learn.desc': 'Hier wird nur angelernt. Status, Test und die Tasterverwaltung liegen unter Gerät / Hardware.',
      'card.rf433.sources': 'Angelernte Funkbuttons', 'card.rf433.sources.desc': 'Namen, stabile Source-IDs und Senderbindungen verwalten. Die Funkempfänger-Diagnose steht direkt in der Hardwareliste darüber.',
      'hardware.rf433': 'CC1101 Funkempfänger', 'hardware.info.frequency': 'Frequenz', 'hardware.info.partNumber': 'Chip Part', 'hardware.info.chipVersion': 'Chip-Version',
      'hardware.info.decodedFrames': 'Dekodierte Frames', 'hardware.info.rejectedFrames': 'Verworfene Frames', 'hardware.info.overflowFrames': 'Pufferüberläufe',
      'hardware.info.rfTestResult': 'Empfangstest', 'hardware.info.rfTestFrame': 'Test-Frame', 'action.rf433Test': 'Empfang testen (5 s)',
      'rf433.test.idle': 'Noch nicht getestet', 'rf433.test.waiting': 'Warte auf Funktelegramm …', 'rf433.test.received': 'Funktelegramm empfangen', 'rf433.test.timeout': 'Kein gültiges Telegramm empfangen', 'rf433.test.cancelled': 'Test abgebrochen',
      'rf433.ready': 'Empfänger bereit', 'rf433.offline': 'Empfänger nicht bereit', 'rf433.learning': 'Anlernen aktiv',
      'rf433.newName': 'Name des neuen Buttons', 'rf433.learnNew': 'Neuen Button anlernen', 'rf433.cancel': 'Abbrechen', 'rf433.refresh': 'Aktualisieren',
      'rf433.pressButton': 'Jetzt den gewünschten Funkbutton mehrfach drücken.', 'rf433.noSources': 'Noch kein Funkbutton angelernt.',
      'rf433.sourceId': 'Source-ID', 'rf433.retained': 'Rohereignisse im Ring', 'rf433.bound': 'Gebunden', 'rf433.unbound': 'Nicht gebunden',
      'rf433.rename': 'Umbenennen', 'rf433.replace': 'Sender ersetzen', 'rf433.unbind': 'Sender lösen', 'rf433.lastFrame': 'Letzter Funkcode',
      'rf433.error': 'Funkaktion fehlgeschlagen', 'rf433.capacity': '{n} von 10 Source-IDs belegt', 'rf433.learned': '{n} von 10 Tastern eingelernt'
    },
    en: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Radio',
      'card.rf433.learn': 'Learn radio button', 'card.rf433.learn.desc': 'Only learning happens here. Receiver status, testing and button management are under Device / Hardware.',
      'card.rf433.sources': 'Learned radio buttons', 'card.rf433.sources.desc': 'Manage names, stable source IDs and transmitter bindings. Receiver diagnostics are shown in the hardware list above.',
      'hardware.rf433': 'CC1101 radio receiver', 'hardware.info.frequency': 'Frequency', 'hardware.info.partNumber': 'Chip part', 'hardware.info.chipVersion': 'Chip version',
      'hardware.info.decodedFrames': 'Decoded frames', 'hardware.info.rejectedFrames': 'Rejected frames', 'hardware.info.overflowFrames': 'Buffer overflows',
      'hardware.info.rfTestResult': 'Receive test', 'hardware.info.rfTestFrame': 'Test frame', 'action.rf433Test': 'Test reception (5 s)',
      'rf433.test.idle': 'Not tested yet', 'rf433.test.waiting': 'Waiting for radio frame …', 'rf433.test.received': 'Radio frame received', 'rf433.test.timeout': 'No valid frame received', 'rf433.test.cancelled': 'Test cancelled',
      'rf433.ready': 'Receiver ready', 'rf433.offline': 'Receiver not ready', 'rf433.learning': 'Learn mode active',
      'rf433.newName': 'Name of new button', 'rf433.learnNew': 'Learn new button', 'rf433.cancel': 'Cancel', 'rf433.refresh': 'Refresh',
      'rf433.pressButton': 'Press the desired radio button several times now.', 'rf433.noSources': 'No radio button learned yet.',
      'rf433.sourceId': 'Source ID', 'rf433.retained': 'Raw events retained', 'rf433.bound': 'Bound', 'rf433.unbound': 'Not bound',
      'rf433.rename': 'Rename', 'rf433.replace': 'Replace transmitter', 'rf433.unbind': 'Unbind transmitter', 'rf433.lastFrame': 'Last radio code',
      'rf433.error': 'Radio action failed', 'rf433.capacity': '{n} of 10 source IDs assigned', 'rf433.learned': '{n} of 10 buttons learned'
    },
    it: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Radio',
      'card.rf433.learn': 'Apprendi pulsante radio', 'card.rf433.learn.desc': 'Qui avviene solo l’apprendimento. Stato, test e gestione dei pulsanti sono in Dispositivo / Hardware.',
      'card.rf433.sources': 'Pulsanti radio appresi', 'card.rf433.sources.desc': 'Gestione di nomi, ID sorgente stabili e associazioni. La diagnostica del ricevitore è nella lista hardware sopra.',
      'hardware.rf433': 'Ricevitore radio CC1101', 'hardware.info.frequency': 'Frequenza', 'hardware.info.partNumber': 'Parte chip', 'hardware.info.chipVersion': 'Versione chip',
      'hardware.info.decodedFrames': 'Frame decodificati', 'hardware.info.rejectedFrames': 'Frame scartati', 'hardware.info.overflowFrames': 'Overflow buffer',
      'hardware.info.rfTestResult': 'Test ricezione', 'hardware.info.rfTestFrame': 'Frame di test', 'action.rf433Test': 'Test ricezione (5 s)',
      'rf433.test.idle': 'Non ancora testato', 'rf433.test.waiting': 'In attesa di un frame radio …', 'rf433.test.received': 'Frame radio ricevuto', 'rf433.test.timeout': 'Nessun frame valido ricevuto', 'rf433.test.cancelled': 'Test annullato',
      'rf433.ready': 'Ricevitore pronto', 'rf433.offline': 'Ricevitore non pronto', 'rf433.learning': 'Apprendimento attivo',
      'rf433.newName': 'Nome del nuovo pulsante', 'rf433.learnNew': 'Apprendi nuovo pulsante', 'rf433.cancel': 'Annulla', 'rf433.refresh': 'Aggiorna',
      'rf433.pressButton': 'Premere ora più volte il pulsante radio desiderato.', 'rf433.noSources': 'Nessun pulsante radio appreso.',
      'rf433.sourceId': 'ID sorgente', 'rf433.retained': 'Eventi grezzi nel ring', 'rf433.bound': 'Associato', 'rf433.unbound': 'Non associato',
      'rf433.rename': 'Rinomina', 'rf433.replace': 'Sostituisci trasmettitore', 'rf433.unbind': 'Scollega trasmettitore', 'rf433.lastFrame': 'Ultimo codice radio',
      'rf433.error': 'Azione radio non riuscita', 'rf433.capacity': '{n} di 10 ID sorgente assegnati', 'rf433.learned': '{n} di 10 pulsanti appresi'
    },
    fr: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Radio',
      'card.rf433.learn': 'Apprendre un bouton radio', 'card.rf433.learn.desc': 'Seul l’apprentissage se fait ici. État, test et gestion des boutons sont sous Appareil / Matériel.',
      'card.rf433.sources': 'Boutons radio appris', 'card.rf433.sources.desc': 'Gérer les noms, ID source stables et liaisons émetteur. Le diagnostic du récepteur est dans la liste matérielle ci-dessus.',
      'hardware.rf433': 'Récepteur radio CC1101', 'hardware.info.frequency': 'Fréquence', 'hardware.info.partNumber': 'Référence puce', 'hardware.info.chipVersion': 'Version puce',
      'hardware.info.decodedFrames': 'Trames décodées', 'hardware.info.rejectedFrames': 'Trames rejetées', 'hardware.info.overflowFrames': 'Débordements tampon',
      'hardware.info.rfTestResult': 'Test réception', 'hardware.info.rfTestFrame': 'Trame de test', 'action.rf433Test': 'Tester réception (5 s)',
      'rf433.test.idle': 'Pas encore testé', 'rf433.test.waiting': 'Attente d’une trame radio …', 'rf433.test.received': 'Trame radio reçue', 'rf433.test.timeout': 'Aucune trame valide reçue', 'rf433.test.cancelled': 'Test annulé',
      'rf433.ready': 'Récepteur prêt', 'rf433.offline': 'Récepteur indisponible', 'rf433.learning': 'Apprentissage actif',
      'rf433.newName': 'Nom du nouveau bouton', 'rf433.learnNew': 'Apprendre un bouton', 'rf433.cancel': 'Annuler', 'rf433.refresh': 'Actualiser',
      'rf433.pressButton': 'Appuyer maintenant plusieurs fois sur le bouton radio souhaité.', 'rf433.noSources': 'Aucun bouton radio appris.',
      'rf433.sourceId': 'ID source', 'rf433.retained': 'Événements bruts conservés', 'rf433.bound': 'Associé', 'rf433.unbound': 'Non associé',
      'rf433.rename': 'Renommer', 'rf433.replace': 'Remplacer émetteur', 'rf433.unbind': 'Dissocier émetteur', 'rf433.lastFrame': 'Dernier code radio',
      'rf433.error': 'Action radio échouée', 'rf433.capacity': '{n} sur 10 ID source attribués', 'rf433.learned': '{n} sur 10 boutons appris'
    },
    swg: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Funk',
      'card.rf433.learn': 'Funkknopf anlerna', 'card.rf433.learn.desc': 'Do wird bloß anglernt. Status, Test ond Knopfverwaltung send bei Grät / Hardware.',
      'card.rf433.sources': 'Anglernte Funkknöpf', 'card.rf433.sources.desc': 'Nama, stabile Source-IDs ond Senderbindunga verwalta. D Empfängerdiagnose steht oba in dr Hardwarelist.',
      'hardware.rf433': 'CC1101 Funkempfänger', 'hardware.info.frequency': 'Frequenz', 'hardware.info.partNumber': 'Chip-Part', 'hardware.info.chipVersion': 'Chip-Version',
      'hardware.info.decodedFrames': 'Dekodierte Frames', 'hardware.info.rejectedFrames': 'Verworfene Frames', 'hardware.info.overflowFrames': 'Pufferüberläuf',
      'hardware.info.rfTestResult': 'Empfangstest', 'hardware.info.rfTestFrame': 'Test-Frame', 'action.rf433Test': 'Empfang testa (5 s)',
      'rf433.test.idle': 'No net testet', 'rf433.test.waiting': 'Wart auf a Funktelegramm …', 'rf433.test.received': 'Funktelegramm empfangen', 'rf433.test.timeout': 'Koi gültigs Telegramm empfangen', 'rf433.test.cancelled': 'Test abbrocha',
      'rf433.ready': 'Empfänger bereit', 'rf433.offline': 'Empfänger net bereit', 'rf433.learning': 'Anlerna läuft',
      'rf433.newName': 'Name vom neia Knopf', 'rf433.learnNew': 'Neia Knopf anlerna', 'rf433.cancel': 'Abbrecha', 'rf433.refresh': 'Neu lada',
      'rf433.pressButton': 'Jetzt dr gewünschte Funkknopf mehrafach drucka.', 'rf433.noSources': 'No koi Funkknopf anglernt.',
      'rf433.sourceId': 'Source-ID', 'rf433.retained': 'Rohereignis em Ring', 'rf433.bound': 'Gebunda', 'rf433.unbound': 'Net gebunda',
      'rf433.rename': 'Umbenenna', 'rf433.replace': 'Sender ersetza', 'rf433.unbind': 'Sender lösa', 'rf433.lastFrame': 'Letschter Funkcode',
      'rf433.error': 'Funkaktion isch schief ganga', 'rf433.capacity': '{n} vo 10 Source-IDs vergebba', 'rf433.learned': '{n} vo 10 Knöpf anglernt'
    }
  };
'''
replace_block("ui-src/app.js", "  const RF433_I18N = {", "  Object.assign(I18N.de, RF433_I18N.de);", RF_I18N)

RF_RENDER = r'''  function renderRfLearn() {
    const root = el('div', 'rf433-panel');
    const capacity = el('strong', 'rf433-capacity');
    const learnInfo = el('div', 'rf433-learn-info');
    const toolbar = el('div', 'rf433-toolbar');
    const nameInput = el('input', 'project-setting-input');
    nameInput.type = 'text'; nameInput.maxLength = 23; nameInput.placeholder = t('rf433.newName'); nameInput.dataset.rfNewName = '1';
    const learnButton = el('button', 'button'); learnButton.type = 'button'; learnButton.dataset.rfAction = 'learn-new'; learnButton.textContent = t('rf433.learnNew');
    const cancelButton = el('button', 'button'); cancelButton.type = 'button'; cancelButton.dataset.rfAction = 'cancel'; cancelButton.textContent = t('rf433.cancel');
    toolbar.append(nameInput, learnButton, cancelButton);
    const message = el('div', 'project-setting-message'); message.setAttribute('aria-live', 'polite'); message.dataset.rfMessage = '1';
    root.append(capacity, learnInfo, toolbar, message);

    const update = () => {
      const data = state.rf433 || {};
      const rf = data.rf || {};
      const learn = data.learn || {};
      capacity.textContent = t('rf433.learned').replace('{n}', String(Number(data.boundRadio || 0)));
      learnInfo.textContent = learn.active
        ? `${t('rf433.learning')} · ${t('rf433.pressButton')} (${Math.ceil(Number(learn.remainingMs || 0) / 1000)} s)`
        : '';
      cancelButton.hidden = !learn.active;
      learnButton.disabled = !rf.ready || !!learn.active || Number(data.assignedRadio || 0) >= Number(data.radioCapacity || 10);
      nameInput.disabled = !rf.ready || !!learn.active;
    };
    Bindings.add('rf433', update);
    queueMicrotask(() => Transport.loadRfSources(true));
    return root;
  }

  function renderRfSourceList() {
    const root = el('div', 'rf433-panel');
    const head = el('div', 'rf433-head');
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
      learnInfo.textContent = learn.active
        ? `${t('rf433.learning')} · ${t('rf433.pressButton')} (${Math.ceil(Number(learn.remainingMs || 0) / 1000)} s)`
        : '';
      if (Number(rf.lastCode || 0) > 0) {
        lastFrame.textContent = `${t('rf433.lastFrame')}: 0x${Number(rf.lastCode).toString(16).toUpperCase().padStart(8, '0')} · ${Number(rf.lastBits || 0)} bit`;
      } else lastFrame.textContent = '';

      list.replaceChildren();
      const sources = Array.isArray(data.sources) ? data.sources.filter(source => source.kind === 'radio') : [];
      if (!sources.length) {
        const empty = el('div', 'form-note'); empty.textContent = t('rf433.noSources'); list.append(empty); return;
      }
      for (const source of sources) {
        const row = el('article', 'rf433-source-row'); row.dataset.rfSourceId = String(source.id);
        const meta = el('div', 'rf433-source-meta');
        const id = el('strong'); id.textContent = `${t('rf433.sourceId')} ${source.id}`;
        const bound = el('span'); bound.textContent = t(source.bound ? 'rf433.bound' : 'rf433.unbound');
        meta.append(id, bound);
        const edit = el('div', 'rf433-source-edit');
        const input = el('input', 'project-setting-input'); input.type = 'text'; input.maxLength = 23; input.value = source.name || ''; input.dataset.rfSourceName = String(source.id);
        const rename = el('button', 'button'); rename.type = 'button'; rename.dataset.rfAction = 'rename'; rename.textContent = t('rf433.rename');
        const replace = el('button', 'button'); replace.type = 'button'; replace.dataset.rfAction = 'replace'; replace.textContent = t('rf433.replace');
        const unbind = el('button', 'button'); unbind.type = 'button'; unbind.dataset.rfAction = 'unbind'; unbind.textContent = t('rf433.unbind'); unbind.disabled = !source.bound;
        edit.append(input, rename, replace, unbind);
        row.append(meta, edit); list.append(row);
      }
    };
    Bindings.add('rf433', update);
    queueMicrotask(() => Transport.loadRfSources(true));
    return root;
  }

'''
replace_block("ui-src/app.js", "  function renderRfSources() {", "  function localeForLabels()", RF_RENDER)

replace_once(
    "ui-src/app.js",
    "projectSettings: renderProjectSettings, rfSources: renderRfSources, heatmapHourly",
    "projectSettings: renderProjectSettings, rfLearn: renderRfLearn, rfSources: renderRfSourceList, heatmapHourly"
)
replace_once(
    "ui-src/app.js",
    "    async loadRfSources() {\n      if (this._rfBusy) return;\n      this._rfBusy = true;\n      try {\n        const data = await this.requestResult('/api/interruptions/sources');",
    "    async loadRfSources(compact = true) {\n      if (this._rfBusy) return;\n      this._rfBusy = true;\n      try {\n        const data = await this.requestResult(`/api/interruptions/sources${compact ? '?compact=1' : ''}`);"
)
replace_once(
    "ui-src/app.js",
    "      const parts = [`name=${encodeURIComponent(String(name || '').trim())}`];",
    "      const parts = [`name=${encodeURIComponent(String(name || '').trim())}`, 'compact=1'];"
)
replace_once(
    "ui-src/app.js",
    "      try { const data = await this.requestResult('/api/interruptions/rf/cancel', { method: 'POST' }); patchState({ rf433: data }); Bindings.notify('rf433'); }",
    "      try { const data = await this.requestResult('/api/interruptions/rf/cancel?compact=1', { method: 'POST' }); patchState({ rf433: data }); Bindings.notify('rf433'); }"
)
replace_once(
    "ui-src/app.js",
    "        const data = await this.requestResult(`/api/interruptions/sources/rename?sourceId=${encodeURIComponent(sourceId)}&name=${encodeURIComponent(String(name || '').trim())}`, { method: 'POST' });",
    "        const data = await this.requestResult(`/api/interruptions/sources/rename?sourceId=${encodeURIComponent(sourceId)}&name=${encodeURIComponent(String(name || '').trim())}&compact=1`, { method: 'POST' });"
)
replace_once(
    "ui-src/app.js",
    "        const data = await this.requestResult(`/api/interruptions/sources/unbind?sourceId=${encodeURIComponent(sourceId)}`, { method: 'POST' });",
    "        const data = await this.requestResult(`/api/interruptions/sources/unbind?sourceId=${encodeURIComponent(sourceId)}&compact=1`, { method: 'POST' });"
)
replace_once(
    "ui-src/app.js",
    "      if (state.activeView === 'home' && state.rf433?.learn?.active && (!this._lastRfTick || now - this._lastRfTick >= 1000)) {\n        this._lastRfTick = now; this.loadRfSources();\n      }",
    "      if (['home', 'device'].includes(state.activeView) && state.rf433?.learn?.active && (!this._lastRfTick || now - this._lastRfTick >= 1000)) {\n        this._lastRfTick = now; this.loadRfSources(true);\n      }"
)
replace_once(
    "ui-src/app.js",
    "        if (data.hardware?.checking && attempt < 4) this.followHardwareCheck(attempt + 1);",
    "        if (data.hardware?.checking && attempt < 24) this.followHardwareCheck(attempt + 1);"
)

# ---------------------------------------------------------------------------
# Tests/docs: assert the new architecture and describe the moved UI.
# ---------------------------------------------------------------------------
replace_once(
    "tools/release_check.py",
    "    check(\"rfSources: renderRfSources\" in JS and \"card.rf433\" in JS, \"RF source manager in Home UI\")",
    "    check(\"rfLearn: renderRfLearn\" in JS and \"rfSources: renderRfSourceList\" in JS, \"RF learning on Home and source manager on Device\")\n    check(\"Rf433Cc1101::startReceiveTest\" in (ROOT / \"hardware_registry.cpp\").read_text(encoding=\"utf-8\") and '\"rf433\"' in (ROOT / \"hardware_registry.cpp\").read_text(encoding=\"utf-8\"), \"RF receiver integrated into HardwareRegistry with test action\")\n    check(\"WiFi.setHostname(AppConfig::FIRMWARE_NAME)\" in (ROOT / \"wifi_module.cpp\").read_text(encoding=\"utf-8\"), \"project Wi-Fi hostname\")\n    check(\"includeRetainedCounts\" in (ROOT / \"rf433_api.cpp\").read_text(encoding=\"utf-8\") and \"compact=1\" in JS, \"compact RF configuration API avoids raw-ring scans\")"
)

replace_once(
    "RF433_TEST.md",
    "2. Weboberfläche → **Home → Funkbuttons / 433 MHz**.\n3. Namen eingeben, z. B. `Anna`.",
    "2. Weboberfläche → **Home → Funkbutton anlernen**. Dort stehen absichtlich nur Anlernen und die Anzahl der aktuell gebundenen Taster.\n3. Namen eingeben, z. B. `Anna`."
)
replace_once(
    "RF433_TEST.md",
    "## Sender ersetzen ohne Historie umzubenennen\n\nBei einem defekten Sender in derselben Quellenzeile **Sender ersetzen** wählen und den neuen Sender drücken.",
    "## Hardwarestatus und Empfangstest\n\nUnter **Gerät → Hardware** erscheint der CC1101 wie RTC, Display und Sound als normales Hardwaremodul. Dort werden Status, SPI-/Chipdaten, Pins sowie Frame-Zähler angezeigt. **Hardware prüfen** fragt den CC1101 über SPI erneut ab. **Empfang testen (5 s)** wartet auf ein gültiges Funktelegramm; ein dabei empfangenes Testtelegramm wird absichtlich nicht als Unterbrechung gespeichert.\n\nDie vollständige Liste der angelernten Funkbuttons befindet sich ebenfalls unter **Gerät → Hardware**. Dort werden Namen geändert, Sender gelöst oder ersetzt.\n\n## Sender ersetzen ohne Historie umzubenennen\n\nBei einem defekten Sender in derselben Quellenzeile **Sender ersetzen** wählen und den neuen Sender drücken."
)
replace_once(
    "RF433_TEST.md",
    "- Bootlog: `CC1101 ready ...`\n- Headerstatus `433 MHz` sollte OK sein.",
    "- Bootlog: `CC1101 ready ...`\n- Headerstatus `433 MHz` sollte OK sein.\n- Unter Gerät / Hardware muss der CC1101 mit denselben Status-/Prüfmechanismen wie die übrige Hardware erscheinen.\n- Der 5-s-Empfangstest muss bei einem passenden Tastendruck ein Test-Frame melden, ohne den Unterbrechungszähler zu erhöhen.\n- Im Heimnetz meldet sich die WLAN-Station als `Unterbrechungszaehler` statt als generischer ESP32-Hostname."
)

print("RF433 hardware/UI refinement applied")
