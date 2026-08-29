#include "DisplayService.h"

#include <Wire.h>

#include "Config.h"
#include "RtcService.h"
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
}

void DisplayService::begin(RtcService* rtc, TimeService* time) {
  rtc_ = rtc;
  time_ = time;

  if (probe(UicConfig::OLED_ADDRESS_1)) {
    present_ = true;
    address_ = UicConfig::OLED_ADDRESS_1;
  } else if (probe(UicConfig::OLED_ADDRESS_2)) {
    present_ = true;
    address_ = UicConfig::OLED_ADDRESS_2;
  }

  Serial.printf("[DISPLAY] SH1106 %s", present_ ? "erkannt" : "nicht erkannt");
  if (present_) Serial.printf(" | Adresse 0x%02X", address_);
  Serial.println();
}

void DisplayService::tick() {
  if (!active_ || offAt_ == 0) return;
  if (static_cast<int32_t>(millis() - offAt_) >= 0) off();
}

void DisplayService::showBootStatus(bool autarkMode) {
  render(autarkMode, false);
}

bool DisplayService::showTest(bool autarkMode) {
  if (!present_) return false;
  render(autarkMode, true);
  return active_;
}

void DisplayService::off() {
  if (!present_) return;
  command(0xAE);
  active_ = false;
  offAt_ = 0;
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

bool DisplayService::initializeController() {
  const uint8_t commands[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0xAD, 0x8B, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
    0xD9, 0x22, 0xDB, 0x35, 0xA4, 0xA6, 0xAF
  };
  for (size_t i = 0; i < sizeof(commands); i++) {
    if (!command(commands[i])) return false;
  }
  return true;
}

void DisplayService::clear() {
  const uint8_t zeros[16] = {0};
  for (uint8_t page = 0; page < 8; page++) {
    setPage(page);
    for (uint8_t block = 0; block < 8; block++) writeData(zeros, sizeof(zeros));
  }
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
    default: break;
  }
}

void DisplayService::text(uint8_t page, const String& value) {
  setPage(page);
  const size_t count = min(static_cast<size_t>(21), value.length());
  for (size_t i = 0; i < count; i++) {
    uint8_t columns[6] = {0};
    glyph(value[i], columns);
    writeData(columns, sizeof(columns));
  }
  const uint8_t blank[6] = {0};
  for (size_t i = count; i < 21; i++) writeData(blank, sizeof(blank));
}

void DisplayService::render(bool autarkMode, bool testMode) {
  if (!present_ || !initializeController()) return;
  clear();
  text(0, testMode ? "DISPLAY TEST" : (autarkMode ? "AUTARKMODUS" : "SYSTEMSTART"));

  if (!rtc_ || !rtc_->present()) text(2, "RTC: NICHT ERKANNT");
  else if (!rtc_->timeValid()) text(2, "RTC: ZEIT FEHLT");
  else text(2, "RTC: ZEIT OK");

  String line = "ZEIT: ";
  line += time_ && time_->valid() ? time_->localTime() : "--:--:--";
  text(4, line);
  text(6, autarkMode ? "AUTARK BEREIT" : "SYSTEM BEREIT");
  command(0xAF);
  active_ = true;
  offAt_ = millis() + UicConfig::DISPLAY_BOOT_MS;
}
