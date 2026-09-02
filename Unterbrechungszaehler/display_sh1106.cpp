#include "display_sh1106.h"

#include <cstring>

#include "config.h"
#include "hardware_config.h"
#include "i2c_bus.h"
#include "serial_log.h"
#include "status_registry.h"

namespace DisplaySh1106 {
namespace {

StatusRegistry::State moduleHealth = StatusRegistry::State::Unknown;
bool isDetected = false;
bool isInitialized = false;
bool transferOk = false;
uint32_t checkedAtMs = 0;
const char *errorText = "";
uint32_t bootScreenUntilMs = 0;
uint32_t manualTestUntilMs = 0;

bool deadlinePending(uint32_t deadlineMs) {
  return deadlineMs != 0U && static_cast<int32_t>(millis() - deadlineMs) < 0;
}

// 128 x 64 x 1 bit = 1024 bytes. A single reusable framebuffer keeps drawing
// code simple while staying far smaller than a graphics framework.
uint8_t frameBuffer[(HardwareConfig::DISPLAY_WIDTH * HardwareConfig::DISPLAY_HEIGHT) / 8]{};

// Compact 5x7 uppercase font. Only ASCII used by the reusable boot/test UI is
// included; lowercase input is normalized to uppercase.
const char FONT_CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-:/";
const uint8_t FONT_DATA[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // space
  {0x7E,0x11,0x11,0x11,0x7E}, // A
  {0x7F,0x49,0x49,0x49,0x36}, // B
  {0x3E,0x41,0x41,0x41,0x22}, // C
  {0x7F,0x41,0x41,0x22,0x1C}, // D
  {0x7F,0x49,0x49,0x49,0x41}, // E
  {0x7F,0x09,0x09,0x09,0x01}, // F
  {0x3E,0x41,0x49,0x49,0x7A}, // G
  {0x7F,0x08,0x08,0x08,0x7F}, // H
  {0x00,0x41,0x7F,0x41,0x00}, // I
  {0x20,0x40,0x41,0x3F,0x01}, // J
  {0x7F,0x08,0x14,0x22,0x41}, // K
  {0x7F,0x40,0x40,0x40,0x40}, // L
  {0x7F,0x02,0x0C,0x02,0x7F}, // M
  {0x7F,0x04,0x08,0x10,0x7F}, // N
  {0x3E,0x41,0x41,0x41,0x3E}, // O
  {0x7F,0x09,0x09,0x09,0x06}, // P
  {0x3E,0x41,0x51,0x21,0x5E}, // Q
  {0x7F,0x09,0x19,0x29,0x46}, // R
  {0x46,0x49,0x49,0x49,0x31}, // S
  {0x01,0x01,0x7F,0x01,0x01}, // T
  {0x3F,0x40,0x40,0x40,0x3F}, // U
  {0x1F,0x20,0x40,0x20,0x1F}, // V
  {0x3F,0x40,0x38,0x40,0x3F}, // W
  {0x63,0x14,0x08,0x14,0x63}, // X
  {0x07,0x08,0x70,0x08,0x07}, // Y
  {0x61,0x51,0x49,0x45,0x43}, // Z
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9
  {0x00,0x60,0x60,0x00,0x00}, // .
  {0x08,0x08,0x08,0x08,0x08}, // -
  {0x00,0x36,0x36,0x00,0x00}, // :
  {0x20,0x10,0x08,0x04,0x02}, // /
};

void setHealth(StatusRegistry::State state, const char *message = "") {
  moduleHealth = state;
  errorText = message ? message : "";
  StatusRegistry::setState("display", state);
}

bool command(uint8_t value) {
  const uint8_t payload[2] = {0x00, value};
  transferOk = I2cBus::write(HardwareConfig::DISPLAY_SH1106_ADDRESS, payload, sizeof(payload));
  if (!transferOk) {
    setHealth(StatusRegistry::State::Error, "I2C command transfer failed");
    SerialLog::errorf("DISPLAY", "SH1106 command failed | command=0x%02X", value);
  }
  return transferOk;
}

bool command2(uint8_t first, uint8_t second) {
  const uint8_t payload[3] = {0x00, first, second};
  transferOk = I2cBus::write(HardwareConfig::DISPLAY_SH1106_ADDRESS, payload, sizeof(payload));
  if (!transferOk) {
    setHealth(StatusRegistry::State::Error, "I2C command transfer failed");
    SerialLog::errorf("DISPLAY", "SH1106 command failed | command=0x%02X", first);
  }
  return transferOk;
}

bool dataChunk(const uint8_t *data, size_t length) {
  if (!data || length == 0 || length > 24) return false;
  uint8_t payload[25];
  payload[0] = 0x40;
  for (size_t i = 0; i < length; ++i) payload[i + 1] = data[i];
  transferOk = I2cBus::write(HardwareConfig::DISPLAY_SH1106_ADDRESS, payload, length + 1);
  if (!transferOk) {
    setHealth(StatusRegistry::State::Error, "I2C data transfer failed");
    SerialLog::error("DISPLAY", "SH1106 data transfer failed");
  }
  return transferOk;
}

void clearFrame() {
  std::memset(frameBuffer, 0, sizeof(frameBuffer));
}

void pixel(int16_t x, int16_t y, bool on = true) {
  if (x < 0 || y < 0 || x >= HardwareConfig::DISPLAY_WIDTH || y >= HardwareConfig::DISPLAY_HEIGHT) return;
  const size_t index = static_cast<size_t>(y / 8) * HardwareConfig::DISPLAY_WIDTH + static_cast<size_t>(x);
  const uint8_t mask = static_cast<uint8_t>(1U << (y & 7));
  if (on) frameBuffer[index] |= mask;
  else frameBuffer[index] &= static_cast<uint8_t>(~mask);
}

void hLine(int16_t x0, int16_t x1, int16_t y) {
  if (x0 > x1) { const int16_t t = x0; x0 = x1; x1 = t; }
  for (int16_t x = x0; x <= x1; ++x) pixel(x, y);
}

void vLine(int16_t x, int16_t y0, int16_t y1) {
  if (y0 > y1) { const int16_t t = y0; y0 = y1; y1 = t; }
  for (int16_t y = y0; y <= y1; ++y) pixel(x, y);
}

void rect(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (w < 1 || h < 1) return;
  hLine(x, static_cast<int16_t>(x + w - 1), y);
  hLine(x, static_cast<int16_t>(x + w - 1), static_cast<int16_t>(y + h - 1));
  vLine(x, y, static_cast<int16_t>(y + h - 1));
  vLine(static_cast<int16_t>(x + w - 1), y, static_cast<int16_t>(y + h - 1));
}

const uint8_t *glyph(char c) {
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - ('a' - 'A'));
  for (size_t i = 0; i < sizeof(FONT_CHARS) - 1; ++i) {
    if (FONT_CHARS[i] == c) return FONT_DATA[i];
  }
  return FONT_DATA[0];
}

void text(int16_t x, int16_t y, const char *value) {
  if (!value) return;
  while (*value && x < HardwareConfig::DISPLAY_WIDTH - 5) {
    const uint8_t *g = glyph(*value++);
    for (uint8_t col = 0; col < 5; ++col) {
      for (uint8_t row = 0; row < 7; ++row) {
        if (g[col] & (1U << row)) pixel(static_cast<int16_t>(x + col), static_cast<int16_t>(y + row));
      }
    }
    x = static_cast<int16_t>(x + 6);
  }
}

void scaledText(int16_t x, int16_t y, const char *value, uint8_t scale) {
  if (!value || scale == 0) return;
  while (*value && x < HardwareConfig::DISPLAY_WIDTH) {
    const uint8_t *g = glyph(*value++);
    for (uint8_t col = 0; col < 5; ++col) {
      for (uint8_t row = 0; row < 7; ++row) {
        if (!(g[col] & (1U << row))) continue;
        for (uint8_t dx = 0; dx < scale; ++dx) {
          for (uint8_t dy = 0; dy < scale; ++dy) {
            pixel(static_cast<int16_t>(x + static_cast<int16_t>(col * scale + dx)),
                  static_cast<int16_t>(y + static_cast<int16_t>(row * scale + dy)));
          }
        }
      }
    }
    x = static_cast<int16_t>(x + static_cast<int16_t>(6U * scale));
  }
}

void centeredText(int16_t y, const char *value) {
  if (!value) return;
  size_t len = std::strlen(value);
  if (len > 21) len = 21;
  const int16_t width = static_cast<int16_t>(len * 6 - (len ? 1 : 0));
  int16_t x = static_cast<int16_t>((HardwareConfig::DISPLAY_WIDTH - width) / 2);
  char clipped[22]{};
  std::memcpy(clipped, value, len);
  clipped[len] = '\0';
  text(x, y, clipped);
}

void drawChipIcon(int16_t x, int16_t y) {
  rect(x + 4, y + 4, 16, 16);
  rect(x + 8, y + 8, 8, 8);
  for (int16_t p = 6; p <= 18; p += 4) {
    hLine(x, x + 3, y + p);
    hLine(x + 20, x + 23, y + p);
    vLine(x + p, y, y + 3);
    vLine(x + p, y + 20, y + 23);
  }
}

bool flushFrame() {
  if (!isInitialized && !initialize()) return false;
  for (uint8_t page = 0; page < 8; ++page) {
    if (!command(static_cast<uint8_t>(0xB0 | page))) return false;
    const uint8_t column = HardwareConfig::DISPLAY_COLUMN_OFFSET;
    if (!command(static_cast<uint8_t>(0x00 | (column & 0x0F)))) return false;
    if (!command(static_cast<uint8_t>(0x10 | ((column >> 4) & 0x0F)))) return false;
    const uint8_t *pageData = &frameBuffer[static_cast<size_t>(page) * HardwareConfig::DISPLAY_WIDTH];
    uint16_t sent = 0;
    while (sent < HardwareConfig::DISPLAY_WIDTH) {
      const uint16_t remaining = static_cast<uint16_t>(HardwareConfig::DISPLAY_WIDTH - sent);
      const size_t chunk = remaining > 24 ? 24 : remaining;
      if (!dataChunk(pageData + sent, chunk)) return false;
      sent = static_cast<uint16_t>(sent + chunk);
    }
  }
  setHealth(StatusRegistry::State::Ok);
  return true;
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("display", "status.display", "display", HardwareConfig::ENABLE_DISPLAY_SH1106);
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106) {
    setHealth(StatusRegistry::State::Disabled);
    StatusRegistry::setVisible("display", false);
    return false;
  }

  I2cBus::begin();
  probe();
  if (isDetected && HardwareConfig::DISPLAY_BOOT_SCREEN_ENABLED) showBootScreen();
  return isDetected;
}

void probe() {
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106) {
    setHealth(StatusRegistry::State::Disabled);
    return;
  }

  setHealth(StatusRegistry::State::Checking);
  isDetected = I2cBus::probe(HardwareConfig::DISPLAY_SH1106_ADDRESS);
  checkedAtMs = millis();
  transferOk = isDetected;
  if (!isDetected) {
    isInitialized = false;
    setHealth(StatusRegistry::State::NoResponse, "no I2C response");
    SerialLog::errorf("DISPLAY", "SH1106: NO RESPONSE | I2C address=0x%02X", HardwareConfig::DISPLAY_SH1106_ADDRESS);
    return;
  }

  setHealth(StatusRegistry::State::Ok);
  SerialLog::successf("DISPLAY", "SH1106: OK | transport=I2C | address=0x%02X | configured=%ux%u",
                      HardwareConfig::DISPLAY_SH1106_ADDRESS,
                      HardwareConfig::DISPLAY_WIDTH, HardwareConfig::DISPLAY_HEIGHT);
}

bool enabled() { return HardwareConfig::ENABLE_DISPLAY_SH1106; }
bool detected() { return isDetected; }
bool initialized() { return isInitialized; }
bool lastTransferOk() { return transferOk; }
StatusRegistry::State health() { return moduleHealth; }
uint32_t lastCheckMs() { return checkedAtMs; }
const char *lastError() { return errorText; }
HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::TransportAck; }
bool bootScreenActive() { return deadlinePending(bootScreenUntilMs); }
bool manualTestActive() { return deadlinePending(manualTestUntilMs); }

bool initialize() {
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106 || !isDetected) return false;

  const bool ok =
      command(0xAE) &&
      command2(0xD5, 0x80) &&
      command2(0xA8, 0x3F) &&
      command2(0xD3, 0x00) &&
      command(0x40) &&
      command(0xA1) &&
      command(0xC8) &&
      command2(0xDA, 0x12) &&
      command2(0x81, 0x80) &&
      command2(0xD9, 0x22) &&
      command2(0xDB, 0x35) &&
      command2(0xAD, 0x8B) &&
      command(0xA4) &&
      command(0xA6) &&
      command(0xAF);

  isInitialized = ok;
  if (ok) {
    setHealth(StatusRegistry::State::Ok);
    SerialLog::success("DISPLAY", "SH1106 initialized | transfer ACK received");
  }
  return ok;
}

bool clear() {
  clearFrame();
  return flushFrame();
}

bool setPower(bool on) {
  if (!isInitialized && !initialize()) return false;
  const bool ok = command(on ? 0xAF : 0xAE);
  if (ok) setHealth(StatusRegistry::State::Ok);
  return ok;
}

bool setContrast(uint8_t contrast) {
  if (!isInitialized && !initialize()) return false;
  const bool ok = command2(0x81, contrast);
  if (ok) setHealth(StatusRegistry::State::Ok);
  return ok;
}

bool showBootScreen() {
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106 || !isDetected) return false;
  if (!setPower(true)) return false;
  clearFrame();
  drawChipIcon(52, 1);
  centeredText(28, AppConfig::PROJECT_NAME);
  centeredText(40, AppConfig::SOFTWARE_VERSION);
  centeredText(52, "STARTING");
  const bool ok = flushFrame();
  if (ok) {
    bootScreenUntilMs = millis() + HardwareConfig::DISPLAY_BOOT_SCREEN_MIN_MS;
    SerialLog::successf("DISPLAY", "Boot screen shown | minimum=%lu ms",
                        static_cast<unsigned long>(HardwareConfig::DISPLAY_BOOT_SCREEN_MIN_MS));
  }
  return ok;
}

void frameClear() { clearFrame(); }
void drawPixel(int16_t x, int16_t y, bool on) { pixel(x, y, on); }
void drawHLine(int16_t x0, int16_t x1, int16_t y) { hLine(x0, x1, y); }
void drawVLine(int16_t x, int16_t y0, int16_t y1) { vLine(x, y0, y1); }
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h) { rect(x, y, w, h); }
void drawText(int16_t x, int16_t y, const char *value) { text(x, y, value); }
void drawTextScaled(int16_t x, int16_t y, const char *value, uint8_t scale) { scaledText(x, y, value, scale); }
void drawCenteredText(int16_t y, const char *value) { centeredText(y, value); }
bool present() { return flushFrame(); }

bool setInverted(bool inverted) {
  if (!isInitialized && !initialize()) return false;
  const bool ok = command(inverted ? 0xA7 : 0xA6);
  if (ok) setHealth(StatusRegistry::State::Ok);
  return ok;
}


bool showTestScreen() {
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106 || !isDetected) return false;
  if (!setPower(true)) return false;
  clearFrame();
  rect(0, 0, HardwareConfig::DISPLAY_WIDTH, HardwareConfig::DISPLAY_HEIGHT);
  centeredText(9, "DISPLAY TEST");
  hLine(8, 119, 20);
  centeredText(27, "SH1106");
  centeredText(39, "128X64");
  hLine(8, 119, 50);
  for (int16_t x = 10; x <= 116; x += 8) {
    pixel(x, 56);
    pixel(x + 1, 57);
    pixel(x + 2, 58);
  }
  const bool ok = flushFrame();
  if (ok) {
    manualTestUntilMs = millis() + HardwareConfig::DISPLAY_TEST_SCREEN_MS;
    SerialLog::success("DISPLAY", "Display test screen shown");
  }
  return ok;
}

}  // namespace DisplaySh1106
