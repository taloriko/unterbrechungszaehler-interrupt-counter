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
constexpr uint32_t RECEIVE_TEST_MS = 5000;

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
  out.code = bestCode;
  out.bitCount = bestBits;
  out.pulseBucket = static_cast<uint8_t>(bucket > 255U ? 255U : bucket);
  out.repeats = 1;
  return true;
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

  const bool diagnostic = receiveTestActiveFlag;
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
