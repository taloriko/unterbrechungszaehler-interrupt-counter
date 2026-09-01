#include "RtcService.h"

#include <math.h>
#include <sys/time.h>
#include <time.h>

#include "Config.h"

void RtcService::begin() {
  Wire.begin(UicConfig::I2C_SDA, UicConfig::I2C_SCL);
  Wire.setClock(100000);
  delay(5);

  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  present_ = Wire.endTransmission() == 0;
  if (present_) sample();
  Serial.printf("[RTC] DS3231 %s | Zeit %s\n", present_ ? "erkannt" : "nicht erkannt", timeValid_ ? "OK" : "ungueltig");
}

bool RtcService::checkDue() const {
  if (!present_) return false;
  if (lastCheckAtMs_ == 0) return true;
  return millis() - lastCheckAtMs_ >= UicConfig::RTC_CHECK_INTERVAL_MS;
}

bool RtcService::tick() {
  if (!checkDue()) return communicationOk_;
  return sample();
}

uint32_t RtcService::lastCheckAgeMs() const {
  if (lastCheckAtMs_ == 0) return 0xFFFFFFFFUL;
  return millis() - lastCheckAtMs_;
}

bool RtcService::applyToSystem() {
  struct tm value = {};
  if (!cachedLocalTime(value) || oscillatorStopFlag_) return false;

  setenv("TZ", UicConfig::TZ_INFO, 1);
  tzset();
  const time_t epochValue = mktime(&value);
  if (epochValue <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) return false;

  struct timeval tv = {};
  tv.tv_sec = epochValue;
  return settimeofday(&tv, nullptr) == 0;
}

bool RtcService::writeSystemTime() {
  if (!present_) return false;
  const time_t now = time(nullptr);
  if (now <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) return false;

  struct tm value = {};
  localtime_r(&now, &value);

  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  Wire.write(static_cast<uint8_t>(0x00));
  Wire.write(decToBcd(static_cast<uint8_t>(value.tm_sec)));
  Wire.write(decToBcd(static_cast<uint8_t>(value.tm_min)));
  Wire.write(decToBcd(static_cast<uint8_t>(value.tm_hour)));
  const uint8_t dow = value.tm_wday == 0 ? 7 : static_cast<uint8_t>(value.tm_wday);
  Wire.write(decToBcd(dow));
  Wire.write(decToBcd(static_cast<uint8_t>(value.tm_mday)));
  Wire.write(decToBcd(static_cast<uint8_t>(value.tm_mon + 1)));
  Wire.write(decToBcd(static_cast<uint8_t>((value.tm_year + 1900) % 100)));
  if (Wire.endTransmission() != 0) {
    communicationOk_ = false;
    lastCheckAtMs_ = millis();
    return false;
  }

  uint8_t status = 0;
  if (readRegister(0x0F, status)) {
    writeRegister(0x0F, status & static_cast<uint8_t>(~0x80));
  }
  return sample();
}

bool RtcService::read(struct tm& value, bool& oscillatorStopFlag) {
  if (!present_) return false;

  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  Wire.write(static_cast<uint8_t>(0x00));
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(UicConfig::RTC_ADDRESS, static_cast<uint8_t>(7)) != 7) return false;

  const uint8_t secReg = Wire.read();
  const uint8_t minReg = Wire.read();
  const uint8_t hourReg = Wire.read();
  Wire.read();
  const uint8_t dayReg = Wire.read();
  const uint8_t monthReg = Wire.read();
  const uint8_t yearReg = Wire.read();

  int hour = 0;
  if (hourReg & 0x40) {
    hour = bcdToDec(hourReg & 0x1F);
    const bool pm = (hourReg & 0x20) != 0;
    if (hour == 12) hour = 0;
    if (pm) hour += 12;
  } else {
    hour = bcdToDec(hourReg & 0x3F);
  }

  const int second = bcdToDec(secReg & 0x7F);
  const int minute = bcdToDec(minReg & 0x7F);
  const int day = bcdToDec(dayReg & 0x3F);
  const int month = bcdToDec(monthReg & 0x1F);
  int year = 2000 + bcdToDec(yearReg);
  if (monthReg & 0x80) year += 100;

  uint8_t status = 0x80;
  if (!readRegister(0x0F, status)) return false;
  oscillatorStopFlag = (status & 0x80) != 0;

  if (second > 59 || minute > 59 || hour > 23 || day < 1 || day > 31 ||
      month < 1 || month > 12 || year < 2024 || year > 2199) return false;

  memset(&value, 0, sizeof(value));
  value.tm_sec = second;
  value.tm_min = minute;
  value.tm_hour = hour;
  value.tm_mday = day;
  value.tm_mon = month - 1;
  value.tm_year = year - 1900;
  value.tm_isdst = -1;
  return true;
}

String RtcService::dateText() const {
  struct tm value = {};
  if (!cachedLocalTime(value)) return "-";
  char text[16];
  snprintf(text, sizeof(text), "%02d.%02d.%04d", value.tm_mday, value.tm_mon + 1, value.tm_year + 1900);
  return String(text);
}

String RtcService::timeText() const {
  struct tm value = {};
  if (!cachedLocalTime(value)) return "-";
  char text[16];
  snprintf(text, sizeof(text), "%02d:%02d:%02d", value.tm_hour, value.tm_min, value.tm_sec);
  return String(text);
}

uint8_t RtcService::bcdToDec(uint8_t value) const {
  return static_cast<uint8_t>((value >> 4) * 10 + (value & 0x0F));
}

uint8_t RtcService::decToBcd(uint8_t value) const {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

bool RtcService::readRegister(uint8_t reg, uint8_t& value) {
  if (!present_) return false;
  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(UicConfig::RTC_ADDRESS, static_cast<uint8_t>(1)) != 1) return false;
  value = Wire.read();
  return true;
}

bool RtcService::writeRegister(uint8_t reg, uint8_t value) {
  if (!present_) return false;
  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool RtcService::sample() {
  lastCheckAtMs_ = millis();
  struct tm value = {};
  bool osf = false;
  if (!read(value, osf)) {
    communicationOk_ = false;
    timeValid_ = false;
    return false;
  }

  setenv("TZ", UicConfig::TZ_INFO, 1);
  tzset();
  const time_t epochValue = mktime(&value);
  if (epochValue <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) {
    communicationOk_ = true;
    oscillatorStopFlag_ = osf;
    timeValid_ = false;
    return false;
  }

  uint8_t tempMsb = 0;
  uint8_t tempLsb = 0;
  if (readRegister(0x11, tempMsb) && readRegister(0x12, tempLsb)) {
    const int8_t signedMsb = static_cast<int8_t>(tempMsb);
    temperatureC_ = static_cast<float>(signedMsb) + static_cast<float>((tempLsb >> 6) & 0x03) * 0.25f;
  }

  communicationOk_ = true;
  oscillatorStopFlag_ = osf;
  timeValid_ = !osf;
  cachedEpoch_ = epochValue;
  cachedAtMs_ = millis();
  return true;
}

bool RtcService::cachedLocalTime(struct tm& value) const {
  if (!communicationOk_ || cachedEpoch_ <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) return false;
  const time_t current = cachedEpoch_ + static_cast<time_t>((millis() - cachedAtMs_) / 1000UL);
  localtime_r(&current, &value);
  return true;
}
