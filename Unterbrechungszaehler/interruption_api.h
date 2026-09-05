#pragma once

#include <Arduino.h>
#include <WebServer.h>

namespace InterruptionApi {

void appendSummaryObject(String &out);
void appendProjectPreferencesObject(String &out);
void appendStorageObject(String &out);
String buildProjectPreferencesJson(bool ok = true);
String buildSummaryJson(bool ok = true);
String buildStorageJson();
String buildAnalyticsBundleJson(const char *metric,
                                const char *hourlyMode,
                                uint16_t hourlyYear,
                                uint8_t hourlyWeek,
                                const char *fromDate,
                                const char *toDate,
                                uint16_t monthWeekYear,
                                const char *source,
                                bool &validRequest);
String buildHourlyHeatmapJson(const char *mode, uint16_t year, uint8_t week, const char *fromDate, const char *toDate, bool &validRequest);
String buildMonthWeekHeatmapJson(uint16_t year, bool &validRequest);
String buildYearMonthHeatmapJson();
void streamCsv(WebServer &server);

}  // namespace InterruptionApi
