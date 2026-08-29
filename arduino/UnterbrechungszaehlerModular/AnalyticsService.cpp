#include "AnalyticsService.h"

#include <time.h>

#include "StorageService.h"

namespace {
static constexpr uint32_t ANALYTICS_CHUNK = 256;

bool isoWeekEpochRange(int isoYear, int isoWeek, uint32_t& startEpoch, uint32_t& endEpoch) {
  if (isoYear < 2000 || isoYear > 2199 || isoWeek < 1 || isoWeek > 53) return false;

  // ISO-Woche 1 enthaelt immer den 4. Januar. Von dort zum Montag der
  // ISO-Woche gehen und anschliessend die gewuenschte Woche addieren.
  struct tm jan4 = {};
  jan4.tm_year = isoYear - 1900;
  jan4.tm_mon = 0;
  jan4.tm_mday = 4;
  jan4.tm_hour = 0;
  jan4.tm_min = 0;
  jan4.tm_sec = 0;
  jan4.tm_isdst = -1;
  if (mktime(&jan4) <= 0) return false;

  const int daysSinceMonday = (jan4.tm_wday + 6) % 7;
  struct tm monday = jan4;
  monday.tm_mday -= daysSinceMonday;
  monday.tm_mday += (isoWeek - 1) * 7;
  monday.tm_hour = 0;
  monday.tm_min = 0;
  monday.tm_sec = 0;
  monday.tm_isdst = -1;
  const time_t start = mktime(&monday);
  if (start <= 0) return false;

  struct tm nextMonday = monday;
  nextMonday.tm_mday += 7;
  nextMonday.tm_isdst = -1;
  const time_t end = mktime(&nextMonday);
  if (end <= start) return false;

  startEpoch = static_cast<uint32_t>(start);
  endEpoch = static_cast<uint32_t>(end);
  return true;
}

uint32_t archiveLowerBound(StorageService* storage, uint32_t target) {
  if (!storage) return 0;
  uint32_t left = 0;
  uint32_t right = storage->archiveCount();

  while (left < right) {
    const uint32_t mid = left + (right - left) / 2;
    uint32_t value = 0;
    if (!storage->readArchive(mid, value)) return left;
    if (value < target) left = mid + 1;
    else right = mid;
  }
  return left;
}
}

void AnalyticsService::begin(StorageService* storage) {
  storage_ = storage;
}

bool AnalyticsService::warmCurrent() {
  if (!ensureBaseAggregates()) return false;
  time_t now = time(nullptr);
  if (now <= static_cast<time_t>(UicConfig::VALID_TIME_MIN)) return true;
  struct tm value = {};
  localtime_r(&now, &value);
  return ensureSelectedWeek(isoYear(value), isoWeek(value));
}

bool AnalyticsService::addEpochToBase(uint32_t epoch) {
  if (epoch <= UicConfig::VALID_TIME_MIN) return true;
  time_t raw = static_cast<time_t>(epoch);
  struct tm value = {};
  localtime_r(&raw, &value);

  const int year = value.tm_year + 1900;
  const int offset = baseYear_ - year;
  if (offset < 0 || offset >= UicConfig::LONGTERM_CACHE_YEARS) return true;

  yearUsed_[offset] = true;
  if (value.tm_mon >= 0 && value.tm_mon < 12) {
    increment(yearMonth_[offset][value.tm_mon]);
    const int week = isoWeek(value);
    if (week >= 1 && week <= 53) increment(monthWeek_[offset][value.tm_mon][week - 1]);
  }
  if (value.tm_wday >= 0 && value.tm_wday < 7 && value.tm_hour >= 0 && value.tm_hour < 24) {
    increment(weekdayHour_[value.tm_wday][value.tm_hour]);
  }
  return true;
}

bool AnalyticsService::addEpochToSelectedWeek(uint32_t epoch, int selectedYear, int selectedWeek) {
  if (epoch <= UicConfig::VALID_TIME_MIN) return true;
  time_t raw = static_cast<time_t>(epoch);
  struct tm value = {};
  localtime_r(&raw, &value);
  if (isoYear(value) != selectedYear || isoWeek(value) != selectedWeek) return true;
  if (value.tm_wday >= 0 && value.tm_wday < 7 && value.tm_hour >= 0 && value.tm_hour < 24) {
    increment(selectedWeekdayHour_[value.tm_wday][value.tm_hour]);
  }
  return true;
}

bool AnalyticsService::ensureBaseAggregates() {
  if (!storage_ || !storage_->archiveReady()) return false;

  const uint32_t revision = storage_->revision();
  const uint32_t count = storage_->archiveCount();
  if (cachedRevision_ == revision) return true;

  // Normalfall nach einer neuen Unterbrechung: genau einen neuen Zeitstempel
  // inkrementell einrechnen statt das komplette Langzeitarchiv neu zu lesen.
  if (cachedRevision_ != 0xFFFFFFFFUL &&
      revision == cachedRevision_ + 1 &&
      count == cachedArchiveCount_ + 1) {
    uint32_t epoch = 0;
    if (storage_->readArchive(count - 1, epoch) && addEpochToBase(epoch)) {
      cachedRevision_ = revision;
      cachedArchiveCount_ = count;
      return true;
    }
  }

  clearBase();
  baseYear_ = determineBaseYear();

  uint32_t buffer[ANALYTICS_CHUNK];
  for (uint32_t start = 0; start < count; start += ANALYTICS_CHUNK) {
    uint32_t readCount = 0;
    if (!storage_->readArchiveChunk(start, ANALYTICS_CHUNK, buffer, readCount)) return false;
    for (uint32_t i = 0; i < readCount; i++) addEpochToBase(buffer[i]);
    delay(0);
  }

  cachedRevision_ = revision;
  cachedArchiveCount_ = count;
  return true;
}

bool AnalyticsService::ensureSelectedWeek(int selectedYear, int selectedWeek) {
  if (!storage_ || !storage_->archiveReady() || selectedWeek < 1 || selectedWeek > 53) return false;

  const uint32_t revision = storage_->revision();
  const uint32_t count = storage_->archiveCount();
  if (selectedRevision_ == revision && selectedYear_ == selectedYear && selectedWeek_ == selectedWeek) return true;

  // Auch der aktuell gewaehlte Wochen-Cache kann bei einem normalen Append
  // mit nur einem neuen Zeitstempel aktualisiert werden.
  if (selectedRevision_ != 0xFFFFFFFFUL &&
      selectedYear_ == selectedYear && selectedWeek_ == selectedWeek &&
      revision == selectedRevision_ + 1 &&
      count == selectedArchiveCount_ + 1) {
    uint32_t epoch = 0;
    if (storage_->readArchive(count - 1, epoch) && addEpochToSelectedWeek(epoch, selectedYear, selectedWeek)) {
      selectedRevision_ = revision;
      selectedArchiveCount_ = count;
      return true;
    }
  }

  memset(selectedWeekdayHour_, 0, sizeof(selectedWeekdayHour_));

  // Der Langzeitring liegt chronologisch vor. Fuer eine einzelne KW werden
  // deshalb zuerst mit O(log n) die Grenzen gesucht und danach nur die
  // Ereignisse dieser Woche gelesen. Das verhindert Vollscans beim KW-Wechsel.
  uint32_t weekStart = 0;
  uint32_t weekEnd = 0;
  uint32_t first = 0;
  uint32_t last = count;
  if (isoWeekEpochRange(selectedYear, selectedWeek, weekStart, weekEnd) && count > 0) {
    first = archiveLowerBound(storage_, weekStart);
    last = archiveLowerBound(storage_, weekEnd);
    if (last < first || last > count) {
      first = 0;
      last = count;
    }
  }

  uint32_t buffer[ANALYTICS_CHUNK];
  for (uint32_t start = first; start < last; start += ANALYTICS_CHUNK) {
    const uint32_t wanted = min<uint32_t>(ANALYTICS_CHUNK, last - start);
    uint32_t readCount = 0;
    if (!storage_->readArchiveChunk(start, wanted, buffer, readCount)) return false;
    for (uint32_t i = 0; i < readCount; i++) addEpochToSelectedWeek(buffer[i], selectedYear, selectedWeek);
    delay(0);
  }

  selectedRevision_ = revision;
  selectedArchiveCount_ = count;
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
