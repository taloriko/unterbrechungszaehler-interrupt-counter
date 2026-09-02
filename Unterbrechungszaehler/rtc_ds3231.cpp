#include "rtc_ds3231.h"

#include "hardware_config.h"
#include "i2c_bus.h"
#include "serial_log.h"
#include "status_registry.h"
#include "time_utils.h"

namespace RtcDs3231 {
namespace {

StatusRegistry::State moduleHealth = StatusRegistry::State::Unknown;
bool isDetected = false;
bool osf = false;
float tempC = 0.0f;
uint32_t checkedAtMs = 0;
uint64_t checkedAtMonotonicMs = 0;
const char *errorText = "";
HardwareTypes::DateTimeValue cachedTime;

bool validBcd(uint8_t value) {
  return (value & 0x0FU) <= 9U && ((value >> 4) & 0x0FU) <= 9U;
}

uint8_t bcdToDec(uint8_t value) { return static_cast<uint8_t>((value >> 4) * 10 + (value & 0x0F)); }
uint8_t decToBcd(uint8_t value) { return static_cast<uint8_t>(((value / 10) << 4) | (value % 10)); }

bool validDateTime(const HardwareTypes::DateTimeValue &value) {
  if (value.year < 2000 || value.year > 2199) return false;
  int64_t epochSeconds = 0;
  return TimeUtils::dateTimeToEpochUtc(value, epochSeconds);
}

void setHealth(StatusRegistry::State state, const char *message = "") {
  moduleHealth = state;
  errorText = message ? message : "";
  StatusRegistry::setState("rtc", state);
}

bool readRegisters() {
  uint8_t clock[7]{};
  if (!I2cBus::readRegisters(HardwareConfig::RTC_DS3231_ADDRESS, 0x00, clock, sizeof(clock))) return false;

  const uint8_t rawSecond = static_cast<uint8_t>(clock[0] & 0x7F);
  const uint8_t rawMinute = static_cast<uint8_t>(clock[1] & 0x7F);
  const uint8_t rawHour = static_cast<uint8_t>(clock[2] & ((clock[2] & 0x40U) ? 0x1FU : 0x3FU));
  const uint8_t rawDay = static_cast<uint8_t>(clock[4] & 0x3F);
  const uint8_t rawMonth = static_cast<uint8_t>(clock[5] & 0x1F);
  if (!validBcd(rawSecond) || !validBcd(rawMinute) || !validBcd(rawHour) ||
      !validBcd(rawDay) || !validBcd(rawMonth) || !validBcd(clock[6])) {
    return false;
  }

  HardwareTypes::DateTimeValue value;
  value.second = bcdToDec(rawSecond);
  value.minute = bcdToDec(rawMinute);

  if (clock[2] & 0x40) {
    uint8_t hour = bcdToDec(clock[2] & 0x1F);
    const bool pm = (clock[2] & 0x20) != 0;
    if (hour == 12) hour = 0;
    value.hour = static_cast<uint8_t>(hour + (pm ? 12 : 0));
  } else {
    value.hour = bcdToDec(clock[2] & 0x3F);
  }

  value.weekday = static_cast<uint8_t>(clock[3] & 0x07);
  if (value.weekday < 1 || value.weekday > 7) return false;
  value.day = bcdToDec(rawDay);
  value.month = bcdToDec(rawMonth);
  value.year = static_cast<uint16_t>(2000 + bcdToDec(clock[6]) + ((clock[5] & 0x80) ? 100 : 0));
  value.valid = validDateTime(value);
  cachedTime = value;

  uint8_t status = 0;
  if (!I2cBus::readRegisters(HardwareConfig::RTC_DS3231_ADDRESS, 0x0F, &status, 1)) return false;
  osf = (status & 0x80) != 0;

  uint8_t temperature[2]{};
  if (!I2cBus::readRegisters(HardwareConfig::RTC_DS3231_ADDRESS, 0x11, temperature, sizeof(temperature))) return false;
  const int8_t integerPart = static_cast<int8_t>(temperature[0]);
  tempC = static_cast<float>(integerPart) + static_cast<float>((temperature[1] >> 6) & 0x03) * 0.25f;
  return true;
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("rtc", "status.rtc", "clock", HardwareConfig::ENABLE_RTC_DS3231);
  if (!HardwareConfig::ENABLE_RTC_DS3231) {
    setHealth(StatusRegistry::State::Disabled);
    StatusRegistry::setVisible("rtc", false);
    return false;
  }

  I2cBus::begin();
  probe();
  return isDetected;
}

void probe() {
  if (!HardwareConfig::ENABLE_RTC_DS3231) {
    setHealth(StatusRegistry::State::Disabled);
    return;
  }

  setHealth(StatusRegistry::State::Checking);
  isDetected = I2cBus::probe(HardwareConfig::RTC_DS3231_ADDRESS);
  checkedAtMs = millis();
  checkedAtMonotonicMs = TimeUtils::monotonicMs();
  if (!isDetected) {
    cachedTime = HardwareTypes::DateTimeValue{};
    setHealth(StatusRegistry::State::NoResponse, "no I2C response");
    SerialLog::errorf("RTC", "DS3231: NO RESPONSE | address=0x%02X", HardwareConfig::RTC_DS3231_ADDRESS);
    return;
  }

  if (!readRegisters()) {
    // A partial I2C transaction must never leave a previously/partially read
    // calendar value eligible as a trusted time source. Health and time
    // validity deliberately fail closed here.
    cachedTime.valid = false;
    setHealth(StatusRegistry::State::Error, "register read failed");
    SerialLog::error("RTC", "DS3231 detected but register read failed");
    return;
  }

  if (osf || !cachedTime.valid) {
    setHealth(StatusRegistry::State::Warning, osf ? "oscillator stop flag set" : "invalid calendar data");
    SerialLog::warningf("RTC", "DS3231: WARNING | OSF=%s | time-valid=%s | temp=%.2f C",
                        osf ? "yes" : "no", cachedTime.valid ? "yes" : "no", tempC);
  } else {
    setHealth(StatusRegistry::State::Ok);
    SerialLog::successf("RTC", "DS3231: OK | %04u-%02u-%02u %02u:%02u:%02u | temp=%.2f C",
                        cachedTime.year, cachedTime.month, cachedTime.day,
                        cachedTime.hour, cachedTime.minute, cachedTime.second, tempC);
  }
}

bool enabled() { return HardwareConfig::ENABLE_RTC_DS3231; }
bool detected() { return isDetected; }
StatusRegistry::State health() { return moduleHealth; }
uint32_t lastCheckMs() { return checkedAtMs; }
uint64_t lastCheckMonotonicMs() { return checkedAtMonotonicMs; }
const char *lastError() { return errorText; }
HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::TransportAck; }
bool oscillatorStopFlag() { return osf; }
float temperatureC() { return tempC; }
const HardwareTypes::DateTimeValue &dateTime() { return cachedTime; }

bool setDateTime(const HardwareTypes::DateTimeValue &value) {
  if (!HardwareConfig::ENABLE_RTC_DS3231 || !validDateTime(value)) return false;
  uint8_t data[7] = {
      decToBcd(value.second), decToBcd(value.minute), decToBcd(value.hour),
      decToBcd(value.weekday ? value.weekday : 1), decToBcd(value.day),
      decToBcd(value.month), decToBcd(static_cast<uint8_t>(value.year % 100))};
  if (value.year >= 2100) data[5] |= 0x80;

  if (!I2cBus::writeRegister(HardwareConfig::RTC_DS3231_ADDRESS, 0x00, data, sizeof(data))) {
    setHealth(StatusRegistry::State::Error, "time write failed");
    return false;
  }

  uint8_t status = 0;
  if (I2cBus::readRegisters(HardwareConfig::RTC_DS3231_ADDRESS, 0x0F, &status, 1)) {
    status &= static_cast<uint8_t>(~0x80);
    I2cBus::writeRegister(HardwareConfig::RTC_DS3231_ADDRESS, 0x0F, &status, 1);
  }
  probe();
  // A remaining warning (for example OSF could not be cleared) means the write
  // was not fully verified and must not be reported as confirmed success.
  return moduleHealth == StatusRegistry::State::Ok;
}

}  // namespace RtcDs3231
