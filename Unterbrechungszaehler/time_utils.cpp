#include "time_utils.h"

#include <esp_timer.h>
#include <time.h>

namespace TimeUtils {
namespace {

bool leapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t daysInMonth(int year, int month) {
  static const uint8_t DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && leapYear(year)) return 29;
  return DAYS[month - 1];
}

// Days since 1970-01-01. This civil-calendar conversion keeps RTC handling
// independent from the process timezone and avoids temporarily modifying TZ.
int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned mp = month > 2 ? month - 3U : month + 9U;
  const unsigned doy = (153U * mp + 2U) / 5U + day - 1U;
  const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
  return static_cast<int64_t>(era) * 146097LL + static_cast<int64_t>(doe) - 719468LL;
}

}  // namespace

uint64_t monotonicMs() {
  return static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
}

bool isPlausibleDateTime(const HardwareTypes::DateTimeValue &value, uint16_t minYear, uint16_t maxYear) {
  if (!value.valid || value.year < minYear || value.year > maxYear) return false;
  if (value.month < 1 || value.month > 12 || value.day < 1 || value.day > daysInMonth(value.year, value.month)) return false;
  return value.hour <= 23 && value.minute <= 59 && value.second <= 59;
}

bool dateTimeToEpochUtc(const HardwareTypes::DateTimeValue &value, int64_t &epochSeconds) {
  if (value.year < 1970 || value.month < 1 || value.month > 12 || value.day < 1 || value.day > daysInMonth(value.year, value.month)) return false;
  if (value.hour > 23 || value.minute > 59 || value.second > 59) return false;
  const int64_t days = daysFromCivil(value.year, value.month, value.day);
  epochSeconds = days * 86400LL + static_cast<int64_t>(value.hour) * 3600LL +
                 static_cast<int64_t>(value.minute) * 60LL + value.second;
  return true;
}

bool epochUtcToDateTime(int64_t epochSeconds, HardwareTypes::DateTimeValue &value) {
  if (epochSeconds < 0) return false;
  const time_t raw = static_cast<time_t>(epochSeconds);
  struct tm utc {};
  if (!gmtime_r(&raw, &utc)) return false;
  value.year = static_cast<uint16_t>(utc.tm_year + 1900);
  value.month = static_cast<uint8_t>(utc.tm_mon + 1);
  value.day = static_cast<uint8_t>(utc.tm_mday);
  value.weekday = static_cast<uint8_t>(utc.tm_wday == 0 ? 7 : utc.tm_wday);
  value.hour = static_cast<uint8_t>(utc.tm_hour);
  value.minute = static_cast<uint8_t>(utc.tm_min);
  value.second = static_cast<uint8_t>(utc.tm_sec);
  value.valid = true;
  return true;
}

bool isPlausibleEpochMs(int64_t epochMs, uint16_t minYear, uint16_t maxYear) {
  if (epochMs <= 0) return false;
  HardwareTypes::DateTimeValue value;
  if (!epochUtcToDateTime(epochMs / 1000LL, value)) return false;
  return isPlausibleDateTime(value, minYear, maxYear);
}

}  // namespace TimeUtils
