#include "interruption_api.h"

#include <cstring>

#include "audio_dy_sv17f.h"
#include "hardware_registry.h"
#include "interruption_aggregates.h"
#include "interruption_service.h"
#include "interruption_store.h"
#include "json_utils.h"
#include "ota_module.h"
#include "project_config.h"
#include "project_preferences.h"
#include "project_time.h"
#include "time_service.h"
#include "wifi_module.h"

namespace InterruptionApi {
namespace {

void fieldString(String &out, const char *key, const char *value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendEscapedString(out, value);
  if (comma) out += ',';
}

void fieldBool(String &out, const char *key, bool value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendBool(out, value);
  if (comma) out += ',';
}

void fieldUInt(String &out, const char *key, uint32_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendUInt(out, value);
  if (comma) out += ',';
}

void fieldUInt64(String &out, const char *key, uint64_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendUInt64(out, value);
  if (comma) out += ',';
}

void fieldInt(String &out, const char *key, int32_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendInt(out, value);
  if (comma) out += ',';
}

void removeTrailingComma(String &out) {
  if (out.endsWith(",")) out.remove(out.length() - 1U);
}

void appendProjectPreferencesObjectInternal(String &out) {
  out += '{';
  fieldBool(out, "soundEnabled", ProjectPreferences::soundEnabled());
  fieldUInt(out, "soundTrack", ProjectPreferences::soundTrack());
  fieldString(out, "soundMode", ProjectPreferences::soundModeName());
  fieldUInt(out, "soundTrackCount", AudioDySv17f::musicCount());
  fieldBool(out, "displayFlashEnabled", ProjectPreferences::displayFlashEnabled());
  fieldString(out, "displayMode", ProjectPreferences::displayModeName());
  fieldUInt(out, "displayBrightness", ProjectPreferences::displayBrightnessPercent());
  fieldUInt(out, "displayDimAfterMinutes", ProjectPreferences::displayDimAfterMinutes());
  fieldUInt(out, "displayDimBrightness", ProjectPreferences::displayDimBrightnessPercent(), false);
  out += '}';
}

void appendSummaryObjectInternal(String &out) {
  const auto &summary = InterruptionService::summary();
  out += '{';
  fieldUInt(out, "todayCount", summary.todayCount);
  fieldUInt(out, "unassignedCount", summary.unassignedCount);
  fieldUInt64(out, "sequence", summary.liveSequence);
  fieldUInt64(out, "persistedSequence", summary.persistedSequence);
  fieldUInt64(out, "revision", summary.revision);

  // Current device monotonic time lets the browser age a relative event that
  // happened during this boot without inventing wall-clock time.
  fieldUInt64(out, "monotonicMs", TimeService::eventTimestamp().monotonicMs);
  fieldUInt(out, "pendingCount", summary.pendingCount);
  fieldUInt(out, "droppedCount", summary.droppedCount);
  fieldString(out, "storageState", InterruptionTypes::storageStateName(summary.storageState));
  fieldBool(out, "soundEnabled", summary.soundEnabled);

  JsonUtils::appendKey(out, "last");
  out += '{';
  fieldBool(out, "available", summary.lastAvailable);
  if (summary.lastAvailable) {
    fieldBool(out, "absoluteValid", summary.lastAbsoluteValid);
    fieldUInt(out, "timeValueSeconds", summary.lastTimeValueSeconds);
    fieldUInt64(out, "monotonicMs", summary.lastMonotonicMs);
    fieldString(out, "timeSource", TimeTypes::sourceName(summary.lastTimeSource));
    fieldString(out, "eventSource", InterruptionTypes::eventSourceName(summary.lastEventSource));
    if (summary.lastDeltaSeconds < InterruptionTypes::DELTA_UNKNOWN) {
      fieldUInt(out, "deltaSeconds", summary.lastDeltaSeconds);
    } else if (summary.lastDeltaSeconds == InterruptionTypes::DELTA_FIRST_OF_DAY) {
      fieldBool(out, "firstOfDay", true);
    }
  }
  removeTrailingComma(out);
  out += '}';
  out += '}';
}

void appendValues(String &out, const uint32_t *values, size_t count) {
  out += '[';
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) out += ',';
    JsonUtils::appendUInt(out, values[i]);
  }
  out += ']';
}

void appendStorageObject(String &out) {
  const auto &raw = InterruptionStore::info();
  const auto &daily = InterruptionAggregates::info();
  const auto &summary = InterruptionService::summary();

  out += '{';
  fieldString(out, "state", InterruptionTypes::storageStateName(summary.storageState));
  fieldUInt(out, "rawCount", raw.count);
  fieldUInt(out, "rawCapacity", raw.capacity);
  fieldUInt64(out, "oldestSequence", InterruptionStore::oldestSequence());
  fieldUInt64(out, "newestSequence", InterruptionStore::newestSequence());
  fieldUInt(out, "fsTotalBytes", static_cast<uint32_t>(raw.fsTotalBytes));
  fieldUInt(out, "fsUsedBytes", static_cast<uint32_t>(raw.fsUsedBytes));
  fieldUInt(out, "dailyCount", daily.dayCount);
  fieldUInt(out, "dailyCapacity", daily.capacity);
  fieldUInt(out, "unassignedCount", daily.unassignedCount);
  fieldUInt(out, "droppedCount", summary.droppedCount);
  fieldBool(out, "recovering", raw.recovering || daily.rebuilding, false);
  out += '}';
}

void servicePhysicalInputDuringRead(uint16_t &counter) {
  ++counter;
  if ((counter & 0x1FU) != 0U) return;

  // Statistics are deliberately lower priority than the physical button. The
  // synchronous WebServer handler periodically runs the generic input path and
  // immediate display/audio path. Persistence stays in the normal loop.
  HardwareRegistry::update();
  InterruptionService::serviceUrgent();
  delay(0);
}

struct HourContext {
  uint32_t values[7U * 24U]{};
  uint16_t scanCounter = 0;
  bool weekMode = false;
  uint16_t year = 0;
  uint8_t week = 0;
  uint16_t from = 0;
  uint16_t to = 0;
};

bool visitHour(const InterruptionAggregates::DailyRecord &record, void *context) {
  auto &ctx = *static_cast<HourContext *>(context);
  servicePhysicalInputDuringRead(ctx.scanCounter);

  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  if (!ProjectTime::dayIndexToDate(record.dayIndex, year, month, day)) return true;

  bool include = false;
  if (ctx.weekMode) {
    uint16_t isoYear = 0;
    uint8_t isoWeek = 0;
    if (ProjectTime::isoWeekForDate(year, month, day, isoYear, isoWeek)) {
      include = isoYear == ctx.year && isoWeek == ctx.week;
    }
  } else {
    include = record.dayIndex >= ctx.from && record.dayIndex <= ctx.to;
  }
  if (!include) return true;

  const uint8_t row = ProjectTime::weekdayFromDayIndex(record.dayIndex);
  for (uint8_t hour = 0; hour < 24; ++hour) {
    const size_t index = static_cast<size_t>(row) * 24U + hour;
    ctx.values[index] += record.hours[hour];
  }
  return true;
}

struct MonthWeekContext {
  uint16_t year = 0;
  uint32_t values[12U * 53U]{};
  uint16_t scanCounter = 0;
};

bool visitMonthWeek(const InterruptionAggregates::DailyRecord &record, void *context) {
  auto &ctx = *static_cast<MonthWeekContext *>(context);
  servicePhysicalInputDuringRead(ctx.scanCounter);

  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  if (!ProjectTime::dayIndexToDate(record.dayIndex, year, month, day) || year != ctx.year) {
    return true;
  }

  uint16_t isoYear = 0;
  uint8_t isoWeek = 0;
  if (!ProjectTime::isoWeekForDate(year, month, day, isoYear, isoWeek) || isoWeek < 1 || isoWeek > 53) {
    return true;
  }

  const size_t index = static_cast<size_t>(month - 1U) * 53U + static_cast<size_t>(isoWeek - 1U);
  ctx.values[index] += record.total;
  return true;
}

struct YearMonthContext {
  uint16_t startYear = 0;
  uint16_t endYear = 0;
  uint32_t values[5U * 12U]{};
  uint16_t maxYear = 0;
  bool onlyFindMax = false;
  uint16_t scanCounter = 0;
};

bool visitYearMonth(const InterruptionAggregates::DailyRecord &record, void *context) {
  auto &ctx = *static_cast<YearMonthContext *>(context);
  servicePhysicalInputDuringRead(ctx.scanCounter);

  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  if (!ProjectTime::dayIndexToDate(record.dayIndex, year, month, day)) return true;

  if (ctx.onlyFindMax) {
    if (year > ctx.maxYear) ctx.maxYear = year;
    return true;
  }
  if (year < ctx.startYear || year > ctx.endYear || month < 1 || month > 12) return true;

  const size_t index = static_cast<size_t>(year - ctx.startYear) * 12U + static_cast<size_t>(month - 1U);
  ctx.values[index] += record.total;
  return true;
}

struct AnalyticsBundleContext {
  bool hourlyWeekMode = false;
  uint16_t hourlyYear = 0;
  uint8_t hourlyWeek = 0;
  uint16_t hourlyFrom = 0;
  uint16_t hourlyTo = 0;
  uint16_t monthWeekYear = 0;
  uint16_t yearMonthStart = 0;
  uint16_t yearMonthEnd = 0;
  uint32_t hourly[7U * 24U]{};
  uint32_t monthWeek[12U * 53U]{};
  uint32_t yearMonth[5U * 12U]{};
  uint16_t scanCounter = 0;
};

bool visitAnalyticsBundle(const InterruptionAggregates::DailyRecord &record, void *context) {
  auto &ctx = *static_cast<AnalyticsBundleContext *>(context);
  servicePhysicalInputDuringRead(ctx.scanCounter);

  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  if (!ProjectTime::dayIndexToDate(record.dayIndex, year, month, day)) return true;

  bool isoComputed = false;
  uint16_t isoYear = 0;
  uint8_t isoWeek = 0;
  auto ensureIso = [&]() {
    if (!isoComputed) {
      isoComputed = ProjectTime::isoWeekForDate(year, month, day, isoYear, isoWeek);
    }
    return isoComputed;
  };

  bool includeHourly = false;
  if (ctx.hourlyWeekMode) {
    includeHourly = ensureIso() && isoYear == ctx.hourlyYear && isoWeek == ctx.hourlyWeek;
  } else {
    includeHourly = record.dayIndex >= ctx.hourlyFrom && record.dayIndex <= ctx.hourlyTo;
  }
  if (includeHourly) {
    const uint8_t row = ProjectTime::weekdayFromDayIndex(record.dayIndex);
    for (uint8_t hour = 0; hour < 24; ++hour) {
      const size_t index = static_cast<size_t>(row) * 24U + hour;
      ctx.hourly[index] += record.hours[hour];
    }
  }

  if (year == ctx.monthWeekYear && month >= 1 && month <= 12 &&
      ensureIso() && isoWeek >= 1 && isoWeek <= 53) {
    const size_t index = static_cast<size_t>(month - 1U) * 53U +
                         static_cast<size_t>(isoWeek - 1U);
    ctx.monthWeek[index] += record.total;
  }

  if (year >= ctx.yearMonthStart && year <= ctx.yearMonthEnd && month >= 1 && month <= 12) {
    const size_t index = static_cast<size_t>(year - ctx.yearMonthStart) * 12U +
                         static_cast<size_t>(month - 1U);
    ctx.yearMonth[index] += record.total;
  }
  return true;
}

void currentLocal(ProjectTime::LocalDateTime &local) {
  const TimeTypes::Snapshot now = TimeService::now();
  if (now.valid) ProjectTime::fromEpochMs(now.epochMs, local);
}

}  // namespace

void appendSummaryObject(String &out) {
  appendSummaryObjectInternal(out);
}

void appendProjectPreferencesObject(String &out) {
  appendProjectPreferencesObjectInternal(out);
}

String buildProjectPreferencesJson(bool ok) {
  String out;
  out.reserve(760);
  out += '{';
  fieldBool(out, "ok", ok);
  JsonUtils::appendKey(out, "projectSettings");
  appendProjectPreferencesObjectInternal(out);
  out += ',';
  JsonUtils::appendKey(out, "summary");
  appendSummaryObjectInternal(out);
  out += '}';
  return out;
}

String buildSummaryJson(bool ok) {
  String out;
  out.reserve(760);
  out += '{';
  fieldBool(out, "ok", ok);
  JsonUtils::appendKey(out, "summary");
  appendSummaryObjectInternal(out);
  out += '}';
  return out;
}

String buildStorageJson() {
  String out;
  out.reserve(760);
  out += '{';
  fieldBool(out, "ok", true);
  JsonUtils::appendKey(out, "storage");
  appendStorageObject(out);
  out += '}';
  return out;
}

String buildAnalyticsBundleJson(const char *hourlyMode,
                                uint16_t hourlyYear,
                                uint8_t hourlyWeek,
                                const char *fromDate,
                                const char *toDate,
                                uint16_t monthWeekYear,
                                bool &validRequest) {
  validRequest = false;
  if (monthWeekYear < 2020 || monthWeekYear > 2099) return String();

  static AnalyticsBundleContext ctx;
  ctx = AnalyticsBundleContext{};
  ctx.monthWeekYear = monthWeekYear;

  if (hourlyMode && strcmp(hourlyMode, "week") == 0) {
    if (hourlyYear < 2020 || hourlyYear > 2099 || hourlyWeek < 1 || hourlyWeek > 53) return String();
    ctx.hourlyWeekMode = true;
    ctx.hourlyYear = hourlyYear;
    ctx.hourlyWeek = hourlyWeek;
  } else if (hourlyMode && strcmp(hourlyMode, "range") == 0) {
    if (!ProjectTime::parseDate(fromDate, ctx.hourlyFrom) ||
        !ProjectTime::parseDate(toDate, ctx.hourlyTo) ||
        ctx.hourlyFrom > ctx.hourlyTo) {
      return String();
    }
  } else {
    return String();
  }

  ProjectTime::LocalDateTime local;
  currentLocal(local);
  uint16_t endYear = 0;
  if (local.valid) {
    endYear = local.year;
  } else {
    YearMonthContext probe;
    probe.onlyFindMax = true;
    InterruptionAggregates::forEach(visitYearMonth, &probe);
    endYear = probe.maxYear ? probe.maxYear : 2025;
  }
  ctx.yearMonthEnd = endYear;
  ctx.yearMonthStart = endYear >= 4 ? static_cast<uint16_t>(endYear - 4U) : endYear;

  if (!InterruptionAggregates::forEach(visitAnalyticsBundle, &ctx)) return String();
  validRequest = true;

  String out;
  out.reserve(9000);
  out += '{';
  fieldBool(out, "ok", true);

  JsonUtils::appendKey(out, "storage");
  appendStorageObject(out);
  out += ',';

  JsonUtils::appendKey(out, "hourly");
  out += '{';
  fieldBool(out, "ok", true);
  fieldUInt(out, "rows", 7);
  fieldUInt(out, "cols", 24);
  fieldInt(out, "currentRow", local.valid ? local.weekday : -1);
  fieldInt(out, "currentCol", local.valid ? local.hour : -1);
  JsonUtils::appendKey(out, "values");
  appendValues(out, ctx.hourly, 7U * 24U);
  out += '}';
  out += ',';

  JsonUtils::appendKey(out, "monthWeek");
  out += '{';
  fieldBool(out, "ok", true);
  fieldUInt(out, "rows", 12);
  fieldUInt(out, "cols", 53);
  fieldUInt(out, "year", monthWeekYear);
  const bool currentMonthWeekYear = local.valid && local.year == monthWeekYear;
  fieldInt(out,
           "currentRow",
           currentMonthWeekYear ? static_cast<int32_t>(local.month - 1U) : -1);
  fieldInt(out,
           "currentCol",
           currentMonthWeekYear ? static_cast<int32_t>(local.isoWeek - 1U) : -1);
  JsonUtils::appendKey(out, "values");
  appendValues(out, ctx.monthWeek, 12U * 53U);
  out += '}';
  out += ',';

  JsonUtils::appendKey(out, "yearMonth");
  out += '{';
  fieldBool(out, "ok", true);
  fieldUInt(out, "rows", 5);
  fieldUInt(out, "cols", 12);
  fieldUInt(out, "startYear", ctx.yearMonthStart);
  fieldUInt(out, "endYear", ctx.yearMonthEnd);
  fieldInt(out,
           "currentRow",
           local.valid && local.year >= ctx.yearMonthStart && local.year <= ctx.yearMonthEnd
               ? static_cast<int32_t>(local.year - ctx.yearMonthStart)
               : -1);
  fieldInt(out, "currentCol", local.valid ? static_cast<int32_t>(local.month - 1U) : -1);
  JsonUtils::appendKey(out, "values");
  appendValues(out, ctx.yearMonth, 5U * 12U);
  out += '}';

  out += '}';
  return out;
}

String buildHourlyHeatmapJson(const char *mode,
                              uint16_t year,
                              uint8_t week,
                              const char *fromDate,
                              const char *toDate,
                              bool &validRequest) {
  HourContext ctx;
  validRequest = false;

  if (mode && strcmp(mode, "week") == 0) {
    if (year < 2020 || year > 2099 || week < 1 || week > 53) return String();
    ctx.weekMode = true;
    ctx.year = year;
    ctx.week = week;
    validRequest = true;
  } else if (mode && strcmp(mode, "range") == 0) {
    uint16_t from = 0;
    uint16_t to = 0;
    if (!ProjectTime::parseDate(fromDate, from) ||
        !ProjectTime::parseDate(toDate, to) ||
        from > to) {
      return String();
    }
    ctx.from = from;
    ctx.to = to;
    validRequest = true;
  } else {
    return String();
  }

  InterruptionAggregates::forEach(visitHour, &ctx);
  ProjectTime::LocalDateTime local;
  currentLocal(local);

  String out;
  out.reserve(1800);
  out += '{';
  fieldBool(out, "ok", true);
  fieldUInt(out, "rows", 7);
  fieldUInt(out, "cols", 24);
  fieldInt(out, "currentRow", local.valid ? local.weekday : -1);
  fieldInt(out, "currentCol", local.valid ? local.hour : -1);
  JsonUtils::appendKey(out, "values");
  appendValues(out, ctx.values, 7U * 24U);
  out += '}';
  return out;
}

String buildMonthWeekHeatmapJson(uint16_t year, bool &validRequest) {
  validRequest = year >= 2020 && year <= 2099;
  if (!validRequest) return String();

  MonthWeekContext ctx;
  ctx.year = year;
  InterruptionAggregates::forEach(visitMonthWeek, &ctx);

  ProjectTime::LocalDateTime local;
  currentLocal(local);
  const bool currentYear = local.valid && local.year == year;

  String out;
  out.reserve(5600);
  out += '{';
  fieldBool(out, "ok", true);
  fieldUInt(out, "rows", 12);
  fieldUInt(out, "cols", 53);
  fieldUInt(out, "year", year);
  fieldInt(out, "currentRow", currentYear ? static_cast<int32_t>(local.month - 1U) : -1);
  fieldInt(out, "currentCol", currentYear ? static_cast<int32_t>(local.isoWeek - 1U) : -1);
  JsonUtils::appendKey(out, "values");
  appendValues(out, ctx.values, 12U * 53U);
  out += '}';
  return out;
}

String buildYearMonthHeatmapJson() {
  ProjectTime::LocalDateTime local;
  currentLocal(local);

  YearMonthContext ctx;
  uint16_t endYear = 0;
  if (local.valid) {
    endYear = local.year;
  } else {
    ctx.onlyFindMax = true;
    InterruptionAggregates::forEach(visitYearMonth, &ctx);
    endYear = ctx.maxYear ? ctx.maxYear : 2025;
  }

  const uint16_t startYear = endYear >= 4 ? static_cast<uint16_t>(endYear - 4U) : endYear;
  ctx = YearMonthContext{};
  ctx.startYear = startYear;
  ctx.endYear = endYear;
  InterruptionAggregates::forEach(visitYearMonth, &ctx);

  String out;
  out.reserve(1000);
  out += '{';
  fieldBool(out, "ok", true);
  fieldUInt(out, "rows", 5);
  fieldUInt(out, "cols", 12);
  fieldUInt(out, "startYear", startYear);
  fieldUInt(out, "endYear", endYear);
  fieldInt(out,
           "currentRow",
           local.valid && local.year >= startYear && local.year <= endYear
               ? static_cast<int32_t>(local.year - startYear)
               : -1);
  fieldInt(out, "currentCol", local.valid ? static_cast<int32_t>(local.month - 1U) : -1);
  JsonUtils::appendKey(out, "values");
  appendValues(out, ctx.values, 5U * 12U);
  out += '}';
  return out;
}

void streamCsv(WebServer &server) {
  const uint64_t first = InterruptionStore::oldestSequence();
  const uint64_t last = InterruptionStore::newestSequence();

  char filename[48] = "interruptions.csv";
  ProjectTime::LocalDateTime nowLocal;
  currentLocal(nowLocal);
  if (nowLocal.valid) {
    snprintf(filename,
             sizeof(filename),
             "interruptions_%04u%02u%02u.csv",
             nowLocal.year,
             nowLocal.month,
             nowLocal.day);
  }

  String disposition = "attachment; filename=\"";
  disposition += filename;
  disposition += '"';
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Content-Disposition", disposition);
  server.chunkResponseBegin("text/csv; charset=utf-8");

  static const char header[] =
      "sequence,timestamp_utc,timestamp_local,date_local,time_local,weekday,iso_week,"
      "time_source,event_source,delta_previous_same_day_seconds,relative_seconds,time_valid\r\n";
  server.chunkWrite(header, sizeof(header) - 1U);

  // Batch output to avoid one TCP/chunk write per CSV row. Between chunks the
  // critical local services are serviced explicitly because WebServer is
  // synchronous and this handler may run for several seconds for 100k rows.
  static char chunk[2048];
  size_t chunkLength = 0;

  auto serviceCriticalPaths = [&]() {
    HardwareRegistry::update();
    InterruptionService::update();
    TimeService::update();
    WifiModule::update();
    OtaModule::update();
    delay(0);  // scheduler/WDT yield, not a timed wait
  };

  auto flushChunk = [&]() {
    if (chunkLength == 0) return;
    server.chunkWrite(chunk, chunkLength);
    chunkLength = 0;
    serviceCriticalPaths();
  };

  char line[280];
  char utc[32];
  char localText[32];
  char date[16];
  char clock[12];

  for (uint64_t sequence = first; sequence != 0 && sequence <= last; ++sequence) {
    InterruptionTypes::RawEvent raw;
    if (!InterruptionStore::readSequence(sequence, raw)) continue;

    utc[0] = '\0';
    localText[0] = '\0';
    date[0] = '\0';
    clock[0] = '\0';
    int weekday = 0;
    int week = 0;
    unsigned long relative = 0;

    if (raw.absoluteValid) {
      ProjectTime::formatUtc(raw.timeValueSeconds, utc, sizeof(utc));
      ProjectTime::formatLocal(raw.timeValueSeconds, localText, sizeof(localText));
      ProjectTime::LocalDateTime local;
      if (ProjectTime::fromEpochSeconds(raw.timeValueSeconds, local)) {
        snprintf(date, sizeof(date), "%04u-%02u-%02u", local.year, local.month, local.day);
        snprintf(clock, sizeof(clock), "%02u:%02u:%02u", local.hour, local.minute, local.second);
        weekday = local.weekday + 1;
        week = local.isoWeek;
      }
    } else {
      relative = raw.timeValueSeconds;
    }

    char delta[16] = "";
    if (raw.deltaSeconds < InterruptionTypes::DELTA_UNKNOWN) {
      snprintf(delta, sizeof(delta), "%lu", static_cast<unsigned long>(raw.deltaSeconds));
    }

    const int length = snprintf(
        line,
        sizeof(line),
        "%llu,%s,%s,%s,%s,%d,%d,%s,%s,%s,%lu,%s\r\n",
        static_cast<unsigned long long>(sequence),
        utc,
        localText,
        date,
        clock,
        weekday,
        week,
        TimeTypes::sourceName(raw.timeSource),
        InterruptionTypes::eventSourceName(raw.eventSource),
        delta,
        relative,
        raw.absoluteValid ? "true" : "false");
    if (length <= 0) continue;

    const size_t safeLength = static_cast<size_t>(length) < sizeof(line)
                                  ? static_cast<size_t>(length)
                                  : sizeof(line) - 1U;
    if (chunkLength + safeLength > sizeof(chunk)) flushChunk();
    memcpy(chunk + chunkLength, line, safeLength);
    chunkLength += safeLength;
    if (chunkLength > sizeof(chunk) - sizeof(line)) flushChunk();
  }

  flushChunk();
  server.chunkResponseEnd();
}

}  // namespace InterruptionApi
