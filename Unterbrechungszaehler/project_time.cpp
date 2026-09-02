#include "project_time.h"

#include <cstdlib>
#include <cstring>
#include <ctime>

#include "project_config.h"

namespace ProjectTime {
namespace {

// Civil date conversion adapted from Howard Hinnant's public-domain algorithms.
// It lets aggregate files store a compact day index without relying on libc TZ
// rules when converting the stored calendar day back to Y/M/D.
int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned mp = month > 2 ? month - 3U : month + 9U;
  const unsigned doy = (153U * mp + 2U) / 5U + day - 1U;
  const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
  return static_cast<int64_t>(era) * 146097LL + static_cast<int64_t>(doe) - 719468LL;
}

void civilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
  z += 719468LL;
  const int64_t era = (z >= 0 ? z : z - 146096LL) / 146097LL;
  const unsigned doe = static_cast<unsigned>(z - era * 146097LL);
  const unsigned yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
  year = static_cast<int>(yoe) + static_cast<int>(era) * 400;
  const unsigned doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
  const unsigned mp = (5U * doy + 2U) / 153U;
  day = doy - (153U * mp + 2U) / 5U + 1U;
  month = mp < 10 ? mp + 3U : mp - 9U;
  year += month <= 2;
}

constexpr int64_t BASE_DAY = 18262LL;  // 2020-01-01 relative to Unix epoch.

bool validDate(uint16_t year, uint8_t month, uint8_t day) {
  if (year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31) return false;
  const int64_t z = daysFromCivil(year, month, day);
  int y = 0; unsigned m = 0, d = 0;
  civilFromDays(z, y, m, d);
  return y == year && m == month && d == day;
}

uint8_t isoWeekday(uint16_t year, uint8_t month, uint8_t day) {
  const int64_t days = daysFromCivil(year, month, day);
  // 1970-01-01 was Thursday (ISO 4).
  int value = static_cast<int>((days + 3LL) % 7LL);
  if (value < 0) value += 7;
  return static_cast<uint8_t>(value + 1); // Monday=1..Sunday=7
}

uint16_t dayOfYear(uint16_t year, uint8_t month, uint8_t day) {
  const int64_t first = daysFromCivil(year, 1, 1);
  return static_cast<uint16_t>(daysFromCivil(year, month, day) - first + 1LL);
}

bool leap(uint16_t year) {
  return (year % 4U == 0U && year % 100U != 0U) || (year % 400U == 0U);
}

uint8_t weeksInIsoYear(uint16_t year) {
  const uint8_t jan1 = isoWeekday(year, 1, 1);
  return (jan1 == 4 || (jan1 == 3 && leap(year))) ? 53 : 52;
}

}  // namespace

void begin() {
  setenv("TZ", ProjectConfig::TIMEZONE_POSIX, 1);
  tzset();
}

bool dateToDayIndex(uint16_t year, uint8_t month, uint8_t day, uint16_t &dayIndex) {
  if (!validDate(year, month, day)) return false;
  const int64_t delta = daysFromCivil(year, month, day) - BASE_DAY;
  if (delta < 0 || delta > 65535LL) return false;
  dayIndex = static_cast<uint16_t>(delta);
  return true;
}

bool dayIndexToDate(uint16_t dayIndex, uint16_t &year, uint8_t &month, uint8_t &day) {
  int y = 0; unsigned m = 0, d = 0;
  civilFromDays(BASE_DAY + static_cast<int64_t>(dayIndex), y, m, d);
  if (y < 0 || y > 65535 || m > 255 || d > 255) return false;
  year = static_cast<uint16_t>(y);
  month = static_cast<uint8_t>(m);
  day = static_cast<uint8_t>(d);
  return true;
}

uint8_t weekdayFromDayIndex(uint16_t dayIndex) {
  // 2020-01-01 was Wednesday => Monday-based index 2.
  return static_cast<uint8_t>((static_cast<uint32_t>(dayIndex) + 2U) % 7U);
}

bool isoWeekForDate(uint16_t year, uint8_t month, uint8_t day, uint16_t &isoYear, uint8_t &isoWeek) {
  if (!validDate(year, month, day)) return false;
  const int weekday = static_cast<int>(isoWeekday(year, month, day));
  const int doy = static_cast<int>(dayOfYear(year, month, day));
  int week = (doy - weekday + 10) / 7;
  int iy = year;
  if (week < 1) {
    iy = static_cast<int>(year) - 1;
    week = weeksInIsoYear(static_cast<uint16_t>(iy));
  } else if (week > weeksInIsoYear(year)) {
    iy = static_cast<int>(year) + 1;
    week = 1;
  }
  isoYear = static_cast<uint16_t>(iy);
  isoWeek = static_cast<uint8_t>(week);
  return true;
}

bool fromEpochSeconds(uint32_t epochSeconds, LocalDateTime &out) {
  out = LocalDateTime{};
  const time_t raw = static_cast<time_t>(epochSeconds);
  struct tm local {};
  if (!localtime_r(&raw, &local)) return false;
  const uint16_t year = static_cast<uint16_t>(local.tm_year + 1900);
  const uint8_t month = static_cast<uint8_t>(local.tm_mon + 1);
  const uint8_t day = static_cast<uint8_t>(local.tm_mday);
  uint16_t index = 0;
  uint16_t isoYear = 0; uint8_t isoWeek = 0;
  if (!dateToDayIndex(year, month, day, index) || !isoWeekForDate(year, month, day, isoYear, isoWeek)) return false;
  out.valid = true;
  out.year = year;
  out.month = month;
  out.day = day;
  out.hour = static_cast<uint8_t>(local.tm_hour);
  out.minute = static_cast<uint8_t>(local.tm_min);
  out.second = static_cast<uint8_t>(local.tm_sec);
  out.weekday = static_cast<uint8_t>((local.tm_wday + 6) % 7); // libc Sunday=0
  out.dayIndex = index;
  out.isoYear = isoYear;
  out.isoWeek = isoWeek;
  return true;
}

bool fromEpochMs(int64_t epochMs, LocalDateTime &out) {
  if (epochMs <= 0) return false;
  return fromEpochSeconds(static_cast<uint32_t>(epochMs / 1000LL), out);
}

bool parseDate(const char *text, uint16_t &dayIndex) {
  if (!text || strlen(text) != 10 || text[4] != '-' || text[7] != '-') return false;
  for (size_t i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) continue;
    if (text[i] < '0' || text[i] > '9') return false;
  }
  const uint16_t year = static_cast<uint16_t>((text[0]-'0')*1000 + (text[1]-'0')*100 + (text[2]-'0')*10 + (text[3]-'0'));
  const uint8_t month = static_cast<uint8_t>((text[5]-'0')*10 + (text[6]-'0'));
  const uint8_t day = static_cast<uint8_t>((text[8]-'0')*10 + (text[9]-'0'));
  return dateToDayIndex(year, month, day, dayIndex);
}

void formatUtc(uint32_t epochSeconds, char *buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) return;
  const time_t raw = static_cast<time_t>(epochSeconds);
  struct tm value {};
  if (!gmtime_r(&raw, &value)) { buffer[0] = '\0'; return; }
  snprintf(buffer, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02dZ",
           value.tm_year + 1900, value.tm_mon + 1, value.tm_mday,
           value.tm_hour, value.tm_min, value.tm_sec);
}

void formatLocal(uint32_t epochSeconds, char *buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) return;
  const time_t raw = static_cast<time_t>(epochSeconds);
  struct tm value {};
  if (!localtime_r(&raw, &value)) { buffer[0] = '\0'; return; }
  snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
           value.tm_year + 1900, value.tm_mon + 1, value.tm_mday,
           value.tm_hour, value.tm_min, value.tm_sec);
}

}  // namespace ProjectTime
