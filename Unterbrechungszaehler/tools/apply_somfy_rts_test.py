#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(text, old, new, label):
    if old not in text:
        raise SystemExit(f"missing patch anchor: {label}")
    return text.replace(old, new, 1)

# Hardware configuration: keep normal fixed-code frequency and add Somfy RTS test frequency.
hardware = read("hardware_config.h")
hardware = replace_once(
    hardware,
    "constexpr uint32_t RF433_FREQUENCY_HZ = 433920000UL;",
    "constexpr uint32_t RF433_FREQUENCY_HZ = 433920000UL;\nconstexpr uint32_t RF433_SOMFY_FREQUENCY_HZ = 433420000UL;  // Somfy RTS receive test",
    "Somfy frequency",
)
write("hardware_config.h", hardware)

# Extend the compact receiver frame only with diagnostic protocol metadata.
header = r'''#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace Rf433Cc1101 {

enum class Protocol : uint8_t {
  FixedOok = 0,
  SomfyRts = 1
};

struct Frame {
  uint32_t code = 0;       // fixed code or stable Somfy remote address
  uint16_t rollingCode = 0;
  uint8_t bitCount = 0;
  uint8_t pulseBucket = 0;  // average short pulse in ~25 us units for fixed code
  uint8_t repeats = 0;
  uint8_t command = 0;      // Somfy command nibble when protocol == SomfyRts
  Protocol protocol = Protocol::FixedOok;
  bool diagnostic = false;
};

struct Info {
  bool initialized = false;
  bool ready = false;
  bool configVerified = false;
  uint8_t partNumber = 0xFF;
  uint8_t version = 0xFF;
  uint32_t activeFrequencyHz = 0;
  uint32_t decodedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint32_t overflowFrames = 0;
  Frame lastFrame{};
  const char *error = "none";
};

bool begin();
bool probe();
void update();
bool pollFrame(Frame &frameOut);
const Info &info();

bool enabled();
StatusRegistry::State health();
uint32_t lastCheckMs();
const char *lastError();
HardwareTypes::FeedbackType feedbackType();

bool startReceiveTest();
void cancelReceiveTest();
bool receiveTestActive();
const char *receiveTestResult();
uint32_t receiveTestRemainingMs();
const Frame &lastTestFrame();

const char *protocolName(Protocol protocol);
const char *somfyCommandName(uint8_t command);

}  // namespace Rf433Cc1101
'''
write("rf433_cc1101.h", header)

cpp = read("rf433_cc1101.cpp")
cpp = replace_once(
    cpp,
    "constexpr uint32_t RECEIVE_TEST_MS = 5000;",
    "constexpr uint32_t RECEIVE_TEST_MS = 10000;\nconstexpr uint32_t RECEIVE_TEST_SWITCH_MS = 1500;\nconstexpr uint32_t SOMFY_HALF_MIN_US = 448;\nconstexpr uint32_t SOMFY_HALF_MAX_US = 832;\nconstexpr uint32_t SOMFY_SYMBOL_MIN_US = 896;\nconstexpr uint32_t SOMFY_SYMBOL_MAX_US = 1664;\nconstexpr uint32_t SOMFY_HW_SYNC_MIN_US = 1792;\nconstexpr uint32_t SOMFY_HW_SYNC_MAX_US = 3328;\nconstexpr uint32_t SOMFY_SW_SYNC_MIN_US = 3395;\nconstexpr uint32_t SOMFY_SW_SYNC_MAX_US = 6305;\nconstexpr uint32_t SOMFY_GLITCH_MIN_US = 300;",
    "test constants",
)
cpp = replace_once(
    cpp,
    "Frame receiveTestFrame;",
    '''Frame receiveTestFrame;

enum class CaptureMode : uint8_t { FixedOok = 0, SomfyRts = 1 };
volatile CaptureMode captureMode = CaptureMode::FixedOok;
bool receiveTestSomfyPhase = false;
uint32_t receiveTestPhaseStartedMs = 0;

volatile bool somfyReceiving = false;
volatile bool somfyWaitingHalf = false;
volatile bool somfyReady = false;
volatile uint8_t somfySyncCount = 0;
volatile uint8_t somfyBitCount = 0;
volatile uint8_t somfyPreviousBit = 0;
volatile uint8_t somfyPayload[7]{};
volatile uint8_t somfyReadyPayload[7]{};
volatile uint8_t somfyReadySyncCount = 0;''',
    "Somfy state",
)
cpp = replace_once(
    cpp,
    '''uint8_t readStatusRegister(uint8_t address) {
  if (!selectChip()) return 0xFF;
  SPI.transfer(static_cast<uint8_t>(address | READ_STATUS));
  const uint8_t value = SPI.transfer(0);
  deselectChip();
  return value;
}
''',
    '''uint8_t readStatusRegister(uint8_t address) {
  if (!selectChip()) return 0xFF;
  SPI.transfer(static_cast<uint8_t>(address | READ_STATUS));
  const uint8_t value = SPI.transfer(0);
  deselectChip();
  return value;
}

uint8_t readConfigRegister(uint8_t address) {
  if (!selectChip()) return 0xFF;
  SPI.transfer(static_cast<uint8_t>(address | READ_SINGLE));
  const uint8_t value = SPI.transfer(0);
  deselectChip();
  return value;
}

bool verifyFixedConfiguration() {
  return readConfigRegister(0x02) == 0x0D &&
         readConfigRegister(0x08) == 0x32 &&
         readConfigRegister(0x0D) == 0x10 &&
         readConfigRegister(0x0E) == 0xB0 &&
         readConfigRegister(0x0F) == 0x71 &&
         readConfigRegister(0x12) == 0x30;
}

void IRAM_ATTR resetSomfyCapture(bool clearReady) {
  somfyReceiving = false;
  somfyWaitingHalf = false;
  somfySyncCount = 0;
  somfyBitCount = 0;
  somfyPreviousBit = 0;
  for (uint8_t i = 0; i < 7; ++i) somfyPayload[i] = 0;
  if (clearReady) {
    somfyReady = false;
    somfyReadySyncCount = 0;
    for (uint8_t i = 0; i < 7; ++i) somfyReadyPayload[i] = 0;
  }
}

void IRAM_ATTR appendSomfyBit() {
  if (somfyBitCount >= 56U || somfyReady) return;
  if (somfyPreviousBit) {
    somfyPayload[somfyBitCount / 8U] |= static_cast<uint8_t>(1U << (7U - (somfyBitCount % 8U)));
  }
  ++somfyBitCount;
  if (somfyBitCount < 56U) return;

  for (uint8_t i = 0; i < 7; ++i) somfyReadyPayload[i] = somfyPayload[i];
  somfyReadySyncCount = somfySyncCount;
  somfyReady = true;
  somfyReceiving = false;
  somfyWaitingHalf = false;
  somfySyncCount = 0;
  somfyBitCount = 0;
  somfyPreviousBit = 0;
  for (uint8_t i = 0; i < 7; ++i) somfyPayload[i] = 0;
}

void IRAM_ATTR feedSomfyDuration(uint32_t duration) {
  if (somfyReady) return;

  if (!somfyReceiving) {
    if (duration >= SOMFY_HW_SYNC_MIN_US && duration <= SOMFY_HW_SYNC_MAX_US) {
      if (somfySyncCount < 31U) ++somfySyncCount;
      return;
    }
    if (duration >= SOMFY_SW_SYNC_MIN_US && duration <= SOMFY_SW_SYNC_MAX_US && somfySyncCount >= 4U) {
      somfyReceiving = true;
      somfyWaitingHalf = false;
      somfyBitCount = 0;
      somfyPreviousBit = 0;
      for (uint8_t i = 0; i < 7; ++i) somfyPayload[i] = 0;
      return;
    }
    somfySyncCount = 0;
    return;
  }

  if (duration >= SOMFY_SYMBOL_MIN_US && duration <= SOMFY_SYMBOL_MAX_US && !somfyWaitingHalf) {
    somfyPreviousBit ^= 1U;
    appendSomfyBit();
    return;
  }
  if (duration >= SOMFY_HALF_MIN_US && duration <= SOMFY_HALF_MAX_US) {
    if (somfyWaitingHalf) {
      somfyWaitingHalf = false;
      appendSomfyBit();
    } else {
      somfyWaitingHalf = true;
    }
    return;
  }

  resetSomfyCapture(false);
}
''',
    "register read and Somfy decoder",
)
old_isr = '''void IRAM_ATTR onDataEdge() {
  const uint32_t nowUs = micros();
  const uint32_t duration = static_cast<uint32_t>(nowUs - lastEdgeUs);
  lastEdgeUs = nowUs;

  if (duration > FRAME_GAP_US) {
    finalizeFrameFromIsr();
    return;
  }
  if (duration < 70U || duration > 4500U) return;

  const bool carrier = gpio_get_level(static_cast<gpio_num_t>(HardwareConfig::RF433_GDO2_PIN)) != 0;
  if (!carrier && writeCount == 0) return;

  if (writeCount >= PULSE_BUFFER_SIZE) {
    ++isrOverflowFrames;
    writeCount = 0;
    return;
  }
  pulseBuffers[writeBuffer][writeCount++] = static_cast<uint16_t>(duration);
}
'''
new_isr = '''void IRAM_ATTR onDataEdge() {
  const uint32_t nowUs = micros();
  const uint32_t duration = static_cast<uint32_t>(nowUs - lastEdgeUs);

  if (captureMode == CaptureMode::SomfyRts) {
    // RTS decoding uses the edge-to-edge Manchester timing. Ignore tiny glitches
    // without moving the reference edge, matching the protocol's timing model.
    if (duration < SOMFY_GLITCH_MIN_US) return;
    lastEdgeUs = nowUs;
    feedSomfyDuration(duration);
    return;
  }

  lastEdgeUs = nowUs;
  if (duration > FRAME_GAP_US) {
    finalizeFrameFromIsr();
    return;
  }
  if (duration < 70U || duration > 4500U) return;

  const bool carrier = gpio_get_level(static_cast<gpio_num_t>(HardwareConfig::RF433_GDO2_PIN)) != 0;
  if (!carrier && writeCount == 0) return;

  if (writeCount >= PULSE_BUFFER_SIZE) {
    ++isrOverflowFrames;
    writeCount = 0;
    return;
  }
  pulseBuffers[writeBuffer][writeCount++] = static_cast<uint16_t>(duration);
}

bool applyCaptureMode(CaptureMode mode) {
  detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));
  if (!strobe(SIDLE)) return false;

  // 26 MHz crystal: 433.92 MHz = 0x10B071, Somfy RTS 433.42 MHz = 0x10AB85.
  const bool somfy = mode == CaptureMode::SomfyRts;
  if (!writeRegister(0x0D, 0x10) ||
      !writeRegister(0x0E, somfy ? 0xAB : 0xB0) ||
      !writeRegister(0x0F, somfy ? 0x85 : 0x71)) {
    return false;
  }
  strobe(SFRX);

  noInterrupts();
  captureMode = mode;
  writeCount = 0;
  readyFrame = false;
  readyCount = 0;
  resetSomfyCapture(true);
  lastEdgeUs = micros();
  interrupts();

  if (!strobe(SRX)) return false;
  currentInfo.activeFrequencyHz = somfy ? HardwareConfig::RF433_SOMFY_FREQUENCY_HZ : HardwareConfig::RF433_FREQUENCY_HZ;
  attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN), onDataEdge, CHANGE);
  return true;
}
'''
cpp = replace_once(cpp, old_isr, new_isr, "ISR and retune")
cpp = replace_once(
    cpp,
    "  out.code = bestCode;\n  out.bitCount = bestBits;",
    "  out.protocol = Protocol::FixedOok;\n  out.code = bestCode;\n  out.bitCount = bestBits;",
    "fixed protocol marker",
)
cpp = replace_once(
    cpp,
    "void processCandidate(const Frame &candidate) {",
    '''bool decodeSomfyPayload(const uint8_t encoded[7], Frame &out) {
  uint8_t decoded[7]{};
  decoded[0] = encoded[0];
  for (uint8_t i = 1; i < 7; ++i) decoded[i] = static_cast<uint8_t>(encoded[i] ^ encoded[i - 1U]);

  uint8_t checksum = 0;
  for (uint8_t i = 0; i < 7; ++i) checksum ^= static_cast<uint8_t>(decoded[i] ^ (decoded[i] >> 4U));
  if ((checksum & 0x0FU) != 0U || decoded[0] == 0U) return false;

  const uint32_t address = (static_cast<uint32_t>(decoded[4]) << 16U) |
                           (static_cast<uint32_t>(decoded[5]) << 8U) |
                           static_cast<uint32_t>(decoded[6]);
  if (address == 0U || address == 0xFFFFFFUL) return false;

  out = Frame{};
  out.protocol = Protocol::SomfyRts;
  out.code = address;
  out.rollingCode = static_cast<uint16_t>((static_cast<uint16_t>(decoded[2]) << 8U) | decoded[3]);
  out.command = static_cast<uint8_t>(decoded[1] >> 4U);
  out.bitCount = 56;
  out.repeats = 1;
  return true;
}

void processSomfyReady() {
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

void processCandidate(const Frame &candidate) {''',
    "Somfy payload decoder",
)
cpp = replace_once(
    cpp,
    "  const bool diagnostic = receiveTestActiveFlag;",
    "  const bool diagnostic = receiveTestActiveFlag && captureMode == CaptureMode::FixedOok;",
    "diagnostic mode",
)
cpp = replace_once(
    cpp,
    '''  strobe(SFRX);
  if (!strobe(SRX)) {
    failReceiver("cc1101_rx_failed", StatusRegistry::State::Error);
    return false;
  }

  lastEdgeUs = micros();
  attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN), onDataEdge, CHANGE);
  currentInfo.ready = true;
''',
    '''  strobe(SFRX);
  if (!strobe(SRX)) {
    failReceiver("cc1101_rx_failed", StatusRegistry::State::Error);
    return false;
  }
  currentInfo.configVerified = verifyFixedConfiguration();
  if (!currentInfo.configVerified) {
    failReceiver("cc1101_config_readback_failed", StatusRegistry::State::Error);
    SerialLog::error("RF433", "CC1101 configuration readback failed; SPI wiring/register writes not confirmed");
    return false;
  }

  captureMode = CaptureMode::FixedOok;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
  lastEdgeUs = micros();
  attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN), onDataEdge, CHANGE);
  currentInfo.ready = true;
''',
    "begin readback",
)
cpp = replace_once(
    cpp,
    '''  if (!strobe(SRX)) {
    detachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN));
    failReceiver("cc1101_probe_rx_failed", StatusRegistry::State::Error);
    return true;
  }

  currentInfo.partNumber = part;
''',
    '''  if (!strobe(SRX)) {
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
''',
    "probe readback",
)
cpp = replace_once(
    cpp,
    '''HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::ProtocolResponse; }

bool startReceiveTest() {''',
    '''HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::ProtocolResponse; }

const char *protocolName(Protocol protocol) {
  return protocol == Protocol::SomfyRts ? "Somfy RTS" : "Fixed OOK";
}

const char *somfyCommandName(uint8_t command) {
  switch (command & 0x0FU) {
    case 0x1: return "My";
    case 0x2: return "Up";
    case 0x3: return "My+Up";
    case 0x4: return "Down";
    case 0x5: return "My+Down";
    case 0x6: return "Up+Down";
    case 0x7: return "My+Up+Down";
    case 0x8: return "Prog";
    case 0x9: return "Sun";
    case 0xA: return "Flag";
    case 0xB: return "StepDown";
    case 0xC: return "Toggle";
    case 0xE: return "Sensor";
    default: return "Unknown";
  }
}

bool startReceiveTest() {''',
    "protocol text helpers",
)
old_test = '''bool startReceiveTest() {
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
new_test = '''bool startReceiveTest() {
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
cpp = replace_once(cpp, old_test, new_test, "auto receive test")
write("rf433_cc1101.cpp", cpp)

# Hardware card: active frequency, register readback and richer test frame details.
registry = read("hardware_registry.cpp")
registry = replace_once(
    registry,
    '    appendInfoString(out, first, "hardware.info.transport", "SPI / 433.92 MHz OOK");',
    '    appendInfoString(out, first, "hardware.info.transport", "SPI / OOK: 433.92 MHz + Somfy RTS 433.42 MHz Test");',
    "RF transport text",
)
registry = replace_once(
    registry,
    '    appendInfoUInt(out, first, "hardware.info.frequency", HardwareConfig::RF433_FREQUENCY_HZ);\n    appendInfoBool(out, first, "hardware.info.initialized", rf.initialized);',
    '    appendInfoUInt(out, first, "hardware.info.frequency", rf.activeFrequencyHz ? rf.activeFrequencyHz : HardwareConfig::RF433_FREQUENCY_HZ);\n    appendInfoBool(out, first, "hardware.info.initialized", rf.initialized);\n    appendInfoBool(out, first, "hardware.info.configVerified", rf.configVerified);',
    "RF readback info",
)
registry = replace_once(
    registry,
    '''    const auto &testFrame = Rf433Cc1101::lastTestFrame();
    if (testFrame.code != 0U) {
      appendInfoString(out, first, "hardware.info.rfTestFrame",
                       hexDword(testFrame.code) + " / " + String(testFrame.bitCount) + " bit");
    }
''',
    '''    const auto &testFrame = Rf433Cc1101::lastTestFrame();
    if (testFrame.code != 0U) {
      String frameText = String(Rf433Cc1101::protocolName(testFrame.protocol)) + " / ";
      if (testFrame.protocol == Rf433Cc1101::Protocol::SomfyRts) {
        frameText += "Addr ";
        frameText += hexDword(testFrame.code);
        frameText += " / ";
        frameText += Rf433Cc1101::somfyCommandName(testFrame.command);
        frameText += " / RC ";
        frameText += String(testFrame.rollingCode);
      } else {
        frameText += hexDword(testFrame.code) + " / " + String(testFrame.bitCount) + " bit";
      }
      appendInfoString(out, first, "hardware.info.rfTestFrame", frameText);
    }
''',
    "RF test frame formatting",
)
write("hardware_registry.cpp", registry)

# UI: make the single explicit test auto-scan both protocols for ten seconds.
js = read("ui-src/app.js")
replacements = [
    ("'hardware.info.rfTestResult': 'Empfangstest', 'hardware.info.rfTestFrame': 'Test-Frame', 'action.rf433Test': 'Empfang testen (5 s)',", "'hardware.info.rfTestResult': 'Empfangstest', 'hardware.info.rfTestFrame': 'Test-Frame', 'hardware.info.configVerified': 'Registerprüfung', 'action.rf433Test': 'Empfang testen (Auto 10 s)',"),
    ("'rf433.test.idle': 'Noch nicht getestet', 'rf433.test.waiting': 'Warte auf Funktelegramm …', 'rf433.test.received': 'Funktelegramm empfangen', 'rf433.test.timeout': 'Kein gültiges Telegramm empfangen', 'rf433.test.cancelled': 'Test abgebrochen',", "'rf433.test.idle': 'Noch nicht getestet', 'rf433.test.waiting': 'Scanne 433,92 MHz und Somfy RTS 433,42 MHz – Taste mehrfach drücken …', 'rf433.test.received': '433,92-MHz-Funktelegramm empfangen', 'rf433.test.somfy_received': 'Somfy RTS auf 433,42 MHz empfangen', 'rf433.test.timeout': 'Kein gültiges Festcode- oder Somfy-RTS-Telegramm empfangen', 'rf433.test.cancelled': 'Test abgebrochen',"),
    ("'hardware.info.rfTestResult': 'Receive test', 'hardware.info.rfTestFrame': 'Test frame', 'action.rf433Test': 'Test reception (5 s)',", "'hardware.info.rfTestResult': 'Receive test', 'hardware.info.rfTestFrame': 'Test frame', 'hardware.info.configVerified': 'Register verification', 'action.rf433Test': 'Test reception (auto 10 s)',"),
    ("'rf433.test.idle': 'Not tested yet', 'rf433.test.waiting': 'Waiting for radio frame …', 'rf433.test.received': 'Radio frame received', 'rf433.test.timeout': 'No valid frame received', 'rf433.test.cancelled': 'Test cancelled',", "'rf433.test.idle': 'Not tested yet', 'rf433.test.waiting': 'Scanning 433.92 MHz and Somfy RTS 433.42 MHz – press repeatedly …', 'rf433.test.received': '433.92 MHz radio frame received', 'rf433.test.somfy_received': 'Somfy RTS received at 433.42 MHz', 'rf433.test.timeout': 'No valid fixed-code or Somfy RTS frame received', 'rf433.test.cancelled': 'Test cancelled',"),
    ("'hardware.info.rfTestResult': 'Test ricezione', 'hardware.info.rfTestFrame': 'Frame di test', 'action.rf433Test': 'Test ricezione (5 s)',", "'hardware.info.rfTestResult': 'Test ricezione', 'hardware.info.rfTestFrame': 'Frame di test', 'hardware.info.configVerified': 'Verifica registri', 'action.rf433Test': 'Test ricezione (auto 10 s)',"),
    ("'rf433.test.idle': 'Non ancora testato', 'rf433.test.waiting': 'In attesa di un frame radio …', 'rf433.test.received': 'Frame radio ricevuto', 'rf433.test.timeout': 'Nessun frame valido ricevuto', 'rf433.test.cancelled': 'Test annullato',", "'rf433.test.idle': 'Non ancora testato', 'rf433.test.waiting': 'Scansione 433,92 MHz e Somfy RTS 433,42 MHz – premere più volte …', 'rf433.test.received': 'Frame radio 433,92 MHz ricevuto', 'rf433.test.somfy_received': 'Somfy RTS ricevuto a 433,42 MHz', 'rf433.test.timeout': 'Nessun frame fixed-code o Somfy RTS valido ricevuto', 'rf433.test.cancelled': 'Test annullato',"),
    ("'hardware.info.rfTestResult': 'Test réception', 'hardware.info.rfTestFrame': 'Trame de test', 'action.rf433Test': 'Tester réception (5 s)',", "'hardware.info.rfTestResult': 'Test réception', 'hardware.info.rfTestFrame': 'Trame de test', 'hardware.info.configVerified': 'Vérification registres', 'action.rf433Test': 'Tester réception (auto 10 s)',"),
    ("'rf433.test.idle': 'Pas encore testé', 'rf433.test.waiting': 'Attente d’une trame radio …', 'rf433.test.received': 'Trame radio reçue', 'rf433.test.timeout': 'Aucune trame valide reçue', 'rf433.test.cancelled': 'Test annulé',", "'rf433.test.idle': 'Pas encore testé', 'rf433.test.waiting': 'Balayage 433,92 MHz et Somfy RTS 433,42 MHz – appuyer plusieurs fois …', 'rf433.test.received': 'Trame radio 433,92 MHz reçue', 'rf433.test.somfy_received': 'Somfy RTS reçu à 433,42 MHz', 'rf433.test.timeout': 'Aucune trame fixed-code ou Somfy RTS valide reçue', 'rf433.test.cancelled': 'Test annulé',"),
    ("'hardware.info.rfTestResult': 'Empfangstest', 'hardware.info.rfTestFrame': 'Test-Frame', 'action.rf433Test': 'Empfang testa (5 s)',", "'hardware.info.rfTestResult': 'Empfangstest', 'hardware.info.rfTestFrame': 'Test-Frame', 'hardware.info.configVerified': 'Registerprüfig', 'action.rf433Test': 'Empfang testa (Auto 10 s)',"),
    ("'rf433.test.idle': 'No net testet', 'rf433.test.waiting': 'Wart auf a Funktelegramm …', 'rf433.test.received': 'Funktelegramm empfangen', 'rf433.test.timeout': 'Koi gültigs Telegramm empfangen', 'rf433.test.cancelled': 'Test abbrocha',", "'rf433.test.idle': 'No net testet', 'rf433.test.waiting': 'Scannt 433,92 MHz ond Somfy RTS 433,42 MHz – Knopf mehrafach drucka …', 'rf433.test.received': '433,92-MHz-Funktelegramm empfangen', 'rf433.test.somfy_received': 'Somfy RTS auf 433,42 MHz empfangen', 'rf433.test.timeout': 'Koi gültigs Festcode- oder Somfy-RTS-Telegramm empfangen', 'rf433.test.cancelled': 'Test abbrocha',"),
]
for old, new in replacements:
    if old not in js:
        raise SystemExit(f"missing JS translation anchor: {old[:80]}")
    js = js.replace(old, new, 1)
js = replace_once(js, "if (data.hardware?.checking && attempt < 4) this.followHardwareCheck(attempt + 1);", "if (data.hardware?.checking && attempt < 40) this.followHardwareCheck(attempt + 1);", "bounded hardware test follow-up")
write("ui-src/app.js", js)

# Portable checks explicitly guard the added diagnostics and Somfy-only test path.
checks = read("tools/release_check.py")
checks = replace_once(
    checks,
    '    check("RF433_SCK_PIN = 14" in hardware and "RF433_MISO_PIN = 32" in hardware and "RF433_MOSI_PIN = 23" in hardware and "RF433_CS_PIN = 25" in hardware and "RF433_GDO0_PIN = 26" in hardware and "RF433_GDO2_PIN = 27" in hardware, "CC1101 pin map")',
    '    check("RF433_SCK_PIN = 14" in hardware and "RF433_MISO_PIN = 32" in hardware and "RF433_MOSI_PIN = 23" in hardware and "RF433_CS_PIN = 25" in hardware and "RF433_GDO0_PIN = 26" in hardware and "RF433_GDO2_PIN = 27" in hardware, "CC1101 pin map")\n    check("RF433_SOMFY_FREQUENCY_HZ = 433420000UL" in hardware, "Somfy RTS 433.42 MHz test frequency")\n    rf_driver = (ROOT / "rf433_cc1101.cpp").read_text(encoding="utf-8")\n    check("Protocol::SomfyRts" in rf_driver and "decodeSomfyPayload" in rf_driver and "somfy_received" in rf_driver, "Somfy RTS receive-only diagnostic decoder")\n    check("verifyFixedConfiguration" in rf_driver and "configVerified" in (ROOT / "rf433_cc1101.h").read_text(encoding="utf-8"), "CC1101 register readback verification")',
    "Somfy release checks",
)
checks = replace_once(checks, 'check("rfSources: renderRfSources" in JS and "card.rf433" in JS, "RF source manager in Home UI")', 'check("rfSources: renderRfSources" in JS and "card.rf433" in JS, "RF source manager in Home UI")\n    check("rf433.test.somfy_received" in JS and "attempt < 40" in JS, "Somfy test UI and bounded 10-second hardware follow-up")', "Somfy UI check")
write("tools/release_check.py", checks)

# Test guide: Somfy is diagnostic-only until proven on the user's real remote.
doc = read("RF433_TEST.md")
doc = replace_once(
    doc,
    "Bewusst eng: gängige **433,92-MHz-ASK/OOK-Festcode-Sender** mit etwa 20–32 Bit und kurzen/langen Pulspaaren. Das ist noch kein universeller 433-MHz-Decoder. Rolling Code, Keeloq und unbekannte/proprietäre Protokolle sind nicht zugesichert.",
    "Bewusst eng: gängige **433,92-MHz-ASK/OOK-Festcode-Sender** mit etwa 20–32 Bit und kurzen/langen Pulspaaren. Zusätzlich besitzt der Hardwaretest jetzt einen **passiven Somfy-RTS-Decoder für 56-Bit-RTS auf 433,42 MHz**. Dieser RTS-Pfad dient zunächst nur zur Diagnose: Er liest Senderadresse, Rolling Code und Befehl, sendet selbst nichts und erzeugt beim Test keine Unterbrechung. Rolling-Code-Protokolle außerhalb Somfy RTS, Keeloq und unbekannte/proprietäre Protokolle sind nicht zugesichert.",
    "Somfy scope",
)
doc = replace_once(
    doc,
    "Unter **Gerät → Hardware** erscheint der CC1101 wie RTC, Display und Sound als normales Hardwaremodul. Dort werden Status, SPI-/Chipdaten, Pins sowie Frame-Zähler angezeigt. **Hardware prüfen** fragt den CC1101 über SPI erneut ab. **Empfang testen (5 s)** wartet auf ein gültiges Funktelegramm; ein dabei empfangenes Testtelegramm wird absichtlich nicht als Unterbrechung gespeichert.",
    "Unter **Gerät → Hardware** erscheint der CC1101 wie RTC, Display und Sound als normales Hardwaremodul. Dort werden Status, SPI-/Chipdaten, Pins, Registerprüfung sowie Frame-Zähler angezeigt. **Hardware prüfen** fragt den CC1101 über SPI erneut ab und verifiziert zusätzlich ausgewählte Konfigurationsregister per Readback. **Empfang testen (Auto 10 s)** wechselt während des Tests zwischen normalem 433,92-MHz-Festcodeempfang und **Somfy RTS 433,42 MHz**. Während der zehn Sekunden die Fernbedienung mehrfach drücken. Ein Testtelegramm wird absichtlich nicht als Unterbrechung gespeichert. Bei Somfy RTS zeigt `Test-Frame` die stabile 24-Bit-Senderadresse, den Befehl und den Rolling Code.",
    "hardware test guide",
)
doc = replace_once(
    doc,
    "- Der 5-s-Empfangstest muss bei einem passenden Tastendruck ein Test-Frame melden, ohne den Unterbrechungszähler zu erhöhen.",
    "- Der Auto-10-s-Empfangstest muss bei einem passenden Tastendruck ein Test-Frame melden, ohne den Unterbrechungszähler zu erhöhen. Für Somfy RTS mehrfach während des Tests drücken; Erfolg wird ausdrücklich als `Somfy RTS auf 433,42 MHz empfangen` angezeigt.\n- `Registerprüfung` sollte `Ja` zeigen. Das bestätigt SPI-Schreib-/Lesezugriff deutlich stärker als nur ein nicht-`0xFF` Chipwert; GDO-Leitungen und Antennen-/RF-Pfad gelten trotzdem erst nach einem erfolgreichen Empfangstest als praktisch bestätigt.",
    "test checklist",
)
write("RF433_TEST.md", doc)

print("Somfy RTS diagnostic patch applied")
