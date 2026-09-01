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
  if (present_) refreshState();
  Serial.printf("[RTC] DS3231 %s | Zeit %s\n", present_ ? "erkannt" : "nicht erkannt", timeValid_ ? "OK" : "ungueltig");
}

void RtcService::tick() {
  if (!present_) return;
  if (lastCheckAt_ != 0 && millis() - lastCheckAt_ < UicConfig::RTC_DIAGNOSTIC_INTERVAL_MS) return;
  refreshState();
}

uint32_t RtcService::lastCheckAgeMs() const {
  if (lastCheckAt_ == 0) return 0xFFFFFFFFUL;
  return millis() - lastCheckAt_;
}

bool RtcService::applyToSystem() {
  if (!present_) return false;
  if (!cachedTimeAvailable_ && !refreshState()) return false;
  if (oscillatorStopFlag_ || !timeValid_) return false;

  struct tm value = cachedTime_;
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
    return false;
  }

  uint8_t status = 0;
  if (readRegister(0x0F, status)) writeRegister(0x0F, status & static_cast<uint8_t>(~0x80));
  return refreshState();
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
  if (!cachedTimeAvailable_) return "-";
  char text[16];
  snprintf(text, sizeof(text), "%02d.%02d.%04d", cachedTime_.tm_mday, cachedTime_.tm_mon + 1, cachedTime_.tm_year + 1900);
  return String(text);
}

String RtcService::timeText() const {
  if (!cachedTimeAvailable_) return "-";
  char text[16];
  snprintf(text, sizeof(text), "%02d:%02d:%02d", cachedTime_.tm_hour, cachedTime_.tm_min, cachedTime_.tm_sec);
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

bool RtcService::readTemperature(float& value) {
  uint8_t msb = 0;
  uint8_t lsb = 0;
  if (!readRegister(0x11, msb) || !readRegister(0x12, lsb)) return false;
  const int8_t signedMsb = static_cast<int8_t>(msb);
  value = static_cast<float>(signedMsb) + static_cast<float>((lsb >> 6) & 0x03) * 0.25f;
  return true;
}

void RtcService::updateDifference() {
  if (!cachedTimeAvailable_) {
    systemDifferenceSeconds_ = 0;
    return;
  }
  const time_t systemNow = time(nullptr);
  if (systemNow <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) {
    systemDifferenceSeconds_ = 0;
    return;
  }
  struct tm rtcValue = cachedTime_;
  const time_t rtcEpoch = mktime(&rtcValue);
  if (rtcEpoch <= 0) {
    systemDifferenceSeconds_ = 0;
    return;
  }
  const int64_t diff = static_cast<int64_t>(rtcEpoch) - static_cast<int64_t>(systemNow);
  systemDifferenceSeconds_ = diff > INT32_MAX ? INT32_MAX : (diff < INT32_MIN ? INT32_MIN : static_cast<int32_t>(diff));
}

bool RtcService::refreshState() {
  lastCheckAt_ = millis();
  if (!present_) {
    timeValid_ = false;
    oscillatorStopFlag_ = false;
    communicationOk_ = false;
    cachedTimeAvailable_ = false;
    return false;
  }

  struct tm value = {};
  bool osf = false;
  const bool timeOk = read(value, osf);
  float temp = NAN;
  const bool tempOk = readTemperature(temp);
  communicationOk_ = timeOk && tempOk;
  oscillatorStopFlag_ = osf;
  timeValid_ = timeOk && !osf;
  if (timeOk) {
    cachedTime_ = value;
    cachedTimeAvailable_ = true;
  }
  if (tempOk) temperatureC_ = temp;
  updateDifference();
  return communicationOk_;
}
