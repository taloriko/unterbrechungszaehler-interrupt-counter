#include "AnalyticsService.h"

#include <time.h>

#include "StorageService.h"

namespace {
static constexpr uint32_t ANALYTICS_CHUNK = 256;
}

void AnalyticsService::begin(StorageService* storage) {
  storage_ = storage;
}

bool AnalyticsService::ensureBaseAggregates() {
  if (!storage_ || !storage_->archiveReady()) return false;
  if (cachedRevision_ == storage_->revision()) return true;

  clearBase();
  baseYear_ = determineBaseYear();

  uint32_t buffer[ANALYTICS_CHUNK];
  for (uint32_t start = 0; start < storage_->archiveCount(); start += ANALYTICS_CHUNK) {
    uint32_t readCount = 0;
    if (!storage_->readArchiveChunk(start, ANALYTICS_CHUNK, buffer, readCount)) return false;

    for (uint32_t i = 0; i < readCount; i++) {
      const uint32_t epoch = buffer[i];
      if (epoch <= UicConfig::VALID_TIME_MIN) continue;

      time_t raw = static_cast<time_t>(epoch);
      struct tm value = {};
      localtime_r(&raw, &value);

      const int year = value.tm_year + 1900;
      const int offset = baseYear_ - year;
      if (offset < 0 || offset >= UicConfig::LONGTERM_CACHE_YEARS) continue;

      yearUsed_[offset] = true;
      if (value.tm_mon >= 0 && value.tm_mon < 12) {
        increment(yearMonth_[offset][value.tm_mon]);
        const int week = isoWeek(value);
        if (week >= 1 && week <= 53) increment(monthWeek_[offset][value.tm_mon][week - 1]);
      }
      if (value.tm_wday >= 0 && value.tm_wday < 7 && value.tm_hour >= 0 && value.tm_hour < 24) {
        increment(weekdayHour_[value.tm_wday][value.tm_hour]);
      }
    }
    delay(0);
  }

  cachedRevision_ = storage_->revision();
  return true;
}

bool AnalyticsService::ensureSelectedWeek(int selectedYear, int selectedWeek) {
  if (!storage_ || !storage_->archiveReady() || selectedWeek < 1 || selectedWeek > 53) return false;
  if (selectedRevision_ == storage_->revision() && selectedYear_ == selectedYear && selectedWeek_ == selectedWeek) return true;

  memset(selectedWeekdayHour_, 0, sizeof(selectedWeekdayHour_));

  uint32_t buffer[ANALYTICS_CHUNK];
  for (uint32_t start = 0; start < storage_->archiveCount(); start += ANALYTICS_CHUNK) {
    uint32_t readCount = 0;
    if (!storage_->readArchiveChunk(start, ANALYTICS_CHUNK, buffer, readCount)) return false;

    for (uint32_t i = 0; i < readCount; i++) {
      const uint32_t epoch = buffer[i];
      if (epoch <= UicConfig::VALID_TIME_MIN) continue;

      time_t raw = static_cast<time_t>(epoch);
      struct tm value = {};
      localtime_r(&raw, &value);
      if (isoYear(value) != selectedYear || isoWeek(value) != selectedWeek) continue;
      if (value.tm_wday >= 0 && value.tm_wday < 7 && value.tm_hour >= 0 && value.tm_hour < 24) {
        increment(selectedWeekdayHour_[value.tm_wday][value.tm_hour]);
      }
    }
    delay(0);
  }

  selectedRevision_ = storage_->revision();
  selectedYear_ = selectedYear;
  selectedWeek_ = selectedWeek;
  return true;
}

uint16_t AnalyticsService::weekdayHour(uint8_t day, uint8_t hour) const {
  return day < 7 && hour < 24 ? weekdayHour_[day][hour] : 0;
}

uint16_t AnalyticsService::selectedWeekdayHour(uint8_t day, uint8_t hour) const {
  return day < 7 && hour < 24 ? selectedWeekdayHour_[day][hour] : 0;
}

uint16_t AnalyticsService::monthWeek(uint8_t yearOffset, uint8_t month, uint8_t weekIndex) const {
  return yearOffset < UicConfig::LONGTERM_CACHE_YEARS && month < 12 && weekIndex < 53
           ? monthWeek_[yearOffset][month][weekIndex] : 0;
}

uint16_t AnalyticsService::yearMonth(uint8_t yearOffset, uint8_t month) const {
  return yearOffset < UicConfig::LONGTERM_CACHE_YEARS && month < 12 ? yearMonth_[yearOffset][month] : 0;
}

bool AnalyticsService::yearUsed(uint8_t yearOffset) const {
  return yearOffset < UicConfig::LONGTERM_CACHE_YEARS ? yearUsed_[yearOffset] : false;
}

int AnalyticsService::isoWeek(const struct tm& value) {
  char text[4] = {0};
  strftime(text, sizeof(text), "%V", &value);
  int week = atoi(text);
  if (week < 1) week = 1;
  if (week > 53) week = 53;
  return week;
}

int AnalyticsService::isoYear(const struct tm& value) {
  char text[8] = {0};
  if (strftime(text, sizeof(text), "%G", &value) > 0) {
    const int year = atoi(text);
    if (year >= 2000 && year <= 2199) return year;
  }
  return value.tm_year + 1900;
}

void AnalyticsService::clearBase() {
  memset(weekdayHour_, 0, sizeof(weekdayHour_));
  memset(monthWeek_, 0, sizeof(monthWeek_));
  memset(yearMonth_, 0, sizeof(yearMonth_));
  memset(yearUsed_, 0, sizeof(yearUsed_));
}

int AnalyticsService::determineBaseYear() {
  time_t now = time(nullptr);
  if (now > static_cast<time_t>(UicConfig::VALID_TIME_MIN)) {
    struct tm value = {};
    localtime_r(&now, &value);
    return value.tm_year + 1900;
  }

  if (storage_ && storage_->archiveCount() > 0) {
    uint32_t epoch = 0;
    if (storage_->readArchive(storage_->archiveCount() - 1, epoch)) {
      time_t raw = static_cast<time_t>(epoch);
      struct tm value = {};
      localtime_r(&raw, &value);
      return value.tm_year + 1900;
    }
  }
  return 2026;
}

void AnalyticsService::increment(uint16_t& value) {
  if (value < 65535) value++;
}
