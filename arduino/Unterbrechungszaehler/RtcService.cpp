#include "RtcService.h"

#include <sys/time.h>
#include <time.h>

#include "Config.h"

void RtcService::begin() {
  Wire.begin(UicConfig::I2C_SDA, UicConfig::I2C_SCL);
  Wire.setClock(100000);
  delay(5);

  setenv("TZ", UicConfig::TZ_INFO, 1);
  tzset();

  refreshState();
  if (present_) configureOutputs();

  Serial.printf("[RTC] DS3231 %s | Zeit %s | Temp %s\n",
                present_ ? "erkannt" : "nicht erkannt",
                timeValid_ ? "OK" : "ungueltig",
                temperatureValid_ ? String(temperatureC_, 2).c_str() : "-");
}

void RtcService::tick() {
  if (lastCheckAt_ != 0 && millis() - lastCheckAt_ < UicConfig::RTC_HEALTH_INTERVAL_MS) return;
  refreshState();
}

bool RtcService::applyToSystem() {
  struct tm value = {};
  bool osf = false;
  if (!read(value, osf) || osf) return false;

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
  if (Wire.endTransmission() != 0) return false;

  uint8_t status = 0;
  if (readRegister(0x0F, status)) {
    writeRegister(0x0F, status & static_cast<uint8_t>(~0x80));
  }

  return refreshState();
}

bool RtcService::read(struct tm& value, bool& oscillatorStopFlag) {
  oscillatorStopFlag = oscillatorStopFlag_;
  return cachedTime(value);
}

String RtcService::dateText() const {
  struct tm value = {};
  if (!cachedTime(value)) return "-";
  char text[16];
  snprintf(text, sizeof(text), "%02d.%02d.%04d", value.tm_mday, value.tm_mon + 1, value.tm_year + 1900);
  return String(text);
}

String RtcService::timeText() const {
  struct tm value = {};
  if (!cachedTime(value)) return "-";
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

bool RtcService::probe() {
  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  return Wire.endTransmission() == 0;
}

bool RtcService::readHardware(struct tm& value, bool& oscillatorStopFlag, float& temperatureC) {
  if (!present_) return false;

  // Ein Burst von 0x00 bis 0x12 liefert Uhrzeit, Status und Temperatur.
  // Damit braucht der regulaere Healthcheck nur einen I2C-Lesezugriff.
  Wire.beginTransmission(UicConfig::RTC_ADDRESS);
  Wire.write(static_cast<uint8_t>(0x00));
  if (Wire.endTransmission(false) != 0) return false;

  constexpr uint8_t registerCount = 19;
  if (Wire.requestFrom(UicConfig::RTC_ADDRESS, registerCount) != registerCount) return false;

  uint8_t reg[registerCount];
  for (uint8_t i = 0; i < registerCount; i++) reg[i] = Wire.read();

  const uint8_t secReg = reg[0];
  const uint8_t minReg = reg[1];
  const uint8_t hourReg = reg[2];
  const uint8_t dayReg = reg[4];
  const uint8_t monthReg = reg[5];
  const uint8_t yearReg = reg[6];

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

  oscillatorStopFlag = (reg[15] & 0x80) != 0;

  const int8_t tempWhole = static_cast<int8_t>(reg[17]);
  const uint8_t tempFraction = static_cast<uint8_t>(reg[18] >> 6);
  temperatureC = static_cast<float>(tempWhole) + static_cast<float>(tempFraction) * 0.25f;

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

bool RtcService::refreshState() {
  const uint32_t startedUs = micros();

  if (!present_) present_ = probe();

  struct tm value = {};
  bool osf = false;
  float temperature = 0.0f;
  const bool ok = present_ && readHardware(value, osf, temperature);

  if (!ok) {
    present_ = false;
    timeValid_ = false;
    lastCheckOk_ = false;
    temperatureValid_ = false;
    systemOffsetValid_ = false;
  } else {
    oscillatorStopFlag_ = osf;
    temperatureC_ = temperature;
    temperatureValid_ = true;

    const time_t rtcEpoch = mktime(&value);
    if (rtcEpoch > static_cast<time_t>(UicConfig::VALID_TIME_MIN)) {
      cachedEpoch_ = rtcEpoch;
      cacheAtMs_ = millis();
      timeValid_ = !osf;
      lastCheckOk_ = true;
      updateSystemOffset();
    } else {
      timeValid_ = false;
      lastCheckOk_ = false;
      systemOffsetValid_ = false;
    }
  }

  lastCheckAt_ = millis();
  lastCheckDurationUs_ = micros() - startedUs;
  checkSequence_++;
  return lastCheckOk_;
}

bool RtcService::cachedTime(struct tm& value) const {
  if (cachedEpoch_ <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) return false;
  const time_t estimated = cachedEpoch_ + static_cast<time_t>((millis() - cacheAtMs_) / 1000UL);
  localtime_r(&estimated, &value);
  return true;
}

void RtcService::configureOutputs() {
  uint8_t control = 0;
  if (readRegister(0x0E, control)) {
    // Alarm-Interrupts und batteriegepufferte Square-Wave-Ausgabe sind ungenutzt.
    control &= static_cast<uint8_t>(~0x43);
    control |= 0x04;  // INTCN: SQW-Ausgang deaktiviert, solange kein Alarm aktiv ist.
    writeRegister(0x0E, control);
  }

  uint8_t status = 0;
  if (readRegister(0x0F, status)) {
    // 32-kHz-Ausgang wird im Projekt nicht verwendet. OSF bleibt dabei unangetastet.
    writeRegister(0x0F, status & static_cast<uint8_t>(~0x08));
  }
}

void RtcService::updateSystemOffset() {
  const time_t now = time(nullptr);
  if (now <= static_cast<time_t>(UicConfig::VALID_TIME_MIN) ||
      cachedEpoch_ <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) {
    systemOffsetValid_ = false;
    return;
  }

  const time_t estimatedRtc = cachedEpoch_ + static_cast<time_t>((millis() - cacheAtMs_) / 1000UL);
  systemOffsetSeconds_ = static_cast<int32_t>(estimatedRtc - now);
  systemOffsetValid_ = true;
}
