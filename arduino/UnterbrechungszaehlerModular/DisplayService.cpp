#include "DisplayService.h"

#include <Wire.h>

#include "Config.h"
#include "NetworkService.h"
#include "RtcService.h"
#include "StorageService.h"
#include "TimeService.h"

namespace {
const uint8_t FONT_DIGITS[10][5] = {
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
};

const uint8_t FONT_UPPER[26][5] = {
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
  {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

const char* layoutName(DisplayLayout layout) {
  switch (layout) {
    case DisplayLayout::Compact: return "kompakt";
    case DisplayLayout::Clock: return "uhr";
    default: return "standard";
  }
}
}

void DisplayService::begin(RtcService* rtc,
                           TimeService* time,
                           StorageService* storage,
                           NetworkService* network) {
  rtc_ = rtc;
  time_ = time;
  storage_ = storage;
  network_ = network;
  loadSettings();

  if (probe(UicConfig::OLED_ADDRESS_1)) {
    present_ = true;
    address_ = UicConfig::OLED_ADDRESS_1;
  } else if (probe(UicConfig::OLED_ADDRESS_2)) {
    present_ = true;
    address_ = UicConfig::OLED_ADDRESS_2;
  }

  Serial.printf("[DISPLAY] SH1106 %s", present_ ? "erkannt" : "nicht erkannt");
  if (present_) {
    Serial.printf(" | 0x%02X | Hell %u | Dim %u nach %us | Aus %lus | Layout %s",
                  address_, brightness_, dimBrightness_, dimAfterSeconds_,
                  static_cast<unsigned long>(offAfterSeconds_), layoutName(layout_));
  }
  Serial.println();
}

void DisplayService::tick() {
  if (!active_) return;
  const uint32_t now = millis();

  if (screenMode_ != ScreenMode::Live && bootUntil_ != 0 &&
      static_cast<int32_t>(now - bootUntil_) >= 0) {
    screenMode_ = ScreenMode::Live;
    bootUntil_ = 0;
    renderFrame(currentAutarkMode_, ScreenMode::Live);
    flush();
  }

  if (screenMode_ == ScreenMode::Live && now - lastFrameAt_ >= 1000UL) {
    renderFrame(currentAutarkMode_, ScreenMode::Live);
    flush();
  }

  if (!dimmed_ && dimAt_ != 0 && static_cast<int32_t>(now - dimAt_) >= 0) {
    if (setContrast(dimBrightness_)) dimmed_ = true;
  }

  if (offAt_ != 0 && static_cast<int32_t>(now - offAt_) >= 0) off();
}

void DisplayService::showBootStatus(bool autarkMode) {
  if (!present_ || !initializeController()) return;
  currentAutarkMode_ = autarkMode;
  screenMode_ = ScreenMode::Boot;
  bootUntil_ = autarkMode ? 0 : millis() + 3000UL;
  renderFrame(autarkMode, ScreenMode::Boot);
  flush();
  wake(autarkMode, true);
}

bool DisplayService::showTest(bool autarkMode) {
  if (!present_) return false;
  if (!active_ && !initializeController()) return false;
  currentAutarkMode_ = autarkMode;
  screenMode_ = ScreenMode::Test;
  bootUntil_ = millis() + 5000UL;
  renderFrame(autarkMode, ScreenMode::Test);
  flush();
  wake(autarkMode, true);
  return active_;
}

void DisplayService::notifyActivity(bool autarkMode) {
  if (!present_) return;
  currentAutarkMode_ = autarkMode;
  screenMode_ = ScreenMode::Live;
  bootUntil_ = 0;
  renderFrame(autarkMode, ScreenMode::Live);

  if (!active_) {
    if (!initializeController()) return;
    flush();
  } else {
    flush();
  }
  wake(autarkMode, true);
}

void DisplayService::notifyEvent(bool autarkMode) {
  if (wakeOnEvent_) {
    notifyActivity(autarkMode);
    return;
  }

  // Wenn Aufwecken deaktiviert ist, wird ein bereits aktives Display trotzdem
  // aktualisiert. Ein ausgeschaltetes Display bleibt dagegen wirklich aus.
  if (!present_ || !active_) return;
  currentAutarkMode_ = autarkMode;
  screenMode_ = ScreenMode::Live;
  renderFrame(autarkMode, ScreenMode::Live);
  flush();
}

void DisplayService::wake(bool autarkMode, bool resetTimers) {
  if (!present_) return;
  setContrast(brightness_);
  applyOrientation();
  command(0xAF);
  active_ = true;
  dimmed_ = false;
  if (resetTimers) armActivityTimers(autarkMode);
}

void DisplayService::off() {
  if (!present_) return;
  command(0xAE);
  active_ = false;
  dimmed_ = false;
  dimAt_ = 0;
  offAt_ = 0;
}

bool DisplayService::setSettings(uint8_t brightness,
                                 uint8_t dimBrightness,
                                 uint16_t dimAfterSeconds,
                                 uint32_t offAfterSeconds,
                                 bool wakeOnEvent,
                                 bool inverted,
                                 bool rotation180,
                                 DisplayLayout layout) {
  if (brightness < 1) brightness = 1;
  if (dimBrightness < 1) dimBrightness = 1;
  if (dimBrightness > brightness) dimBrightness = brightness;
  if (dimAfterSeconds < 5) dimAfterSeconds = 5;
  if (dimAfterSeconds > 3600) dimAfterSeconds = 3600;
  if (offAfterSeconds != 0 && offAfterSeconds < 5) offAfterSeconds = 5;
  if (offAfterSeconds > 86400UL) offAfterSeconds = 86400UL;
  if (static_cast<uint8_t>(layout) > static_cast<uint8_t>(DisplayLayout::Clock)) {
    layout = DisplayLayout::Standard;
  }

  preferences_.begin("interrupt", false);
  bool ok = true;
  ok = preferences_.putUChar("oledBright", brightness) > 0 && ok;
  ok = preferences_.putUChar("oledDimBrt", dimBrightness) > 0 && ok;
  ok = preferences_.putUShort("oledDimSec", dimAfterSeconds) > 0 && ok;
  preferences_.putULong("oledOffSec", offAfterSeconds);
  preferences_.putBool("oledWakeEvt", wakeOnEvent);
  preferences_.putBool("oledInvert", inverted);
  preferences_.putBool("oledRot180", rotation180);
  preferences_.putUChar("oledLayout", static_cast<uint8_t>(layout));
  preferences_.end();
  if (!ok) return false;

  brightness_ = brightness;
  dimBrightness_ = dimBrightness;
  dimAfterSeconds_ = dimAfterSeconds;
  offAfterSeconds_ = offAfterSeconds;
  wakeOnEvent_ = wakeOnEvent;
  inverted_ = inverted;
  rotation180_ = rotation180;
  layout_ = layout;

  renderFrame(currentAutarkMode_, screenMode_ == ScreenMode::Boot ? ScreenMode::Boot : ScreenMode::Live);
  if (active_) {
    applyOrientation();
    flush();
    setContrast(brightness_);
    dimmed_ = false;
    armActivityTimers(currentAutarkMode_);
  }
  return true;
}

void DisplayService::loadSettings() {
  preferences_.begin("interrupt", true);
  brightness_ = preferences_.getUChar("oledBright", 127);
  dimBrightness_ = preferences_.getUChar("oledDimBrt", 32);
  dimAfterSeconds_ = preferences_.getUShort("oledDimSec", 60);
  offAfterSeconds_ = preferences_.getULong("oledOffSec", 0);
  wakeOnEvent_ = preferences_.getBool("oledWakeEvt", true);
  inverted_ = preferences_.getBool("oledInvert", false);
  rotation180_ = preferences_.getBool("oledRot180", false);
  const uint8_t layoutValue = preferences_.getUChar("oledLayout", 0);
  preferences_.end();

  if (brightness_ < 1) brightness_ = 127;
  if (dimBrightness_ < 1) dimBrightness_ = 32;
  if (dimBrightness_ > brightness_) dimBrightness_ = brightness_;
  if (dimAfterSeconds_ < 5 || dimAfterSeconds_ > 3600) dimAfterSeconds_ = 60;
  if (offAfterSeconds_ > 86400UL) offAfterSeconds_ = 0;
  layout_ = layoutValue <= static_cast<uint8_t>(DisplayLayout::Clock)
              ? static_cast<DisplayLayout>(layoutValue)
              : DisplayLayout::Standard;
}

bool DisplayService::probe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool DisplayService::command(uint8_t commandValue) {
  if (!present_ || !address_) return false;
  Wire.beginTransmission(address_);
  Wire.write(static_cast<uint8_t>(0x00));
  Wire.write(commandValue);
  return Wire.endTransmission() == 0;
}

bool DisplayService::setContrast(uint8_t value) {
  return command(0x81) && command(value);
}

bool DisplayService::applyOrientation() {
  if (!present_) return false;
  const bool segmentOk = command(rotation180_ ? 0xA0 : 0xA1);
  const bool scanOk = command(rotation180_ ? 0xC0 : 0xC8);
  const bool invertOk = command(inverted_ ? 0xA7 : 0xA6);
  return segmentOk && scanOk && invertOk;
}

bool DisplayService::initializeController() {
  const uint8_t commands[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0xAD, 0x8B, 0xDA, 0x12, 0xD9, 0x22, 0xDB, 0x35, 0xA4
  };
  for (size_t i = 0; i < sizeof(commands); i++) {
    if (!command(commands[i])) return false;
  }
  if (!applyOrientation()) return false;
  if (!setContrast(brightness_)) return false;
  return command(0xAF);
}

void DisplayService::setPage(uint8_t page) {
  command(static_cast<uint8_t>(0xB0 | (page & 0x07)));
  command(0x02);
  command(0x10);
}

void DisplayService::writeData(const uint8_t* data, size_t length) {
  size_t pos = 0;
  while (pos < length) {
    const size_t chunk = min(static_cast<size_t>(16), length - pos);
    Wire.beginTransmission(address_);
    Wire.write(static_cast<uint8_t>(0x40));
    for (size_t i = 0; i < chunk; i++) Wire.write(data[pos + i]);
    Wire.endTransmission();
    pos += chunk;
  }
}

void DisplayService::flush() {
  if (!present_ || !active_) return;
  for (uint8_t page = 0; page < 8; page++) {
    setPage(page);
    writeData(framebuffer_ + static_cast<size_t>(page) * WIDTH, WIDTH);
  }
  lastFrameAt_ = millis();
}

void DisplayService::glyph(char value, uint8_t out[5]) {
  memset(out, 0, 5);
  if (value >= '0' && value <= '9') {
    memcpy(out, FONT_DIGITS[value - '0'], 5);
    return;
  }
  if (value >= 'a' && value <= 'z') value = static_cast<char>(value - 'a' + 'A');
  if (value >= 'A' && value <= 'Z') {
    memcpy(out, FONT_UPPER[value - 'A'], 5);
    return;
  }
  switch (value) {
    case ':': out[1] = 0x36; out[2] = 0x36; break;
    case '-': for (uint8_t i = 0; i < 5; i++) out[i] = 0x08; break;
    case '.': out[1] = 0x60; out[2] = 0x60; break;
    case '/': out[0] = 0x60; out[1] = 0x18; out[2] = 0x06; out[3] = 0x01; break;
    case '_': for (uint8_t i = 0; i < 5; i++) out[i] = 0x40; break;
    default: break;
  }
}

void DisplayService::clearBuffer() {
  memset(framebuffer_, 0, sizeof(framebuffer_));
}

void DisplayService::pixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
  const size_t index = static_cast<size_t>(y / 8) * WIDTH + x;
  const uint8_t mask = static_cast<uint8_t>(1U << (y & 7));
  if (on) framebuffer_[index] |= mask;
  else framebuffer_[index] &= static_cast<uint8_t>(~mask);
}

void DisplayService::drawChar(int16_t x, int16_t y, char value, uint8_t scale) {
  uint8_t columns[5] = {0};
  glyph(value, columns);
  scale = scale < 1 ? 1 : scale;

  for (uint8_t col = 0; col < 5; col++) {
    for (uint8_t row = 0; row < 7; row++) {
      if ((columns[col] & (1U << row)) == 0) continue;
      for (uint8_t dx = 0; dx < scale; dx++) {
        for (uint8_t dy = 0; dy < scale; dy++) {
          pixel(x + col * scale + dx, y + row * scale + dy);
        }
      }
    }
  }
}

void DisplayService::drawText(int16_t x, int16_t y, const String& value, uint8_t scale) {
  const int16_t advance = static_cast<int16_t>(6 * scale);
  for (size_t i = 0; i < value.length(); i++) {
    if (x + 5 * scale >= WIDTH) break;
    drawChar(x, y, value[i], scale);
    x += advance;
  }
}

void DisplayService::drawHLine(int16_t x, int16_t y, int16_t width) {
  for (int16_t i = 0; i < width; i++) pixel(x + i, y);
}

String DisplayService::timeText() const {
  return time_ && time_->valid() ? time_->localTime() : String("--:--:--");
}

String DisplayService::shortDate() const {
  if (!time_ || !time_->valid()) return "--.--.----";
  return time_->localDate();
}

String DisplayService::rtcText() const {
  if (!rtc_ || !rtc_->present()) return "RTC FEHLT";
  return rtc_->timeValid() ? "RTC OK" : "RTC ZEIT?";
}

String DisplayService::networkText() const {
  if (currentAutarkMode_) return "AUTARK";
  if (network_ && network_->connected()) return "WLAN OK";
  if (network_ && network_->accessPointActive()) return "AP AKTIV";
  return "WLAN OFF";
}

void DisplayService::renderFrame(bool autarkMode, ScreenMode mode) {
  currentAutarkMode_ = autarkMode;
  clearBuffer();
  if (mode == ScreenMode::Boot) renderBoot(autarkMode);
  else if (mode == ScreenMode::Test) renderTest(autarkMode);
  else renderLive(autarkMode);
  frameRevision_++;
}

void DisplayService::renderBoot(bool autarkMode) {
  drawText(2, 2, autarkMode ? "AUTARKMODUS" : "SYSTEMSTART");
  drawHLine(0, 11, WIDTH);
  drawText(2, 17, rtcText());
  drawText(2, 29, String("ZEIT ") + timeText());
  drawText(2, 41, networkText());
  drawText(2, 53, autarkMode ? "AUTARK BEREIT" : "SYSTEM BEREIT");
}

void DisplayService::renderTest(bool autarkMode) {
  drawText(2, 2, "DISPLAY TEST");
  drawHLine(0, 11, WIDTH);
  drawText(2, 17, "128 X 64 PIXEL");
  drawText(2, 29, String("HELL ") + brightness_ + " DIM " + dimBrightness_);
  drawText(2, 41, rotation180_ ? "ROTATION 180" : "ROTATION 0");
  drawText(2, 53, autarkMode ? "MODUS AUTARK" : "MODUS NORMAL");
}

void DisplayService::renderLive(bool autarkMode) {
  switch (layout_) {
    case DisplayLayout::Compact: renderCompact(autarkMode); break;
    case DisplayLayout::Clock: renderClock(autarkMode); break;
    default: renderStandard(autarkMode); break;
  }
}

void DisplayService::renderStandard(bool autarkMode) {
  const uint32_t count = storage_ ? storage_->recentCount() : 0;
  drawText(2, 1, "UNTERBRECHUNGEN");
  drawHLine(0, 10, WIDTH);
  drawText(2, 15, String("ZEIT  ") + timeText());
  drawText(2, 27, String("GESAMT ") + count);
  drawText(2, 39, rtcText());
  drawText(2, 51, autarkMode ? "AUTARK" : networkText());
}

void DisplayService::renderCompact(bool autarkMode) {
  const uint32_t count = storage_ ? storage_->recentCount() : 0;
  drawText(2, 1, timeText());
  drawText(62, 1, shortDate());
  drawHLine(0, 11, WIDTH);
  drawText(2, 17, String("UNTERBR ") + count);
  drawText(2, 29, rtcText());
  drawText(2, 41, autarkMode ? "AUTARK" : networkText());
  drawText(2, 53, dimmed_ ? "DISPLAY GEDIMMT" : "DISPLAY AKTIV");
}

void DisplayService::renderClock(bool autarkMode) {
  const uint32_t count = storage_ ? storage_->recentCount() : 0;
  String clock = timeText();
  if (clock.length() > 5) clock = clock.substring(0, 5);
  drawText(15, 3, clock, 2);
  drawHLine(0, 21, WIDTH);
  drawText(34, 27, shortDate());
  drawText(2, 40, String("EREIGNISSE ") + count);
  drawText(2, 52, autarkMode ? "AUTARK" : networkText());
}

void DisplayService::armActivityTimers(bool autarkMode) {
  currentAutarkMode_ = autarkMode;
  const uint32_t now = millis();
  dimAt_ = now + static_cast<uint32_t>(dimAfterSeconds_) * 1000UL;

  // Im Autarkbetrieb bleibt die feste 15-s-Abschaltung bestehen. Im
  // Normalbetrieb ist 0 = nie ganz ausschalten.
  if (autarkMode) offAt_ = now + UicConfig::DISPLAY_BOOT_MS;
  else offAt_ = offAfterSeconds_ ? now + offAfterSeconds_ * 1000UL : 0;
}
