#pragma once

#include <Arduino.h>

#include "Config.h"

class StorageService;

class AnalyticsService {
public:
  void begin(StorageService* storage);
  bool ensureBaseAggregates();
  bool ensureSelectedWeek(int isoYear, int isoWeek);
  bool warmCurrent();

  int baseYear() const { return baseYear_; }
  uint16_t weekdayHour(uint8_t day, uint8_t hour) const;
  uint16_t selectedWeekdayHour(uint8_t day, uint8_t hour) const;
  uint16_t monthWeek(uint8_t yearOffset, uint8_t month, uint8_t weekIndex) const;
  uint16_t yearMonth(uint8_t yearOffset, uint8_t month) const;
  bool yearUsed(uint8_t yearOffset) const;

  static int isoWeek(const struct tm& value);
  static int isoYear(const struct tm& value);

private:
  void clearBase();
  int determineBaseYear();
  void increment(uint16_t& value);
  bool addEpochToBase(uint32_t epoch);
  bool addEpochToSelectedWeek(uint32_t epoch, int selectedYear, int selectedWeek);

  StorageService* storage_ = nullptr;
  uint32_t cachedRevision_ = 0xFFFFFFFFUL;
  uint32_t cachedArchiveCount_ = 0;
  int baseYear_ = 2026;
  uint16_t weekdayHour_[7][24] = {};
  uint16_t monthWeek_[UicConfig::LONGTERM_CACHE_YEARS][12][53] = {};
  uint16_t yearMonth_[UicConfig::LONGTERM_CACHE_YEARS][12] = {};
  bool yearUsed_[UicConfig::LONGTERM_CACHE_YEARS] = {};

  uint32_t selectedRevision_ = 0xFFFFFFFFUL;
  uint32_t selectedArchiveCount_ = 0;
  int selectedYear_ = 0;
  int selectedWeek_ = 0;
  uint16_t selectedWeekdayHour_[7][24] = {};
};
