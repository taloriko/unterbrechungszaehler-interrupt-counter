#include "rf433_cc1101.h"

#include <SPI.h>
#include <driver/gpio.h>
#include <cstring>

#include "hardware_config.h"
#include "serial_log.h"
#include "status_registry.h"

namespace Rf433Cc1101 {
namespace {

constexpr uint8_t WRITE_BURST = 0x40;
constexpr uint8_t READ_SINGLE = 0x80;
constexpr uint8_t READ_STATUS = 0xC0;
constexpr uint8_t SRES = 0x30;
constexpr uint8_t SFRX = 0x3A;
constexpr uint8_t SIDLE = 0x36;
constexpr uint8_t SRX = 0x34;
constexpr uint8_t PARTNUM = 0x30;
constexpr uint8_t VERSION = 0x31;
constexpr uint16_t PULSE_BUFFER_SIZE = 160;
constexpr uint16_t MIN_FRAME_PULSES = 36;
constexpr uint32_t FRAME_GAP_US = 5000;
constexpr uint32_t FORCE_FRAME_GAP_US = 7000;
constexpr uint32_t REPEAT_WINDOW_MS = 650;
constexpr uint32_t PRESS_DEDUPE_MS = 550;
constexpr uint32_t RECEIVE_TEST_MS = 10000;
constexpr uint32_t RECEIVE_TEST_SWITCH_MS = 1500;
constexpr uint32_t SOMFY_HALF_MIN_US = 448;
constexpr uint32_t SOMFY_HALF_MAX_US = 832;
constexpr uint32_t SOMFY_SYMBOL_MIN_US = 896;
constexpr uint32_t SOMFY_SYMBOL_MAX_US = 1664;
constexpr uint32_t SOMFY_HW_SYNC_MIN_US = 1792;
constexpr uint32_t SOMFY_HW_SYNC_MAX_US = 3328;
constexpr uint32_t SOMFY_SW_SYNC_MIN_US = 3395;
constexpr uint32_t SOMFY_SW_SYNC_MAX_US = 6305;
constexpr uint32_t SOMFY_GLITCH_MIN_US = 300;

struct RegisterSetting { uint8_t address; uint8_t value; };

// 433.92 MHz, ASK/OOK, asynchronous serial receive. The values intentionally
// keep the CC1101 as a narrow, low-overhead demodulator; protocol recognition
// happens on the ESP32 from GDO0 pulse widths. This first prototype targets
// common fixed-code remotes, not rolling-code/keyfob protocols.
constexpr RegisterSetting SETTINGS[] = {
    {0x02, 0x0D},  // IOCFG0: asynchronous serial data output
    {0x00, 0x0E},  // IOCFG2: carrier sense
    {0x03, 0x47},  // FIFOTHR
    {0x07, 0x00},  // PKTCTRL1
    {0x08, 0x32},  // PKTCTRL0: asynchronous serial mode
    {0x0B, 0x06},  // FSCTRL1
    {0x0D, 0x10},  // FREQ2
    {0x0E, 0xB0},  // FREQ1
    {0x0F, 0x71},  // FREQ0 -> 433.92 MHz with 26 MHz crystal
    {0x10, 0xC6},  // MDMCFG4: ~101 kHz RX BW, ~2.5 kBaud reference
    {0x11, 0x93},  // MDMCFG3
    {0x12, 0x30},  // MDMCFG2: ASK/OOK, no sync qualifier
    {0x13, 0x22},  // MDMCFG1
    {0x14, 0xF8},  // MDMCFG0
    {0x15, 0x42},  // DEVIATN (irrelevant for OOK but stable SmartRF value)
    {0x18, 0x18},  // MCSM0: autocalibrate IDLE -> RX/TX
    {0x19, 0x14},  // FOCCFG
    {0x1A, 0x1C},  // BSCFG
    {0x1B, 0x04},  // AGCCTRL2
    {0x1C, 0x00},  // AGCCTRL1
    {0x1D, 0x92},  // AGCCTRL0
    {0x20, 0xFB},  // WORCTRL
    {0x21, 0x56},  // FREND1
    {0x22, 0x11},  // FREND0
    {0x23, 0xEA},  // FSCAL3
    {0x24, 0x2A},  // FSCAL2
    {0x25, 0x00},  // FSCAL1
    {0x26, 0x1F},  // FSCAL0
    {0x2C, 0x81},  // TEST2
    {0x2D, 0x35},  // TEST1
    {0x2E, 0x09},  // TEST0
};

SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE0);
Info currentInfo;

volatile uint16_t pulseBuffers[2][PULSE_BUFFER_SIZE]{};
volatile uint8_t writeBuffer = 0;
volatile uint16_t writeCount = 0;
volatile uint32_t lastEdgeUs = 0;
volatile bool readyFrame = false;
volatile uint8_t readyBuffer = 0;
volatile uint16_t readyCount = 0;
volatile uint32_t isrOverflowFrames = 0;

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
volatile uint8_t somfyReadySyncCount = 0;

void setHealth(StatusRegistry::State state) {
  currentHealth = state;
  StatusRegistry::setState("rf433", state);
}

void failReceiver(const char *error, StatusRegistry::State state) {
  currentInfo.ready = false;
  currentInfo.error = error ? error : "rf433_error";
  checkedAtMs = millis();
  setHealth(state);
}

bool selectChip() {
  SPI.beginTransaction(spiSettings);
  digitalWrite(HardwareConfig::RF433_CS_PIN, LOW);
  const uint32_t started = micros();
  while (digitalRead(HardwareConfig::RF433_MISO_PIN) != LOW) {
    if (static_cast<uint32_t>(micros() - started) > 2500U) {
      digitalWrite(HardwareConfig::RF433_CS_PIN, HIGH);
      SPI.endTransaction();
      return false;
    }
  }
  return true;
}

void deselectChip() {
  digitalWrite(HardwareConfig::RF433_CS_PIN, HIGH);
  SPI.endTransaction();
}

bool strobe(uint8_t command) {
  if (!selectChip()) return false;
  SPI.transfer(command);
  deselectChip();
  return true;
}

bool writeRegister(uint8_t address, uint8_t value) {
  if (!selectChip()) return false;
  SPI.transfer(address);
  SPI.transfer(value);
  deselectChip();
  return true;
}

uint8_t readStatusRegister(uint8_t address) {
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

void IRAM_ATTR finalizeFrameFromIsr() {
  if (writeCount < MIN_FRAME_PULSES) {
    writeCount = 0;
    return;
  }
  if (readyFrame) {
    ++isrOverflowFrames;
    writeCount = 0;
    return;
  }
  readyBuffer = writeBuffer;
  readyCount = writeCount;
  readyFrame = true;
  writeBuffer ^= 1U;
  writeCount = 0;
}

void IRAM_ATTR onDataEdge() {
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

void finalizeSilentFrameIfNeeded() {
  if (writeCount < MIN_FRAME_PULSES) return;
  if (static_cast<uint32_t>(micros() - lastEdgeUs) < FORCE_FRAME_GAP_US) return;
  noInterrupts();
  finalizeFrameFromIsr();
  interrupts();
}

bool similarBucket(uint8_t a, uint8_t b) {
  const uint8_t difference = a > b ? static_cast<uint8_t>(a - b) : static_cast<uint8_t>(b - a);
  return difference <= 3U;
}

bool sameFrame(const Frame &a, const Frame &b) {
  return a.code == b.code && a.bitCount == b.bitCount && similarBucket(a.pulseBucket, b.pulseBucket);
}

bool decodeFrame(const uint16_t *timings, uint16_t count, Frame &out) {
  if (!timings || count < MIN_FRAME_PULSES) return false;

  bool found = false;
  uint8_t bestBits = 0;
  uint32_t bestCode = 0;
  uint32_t bestShortSum = 0;
  uint32_t bestRatioError = UINT32_MAX;

  // A fixed-code OOK frame is normally alternating short/long pulse pairs. Try
  // the first few phase offsets instead of hardcoding one remote family.
  for (uint8_t start = 0; start < 4 && start + 1U < count; ++start) {
    uint32_t code = 0;
    uint8_t bits = 0;
    uint32_t shortSum = 0;
    uint32_t ratioError = 0;

    for (uint16_t i = start; i + 1U < count && bits < 32U; i += 2U) {
      const uint32_t first = timings[i];
      const uint32_t second = timings[i + 1U];
      const uint32_t shortPulse = first < second ? first : second;
      const uint32_t longPulse = first < second ? second : first;
      if (shortPulse < 100U || shortPulse > 1400U || longPulse > 4600U) break;
      const uint32_t ratio100 = (longPulse * 100U) / shortPulse;
      if (ratio100 < 165U || ratio100 > 480U) break;

      const uint32_t idealLong = shortPulse * 3U;
      ratioError += idealLong > longPulse ? idealLong - longPulse : longPulse - idealLong;
      shortSum += shortPulse;
      code = (code << 1U) | (first > second ? 1U : 0U);
      ++bits;
    }

    if (bits < 20U || bits > 32U || code == 0U) continue;
    if (!found || bits > bestBits || (bits == bestBits && ratioError < bestRatioError)) {
      found = true;
      bestBits = bits;
      bestCode = code;
      bestShortSum = shortSum;
      bestRatioError = ratioError;
    }
  }

  if (!found || bestBits == 0) return false;
  const uint32_t averageShort = bestShortSum / bestBits;
  const uint32_t bucket = (averageShort + 12U) / 25U;
  out.protocol = Protocol::FixedOok;
  out.code = bestCode;
  out.bitCount = bestBits;
  out.pulseBucket = static_cast<uint8_t>(bucket > 255U ? 255U : bucket);
  out.repeats = 1;
  return true;
}

bool decodeSomfyPayload(const uint8_t encoded[7], Frame &out) {
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

void processCandidate(const Frame &candidate) {
  const uint32_t nowMs = millis();
  if (sameFrame(candidate, pendingCandidate) && static_cast<uint32_t>(nowMs - pendingSeenMs) <= REPEAT_WINDOW_MS) {
    if (pendingRepeats < 255U) ++pendingRepeats;
  } else {
    pendingCandidate = candidate;
    pendingRepeats = 1;
  }
  pendingSeenMs = nowMs;

  if (pendingRepeats < 2U) return;
  if (sameFrame(candidate, currentInfo.lastFrame) && static_cast<uint32_t>(nowMs - lastEmitMs) < PRESS_DEDUPE_MS) return;

  const bool diagnostic = receiveTestActiveFlag && captureMode == CaptureMode::FixedOok;
  if (diagnostic) {
    receiveTestActiveFlag = false;
    receiveTestResultText = "received";
    receiveTestFrame = candidate;
    checkedAtMs = nowMs;
    setHealth(StatusRegistry::State::Ok);
    SerialLog::successf("RF433", "Receive test passed | bits=%u | code=0x%08lX",
                        static_cast<unsigned int>(candidate.bitCount),
                        static_cast<unsigned long>(candidate.code));
  }

  emittedFrame = candidate;
  emittedFrame.repeats = pendingRepeats;
  emittedFrame.diagnostic = diagnostic;
  emittedAvailable = true;
  currentInfo.lastFrame = emittedFrame;
  ++currentInfo.decodedFrames;
  lastEmitMs = nowMs;
}

void processReadyFrame() {
  finalizeSilentFrameIfNeeded();
  if (!readyFrame) return;

  uint16_t local[PULSE_BUFFER_SIZE];
  uint16_t count = 0;
  noInterrupts();
  if (readyFrame) {
    count = readyCount > PULSE_BUFFER_SIZE ? PULSE_BUFFER_SIZE : readyCount;
    for (uint16_t i = 0; i < count; ++i) local[i] = pulseBuffers[readyBuffer][i];
    readyFrame = false;
    readyCount = 0;
  }
  const uint32_t overflow = isrOverflowFrames;
  interrupts();
  currentInfo.overflowFrames = overflow;
  if (count == 0) return;

  Frame candidate;
  if (!decodeFrame(local, count, candidate)) {
    ++currentInfo.rejectedFrames;
    return;
  }
  processCandidate(candidate);
}

}  // namespace

bool begin() {
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
  if (!verifyFixedConfiguration()) {
    failReceiver("cc1101_probe_readback_failed", StatusRegistry::State::Error);
    SerialLog::error("RF433", "Manual probe: configuration register readback failed");
    return true;
  }

  currentInfo.configVerified = true;
  currentInfo.activeFrequencyHz = HardwareConfig::RF433_FREQUENCY_HZ;
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

bool startReceiveTest() {
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

bool pollFrame(Frame &frameOut) {
  update();
  if (!emittedAvailable) return false;
  frameOut = emittedFrame;
  emittedAvailable = false;
  return true;
}

const Info &info() {
  currentInfo.overflowFrames = isrOverflowFrames;
  return currentInfo;
}

}  // namespace Rf433Cc1101
