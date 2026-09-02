#pragma once

#include <Arduino.h>

namespace ProjectTime {

struct LocalDateTime {
  bool valid = false;
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint8_t weekday = 0;  // Monday=0 ... Sunday=6
  uint16_t dayIndex = 0;
  uint16_t isoYear = 0;
  uint8_t isoWeek = 0;
};

void begin();
bool fromEpochSeconds(uint32_t epochSeconds, LocalDateTime &out);
bool fromEpochMs(int64_t epochMs, LocalDateTime &out);

bool parseDate(const char *text, uint16_t &dayIndex);
bool dayIndexToDate(uint16_t dayIndex, uint16_t &year, uint8_t &month, uint8_t &day);
bool dateToDayIndex(uint16_t year, uint8_t month, uint8_t day, uint16_t &dayIndex);
uint8_t weekdayFromDayIndex(uint16_t dayIndex);
bool isoWeekForDate(uint16_t year, uint8_t month, uint8_t day, uint16_t &isoYear, uint8_t &isoWeek);

void formatUtc(uint32_t epochSeconds, char *buffer, size_t bufferSize);
void formatLocal(uint32_t epochSeconds, char *buffer, size_t bufferSize);

}  // namespace ProjectTime
