#include "AnalyticsService.h"

#include <LittleFS.h>
#include <time.h>

#include "StorageService.h"

namespace {
static constexpr uint32_t ANALYTICS_CHUNK = 256;

struct __attribute__((packed)) AnalyticsCacheHeader {
  uint32_t magic;
  uint16_t version;
  int16_t baseYear;
  uint32_t archiveCount;
  uint32_t lastEpoch;
  uint32_t payloadBytes;
};

static constexpr size_t CACHE_WEEKDAY_BYTES = 7U * 24U * sizeof(uint16_t);
static constexpr size_t CACHE_MONTH_WEEK_BYTES =
  static_cast<size_t>(UicConfig::LONGTERM_CACHE_YEARS) * 12U * 53U * sizeof(uint16_t);
static constexpr size_t CACHE_YEAR_MONTH_BYTES =
  static_cast<size_t>(UicConfig::LONGTERM_CACHE_YEARS) * 12U * sizeof(uint16_t);
static constexpr size_t CACHE_YEAR_USED_BYTES = UicConfig::LONGTERM_CACHE_YEARS;
static constexpr size_t CACHE_PAYLOAD_BYTES =
  CACHE_WEEKDAY_BYTES + CACHE_MONTH_WEEK_BYTES + CACHE_YEAR_MONTH_BYTES + CACHE_YEAR_USED_BYTES;

static constexpr size_t CACHE_WEEKDAY_OFFSET = sizeof(AnalyticsCacheHeader);
static constexpr size_t CACHE_MONTH_WEEK_OFFSET = CACHE_WEEKDAY_OFFSET + CACHE_WEEKDAY_BYTES;
static constexpr size_t CACHE_YEAR_MONTH_OFFSET = CACHE_MONTH_WEEK_OFFSET + CACHE_MONTH_WEEK_BYTES;
static constexpr size_t CACHE_YEAR_USED_OFFSET = CACHE_YEAR_MONTH_OFFSET + CACHE_YEAR_MONTH_BYTES;

bool isoWeekEpochRange(int isoYear, int isoWeek, uint32_t& startEpoch, uint32_t& endEpoch) {
  if (isoYear < 2000 || isoYear > 2199 || isoWeek < 1 || isoWeek > 53) return false;

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

uint32_t lastArchiveEpoch(StorageService* storage) {
  if (!storage || storage->archiveCount() == 0) return 0;
  uint32_t epoch = 0;
  return storage->readArchive(storage->archiveCount() - 1, epoch) ? epoch : 0;
}

size_t weekdayCacheOffset(uint8_t day, uint8_t hour) {
  return CACHE_WEEKDAY_OFFSET + (static_cast<size_t>(day) * 24U + hour) * sizeof(uint16_t);
}

size_t monthWeekCacheOffset(uint8_t yearOffset, uint8_t month, uint8_t weekIndex) {
  const size_t index = (static_cast<size_t>(yearOffset) * 12U * 53U) +
                       (static_cast<size_t>(month) * 53U) + weekIndex;
  return CACHE_MONTH_WEEK_OFFSET + index * sizeof(uint16_t);
}

size_t yearMonthCacheOffset(uint8_t yearOffset, uint8_t month) {
  const size_t index = static_cast<size_t>(yearOffset) * 12U + month;
  return CACHE_YEAR_MONTH_OFFSET + index * sizeof(uint16_t);
}
}

void AnalyticsService::begin(StorageService* storage) {
  storage_ = storage;
  if (loadBaseCache()) {
    Serial.printf("[ANALYTIK] Cache geladen: %lu Ereignisse\n",
                  static_cast<unsigned long>(cachedArchiveCount_));
  }
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

bool AnalyticsService::loadBaseCache() {
  if (!storage_ || !storage_->archiveReady() || !storage_->fsReady() ||
      !LittleFS.exists(UicConfig::ANALYTICS_CACHE_FILE)) return false;

  const uint32_t count = storage_->archiveCount();
  const uint32_t lastEpoch = lastArchiveEpoch(storage_);
  const int expectedBaseYear = determineBaseYear();

  File file = LittleFS.open(UicConfig::ANALYTICS_CACHE_FILE, FILE_READ);
  if (!file) return false;

  AnalyticsCacheHeader header = {};
  const bool headerOk = file.read(reinterpret_cast<uint8_t*>(&header), sizeof(header)) == sizeof(header);
  const size_t expectedSize = sizeof(AnalyticsCacheHeader) + CACHE_PAYLOAD_BYTES;
  if (!headerOk ||
      header.magic != UicConfig::ANALYTICS_CACHE_MAGIC ||
      header.version != UicConfig::ANALYTICS_CACHE_VERSION ||
      header.baseYear != expectedBaseYear ||
      header.archiveCount != count ||
      header.lastEpoch != lastEpoch ||
      header.payloadBytes != CACHE_PAYLOAD_BYTES ||
      file.size() != expectedSize) {
    file.close();
    return false;
  }

  const bool weekdayOk = file.read(reinterpret_cast<uint8_t*>(weekdayHour_), sizeof(weekdayHour_)) == sizeof(weekdayHour_);
  const bool monthWeekOk = file.read(reinterpret_cast<uint8_t*>(monthWeek_), sizeof(monthWeek_)) == sizeof(monthWeek_);
  const bool yearMonthOk = file.read(reinterpret_cast<uint8_t*>(yearMonth_), sizeof(yearMonth_)) == sizeof(yearMonth_);
  uint8_t used[UicConfig::LONGTERM_CACHE_YEARS] = {};
  const bool yearUsedOk = file.read(used, sizeof(used)) == sizeof(used);
  file.close();

  if (!weekdayOk || !monthWeekOk || !yearMonthOk || !yearUsedOk) {
    clearBase();
    return false;
  }

  for (uint8_t i = 0; i < UicConfig::LONGTERM_CACHE_YEARS; i++) yearUsed_[i] = used[i] != 0;
  baseYear_ = expectedBaseYear;
  cachedRevision_ = storage_->revision();
  cachedArchiveCount_ = count;
  return true;
}

bool AnalyticsService::saveBaseCache(uint32_t lastEpoch) {
  if (!storage_ || !storage_->fsReady()) return false;

  LittleFS.remove(UicConfig::ANALYTICS_CACHE_TMP_FILE);
  File file = LittleFS.open(UicConfig::ANALYTICS_CACHE_TMP_FILE, FILE_WRITE);
  if (!file) return false;

  AnalyticsCacheHeader header = {};
  header.magic = UicConfig::ANALYTICS_CACHE_MAGIC;
  header.version = UicConfig::ANALYTICS_CACHE_VERSION;
  header.baseYear = static_cast<int16_t>(baseYear_);
  header.archiveCount = cachedArchiveCount_;
  header.lastEpoch = lastEpoch;
  header.payloadBytes = CACHE_PAYLOAD_BYTES;

  bool ok = file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) == sizeof(header);
  ok = ok && file.write(reinterpret_cast<const uint8_t*>(weekdayHour_), sizeof(weekdayHour_)) == sizeof(weekdayHour_);
  ok = ok && file.write(reinterpret_cast<const uint8_t*>(monthWeek_), sizeof(monthWeek_)) == sizeof(monthWeek_);
  ok = ok && file.write(reinterpret_cast<const uint8_t*>(yearMonth_), sizeof(yearMonth_)) == sizeof(yearMonth_);

  uint8_t used[UicConfig::LONGTERM_CACHE_YEARS] = {};
  for (uint8_t i = 0; i < UicConfig::LONGTERM_CACHE_YEARS; i++) used[i] = yearUsed_[i] ? 1 : 0;
  ok = ok && file.write(used, sizeof(used)) == sizeof(used);
  file.flush();
  file.close();

  if (!ok) {
    LittleFS.remove(UicConfig::ANALYTICS_CACHE_TMP_FILE);
    return false;
  }

  LittleFS.remove(UicConfig::ANALYTICS_CACHE_FILE);
  if (!LittleFS.rename(UicConfig::ANALYTICS_CACHE_TMP_FILE, UicConfig::ANALYTICS_CACHE_FILE)) {
    LittleFS.remove(UicConfig::ANALYTICS_CACHE_TMP_FILE);
    return false;
  }
  return true;
}

bool AnalyticsService::updateBaseCacheAppend(uint32_t epoch, uint32_t previousCount, uint32_t newCount) {
  if (!storage_ || !storage_->fsReady() || !LittleFS.exists(UicConfig::ANALYTICS_CACHE_FILE)) return false;

  File file = LittleFS.open(UicConfig::ANALYTICS_CACHE_FILE, "r+");
  if (!file) return false;

  AnalyticsCacheHeader header = {};
  if (file.read(reinterpret_cast<uint8_t*>(&header), sizeof(header)) != sizeof(header) ||
      header.magic != UicConfig::ANALYTICS_CACHE_MAGIC ||
      header.version != UicConfig::ANALYTICS_CACHE_VERSION ||
      header.baseYear != baseYear_ ||
      header.archiveCount != previousCount ||
      header.payloadBytes != CACHE_PAYLOAD_BYTES) {
    file.close();
    return false;
  }

  time_t raw = static_cast<time_t>(epoch);
  struct tm value = {};
  localtime_r(&raw, &value);
  const int offset = baseYear_ - (value.tm_year + 1900);

  bool ok = true;
  if (offset >= 0 && offset < UicConfig::LONGTERM_CACHE_YEARS) {
    const uint8_t yearOffset = static_cast<uint8_t>(offset);
    if (value.tm_wday >= 0 && value.tm_wday < 7 && value.tm_hour >= 0 && value.tm_hour < 24) {
      ok = file.seek(weekdayCacheOffset(value.tm_wday, value.tm_hour), SeekSet) &&
           file.write(reinterpret_cast<const uint8_t*>(&weekdayHour_[value.tm_wday][value.tm_hour]), sizeof(uint16_t)) == sizeof(uint16_t);
    }
    if (ok && value.tm_mon >= 0 && value.tm_mon < 12) {
      const int week = isoWeek(value);
      ok = file.seek(yearMonthCacheOffset(yearOffset, value.tm_mon), SeekSet) &&
           file.write(reinterpret_cast<const uint8_t*>(&yearMonth_[yearOffset][value.tm_mon]), sizeof(uint16_t)) == sizeof(uint16_t);
      if (ok && week >= 1 && week <= 53) {
        ok = file.seek(monthWeekCacheOffset(yearOffset, value.tm_mon, week - 1), SeekSet) &&
             file.write(reinterpret_cast<const uint8_t*>(&monthWeek_[yearOffset][value.tm_mon][week - 1]), sizeof(uint16_t)) == sizeof(uint16_t);
      }
    }
    if (ok) {
      const uint8_t used = yearUsed_[yearOffset] ? 1 : 0;
      ok = file.seek(CACHE_YEAR_USED_OFFSET + yearOffset, SeekSet) && file.write(&used, 1) == 1;
    }
  }

  if (ok) {
    header.archiveCount = newCount;
    header.lastEpoch = epoch;
    ok = file.seek(0, SeekSet) &&
         file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) == sizeof(header);
  }
  file.flush();
  file.close();
  return ok;
}

bool AnalyticsService::ensureBaseAggregates() {
  if (!storage_ || !storage_->archiveReady()) return false;

  const uint32_t revision = storage_->revision();
  const uint32_t count = storage_->archiveCount();
  const int expectedBaseYear = determineBaseYear();
  if (cachedRevision_ == revision && cachedArchiveCount_ == count && baseYear_ == expectedBaseYear) return true;

  if (cachedRevision_ != 0xFFFFFFFFUL &&
      baseYear_ == expectedBaseYear &&
      revision == cachedRevision_ + 1 &&
      count == cachedArchiveCount_ + 1) {
    const uint32_t previousCount = cachedArchiveCount_;
    uint32_t epoch = 0;
    if (storage_->readArchive(count - 1, epoch) && addEpochToBase(epoch)) {
      cachedRevision_ = revision;
      cachedArchiveCount_ = count;
      if (!updateBaseCacheAppend(epoch, previousCount, count)) {
        Serial.println("[ANALYTIK] Cache konnte nicht inkrementell aktualisiert werden; RAM-Daten bleiben gueltig.");
      }
      return true;
    }
  }

  clearBase();
  baseYear_ = expectedBaseYear;

  uint32_t buffer[ANALYTICS_CHUNK];
  for (uint32_t start = 0; start < count; start += ANALYTICS_CHUNK) {
    uint32_t readCount = 0;
    if (!storage_->readArchiveChunk(start, ANALYTICS_CHUNK, buffer, readCount)) return false;
    for (uint32_t i = 0; i < readCount; i++) addEpochToBase(buffer[i]);
    delay(0);
  }

  cachedRevision_ = revision;
  cachedArchiveCount_ = count;
  const uint32_t lastEpoch = lastArchiveEpoch(storage_);
  if (saveBaseCache(lastEpoch)) {
    Serial.printf("[ANALYTIK] Cache neu aufgebaut: %lu Ereignisse\n", static_cast<unsigned long>(count));
  } else {
    Serial.println("[ANALYTIK] Cache konnte nicht gespeichert werden; Heatmap bleibt im RAM nutzbar.");
  }
  return true;
}

bool AnalyticsService::ensureSelectedWeek(int selectedYear, int selectedWeek) {
  if (!storage_ || !storage_->archiveReady() || selectedWeek < 1 || selectedWeek > 53) return false;

  const uint32_t revision = storage_->revision();
  const uint32_t count = storage_->archiveCount();
  if (selectedRevision_ == revision && selectedYear_ == selectedYear && selectedWeek_ == selectedWeek) return true;

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
