// LongTermArchive.ino
// Long-term archive for up to 100,000 absolute timestamps.
// The existing 10,000-entry ring remains untouched for fast recent/detail views.

static const char* LONGTERM_FILE = "/events_10y.bin";
static const uint32_t LONGTERM_MAGIC = 0x55494C31;
static const uint16_t LONGTERM_VERSION = 1;
static const uint32_t LONGTERM_CAPACITY = 100000;
static const uint8_t LONGTERM_CACHE_YEARS = 16;
static const uint8_t LONGTERM_WEEKS = 53;

struct __attribute__((packed)) LongTermHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t capacity;
  uint32_t writeIndex;
  uint32_t count;
};

static LongTermHeader longTermHeader = {};
static bool longTermReady = false;
static bool longTermInitialized = false;
static uint32_t longTermLastActionSeq = 0;

// Aggregates kept in RAM.
static uint16_t longTermWeekdayHour[7][24];
static uint16_t longTermMonthWeek[LONGTERM_CACHE_YEARS][12][LONGTERM_WEEKS];
static uint16_t longTermYearMonth[LONGTERM_CACHE_YEARS][12];
static bool longTermYearUsed[LONGTERM_CACHE_YEARS];
static int longTermCacheBaseYear = 0;
static uint32_t longTermCacheCount = 0xFFFFFFFFUL;
static uint32_t longTermCacheWriteIndex = 0xFFFFFFFFUL;

// Separate cache for one selected ISO week used by the weekday/hour heatmap.
static uint16_t longTermSelectedWeekdayHour[7][24];
static int longTermSelectedWeekYear = 0;
static int longTermSelectedWeek = 0;
static uint32_t longTermSelectedWeekCount = 0xFFFFFFFFUL;
static uint32_t longTermSelectedWeekWriteIndex = 0xFFFFFFFFUL;

static size_t longTermDataOffset(uint32_t index) {
  return sizeof(LongTermHeader) + (size_t)index * sizeof(uint32_t);
}

static bool longTermWriteHeader(File& f) {
  if (!f.seek(0, SeekSet)) return false;
  size_t n = f.write((const uint8_t*)&longTermHeader, sizeof(longTermHeader));
  f.flush();
  return n == sizeof(longTermHeader);
}

static bool longTermCreateFile() {
  File f = LittleFS.open(LONGTERM_FILE, FILE_WRITE);
  if (!f) return false;

  longTermHeader.magic = LONGTERM_MAGIC;
  longTermHeader.version = LONGTERM_VERSION;
  longTermHeader.reserved = 0;
  longTermHeader.capacity = LONGTERM_CAPACITY;
  longTermHeader.writeIndex = 0;
  longTermHeader.count = 0;

  if (!longTermWriteHeader(f)) {
    f.close();
    return false;
  }

  const size_t finalSize = sizeof(LongTermHeader) + (size_t)LONGTERM_CAPACITY * sizeof(uint32_t);
  if (!f.seek(finalSize - 1, SeekSet)) {
    f.close();
    return false;
  }
  uint8_t zero = 0;
  if (f.write(&zero, 1) != 1) {
    f.close();
    return false;
  }
  f.flush();
  f.close();
  return true;
}

static bool longTermLoadHeader() {
  if (!LittleFS.exists(LONGTERM_FILE)) return false;
  File f = LittleFS.open(LONGTERM_FILE, FILE_READ);
  if (!f) return false;
  bool ok = f.read((uint8_t*)&longTermHeader, sizeof(longTermHeader)) == sizeof(longTermHeader);
  f.close();
  return ok &&
         longTermHeader.magic == LONGTERM_MAGIC &&
         longTermHeader.version == LONGTERM_VERSION &&
         longTermHeader.capacity == LONGTERM_CAPACITY &&
         longTermHeader.writeIndex < LONGTERM_CAPACITY &&
         longTermHeader.count <= LONGTERM_CAPACITY;
}

static bool longTermReadChronologicalFromFile(File& f, uint32_t chronologicalIndex, uint32_t& value) {
  if (!longTermReady || chronologicalIndex >= longTermHeader.count) return false;
  uint32_t oldest = (longTermHeader.writeIndex + longTermHeader.capacity - longTermHeader.count) % longTermHeader.capacity;
  uint32_t physical = (oldest + chronologicalIndex) % longTermHeader.capacity;
  if (!f.seek(longTermDataOffset(physical), SeekSet)) return false;
  return f.read((uint8_t*)&value, sizeof(value)) == sizeof(value);
}

static void longTermInvalidateCaches() {
  longTermCacheCount = 0xFFFFFFFFUL;
  longTermCacheWriteIndex = 0xFFFFFFFFUL;
  longTermSelectedWeekCount = 0xFFFFFFFFUL;
  longTermSelectedWeekWriteIndex = 0xFFFFFFFFUL;
}

static bool longTermAppend(uint32_t ts) {
  if (!longTermReady || ts <= 1700000000UL) return false;
  File f = LittleFS.open(LONGTERM_FILE, "r+");
  if (!f) return false;

  if (!f.seek(longTermDataOffset(longTermHeader.writeIndex), SeekSet) ||
      f.write((const uint8_t*)&ts, sizeof(ts)) != sizeof(ts)) {
    f.close();
    return false;
  }

  longTermHeader.writeIndex = (longTermHeader.writeIndex + 1) % longTermHeader.capacity;
  if (longTermHeader.count < longTermHeader.capacity) longTermHeader.count++;
  bool ok = longTermWriteHeader(f);
  f.close();
  if (ok) longTermInvalidateCaches();
  return ok;
}

static bool longTermDeleteLast() {
  if (!longTermReady || longTermHeader.count == 0) return false;
  File f = LittleFS.open(LONGTERM_FILE, "r+");
  if (!f) return false;
  longTermHeader.writeIndex = (longTermHeader.writeIndex + longTermHeader.capacity - 1) % longTermHeader.capacity;
  longTermHeader.count--;
  bool ok = longTermWriteHeader(f);
  f.close();
  if (ok) longTermInvalidateCaches();
  return ok;
}

static bool longTermSeedFromNormalRing() {
  if (!longTermReady || longTermHeader.count != 0 || !ringOk || ringHeader.count == 0) return true;

  File in = LittleFS.open(RING_FILE, FILE_READ);
  File out = LittleFS.open(LONGTERM_FILE, "r+");
  if (!in || !out) {
    if (in) in.close();
    if (out) out.close();
    return false;
  }

  uint32_t copied = 0;
  for (uint32_t i = 0; i < ringHeader.count && copied < LONGTERM_CAPACITY; i++) {
    uint32_t ts = 0;
    if (!readChronologicalFromFile(in, i, ts) || ts <= 1700000000UL) continue;
    if (!out.seek(longTermDataOffset(longTermHeader.writeIndex), SeekSet) ||
        out.write((const uint8_t*)&ts, sizeof(ts)) != sizeof(ts)) {
      in.close();
      out.close();
      return false;
    }
    longTermHeader.writeIndex = (longTermHeader.writeIndex + 1) % longTermHeader.capacity;
    if (longTermHeader.count < longTermHeader.capacity) longTermHeader.count++;
    copied++;
  }

  bool ok = longTermWriteHeader(out);
  in.close();
  out.close();
  if (ok) {
    longTermInvalidateCaches();
    Serial.printf("[LANGZEIT] Initialisiert mit %lu vorhandenen Eintraegen.\n", (unsigned long)copied);
  }
  return ok;
}

static bool longTermInit() {
  if (longTermLoadHeader()) {
    longTermReady = true;
  } else {
    if (LittleFS.exists(LONGTERM_FILE)) LittleFS.remove(LONGTERM_FILE);
    if (!longTermCreateFile() || !longTermLoadHeader()) return false;
    longTermReady = true;
  }
  return longTermSeedFromNormalRing();
}

static int longTermDetermineBaseYear() {
  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm t;
    localtime_r(&now, &t);
    return t.tm_year + 1900;
  }

  if (longTermReady && longTermHeader.count) {
    File f = LittleFS.open(LONGTERM_FILE, FILE_READ);
    if (f) {
      uint32_t ts = 0;
      if (longTermReadChronologicalFromFile(f, longTermHeader.count - 1, ts) && ts > 1700000000UL) {
        time_t tv = (time_t)ts;
        struct tm t;
        localtime_r(&tv, &t);
        f.close();
        return t.tm_year + 1900;
      }
      f.close();
    }
  }
  return 2026;
}

static int longTermIsoWeek(const struct tm& t) {
  char weekBuf[4] = {0};
  strftime(weekBuf, sizeof(weekBuf), "%V", &t);
  int week = atoi(weekBuf);
  if (week < 1) week = 1;
  if (week > 53) week = 53;
  return week;
}

static int longTermIsoYear(const struct tm& t) {
  char yearBuf[8] = {0};
  if (strftime(yearBuf, sizeof(yearBuf), "%G", &t) > 0) {
    int year = atoi(yearBuf);
    if (year >= 2000 && year <= 2199) return year;
  }

  int year = t.tm_year + 1900;
  int week = longTermIsoWeek(t);
  if (t.tm_mon == 0 && week >= 52) return year - 1;
  if (t.tm_mon == 11 && week == 1) return year + 1;
  return year;
}

static void longTermCurrentIsoYearWeek(int& year, int& week) {
  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm t;
    localtime_r(&now, &t);
    year = longTermIsoYear(t);
    week = longTermIsoWeek(t);
    return;
  }

  year = longTermDetermineBaseYear();
  week = 1;
  if (longTermReady && longTermHeader.count) {
    File f = LittleFS.open(LONGTERM_FILE, FILE_READ);
    if (f) {
      uint32_t ts = 0;
      if (longTermReadChronologicalFromFile(f, longTermHeader.count - 1, ts) && ts > 1700000000UL) {
        time_t tv = (time_t)ts;
        struct tm t;
        localtime_r(&tv, &t);
        year = longTermIsoYear(t);
        week = longTermIsoWeek(t);
      }
      f.close();
    }
  }
}

static void longTermInc16(uint16_t& value) {
  if (value < 65535) value++;
}

static bool longTermRebuildAggregates() {
  if (!longTermReady) return false;
  if (longTermCacheCount == longTermHeader.count && longTermCacheWriteIndex == longTermHeader.writeIndex) return true;

  memset(longTermWeekdayHour, 0, sizeof(longTermWeekdayHour));
  memset(longTermMonthWeek, 0, sizeof(longTermMonthWeek));
  memset(longTermYearMonth, 0, sizeof(longTermYearMonth));
  memset(longTermYearUsed, 0, sizeof(longTermYearUsed));
  longTermCacheBaseYear = longTermDetermineBaseYear();

  File f = LittleFS.open(LONGTERM_FILE, FILE_READ);
  if (!f) return false;

  for (uint32_t i = 0; i < longTermHeader.count; i++) {
    uint32_t ts = 0;
    if (!longTermReadChronologicalFromFile(f, i, ts) || ts <= 1700000000UL) continue;

    time_t tv = (time_t)ts;
    struct tm t;
    localtime_r(&tv, &t);
    int year = t.tm_year + 1900;
    int yi = longTermCacheBaseYear - year;
    if (yi < 0 || yi >= LONGTERM_CACHE_YEARS) continue;

    longTermYearUsed[yi] = true;
    if (t.tm_mon >= 0 && t.tm_mon < 12) longTermInc16(longTermYearMonth[yi][t.tm_mon]);
    if (t.tm_wday >= 0 && t.tm_wday < 7 && t.tm_hour >= 0 && t.tm_hour < 24) {
      longTermInc16(longTermWeekdayHour[t.tm_wday][t.tm_hour]);
    }

    int week = longTermIsoWeek(t);
    if (t.tm_mon >= 0 && t.tm_mon < 12) longTermInc16(longTermMonthWeek[yi][t.tm_mon][week - 1]);
  }
  f.close();

  longTermCacheCount = longTermHeader.count;
  longTermCacheWriteIndex = longTermHeader.writeIndex;
  return true;
}

static bool longTermRebuildSelectedWeek(int selectedYear, int selectedWeek) {
  if (!longTermReady || selectedWeek < 1 || selectedWeek > 53) return false;
  if (longTermSelectedWeekYear == selectedYear &&
      longTermSelectedWeek == selectedWeek &&
      longTermSelectedWeekCount == longTermHeader.count &&
      longTermSelectedWeekWriteIndex == longTermHeader.writeIndex) return true;

  memset(longTermSelectedWeekdayHour, 0, sizeof(longTermSelectedWeekdayHour));
  File f = LittleFS.open(LONGTERM_FILE, FILE_READ);
  if (!f) return false;

  for (uint32_t i = 0; i < longTermHeader.count; i++) {
    uint32_t ts = 0;
    if (!longTermReadChronologicalFromFile(f, i, ts) || ts <= 1700000000UL) continue;
    time_t tv = (time_t)ts;
    struct tm t;
    localtime_r(&tv, &t);
    if (longTermIsoYear(t) != selectedYear || longTermIsoWeek(t) != selectedWeek) continue;
    if (t.tm_wday >= 0 && t.tm_wday < 7 && t.tm_hour >= 0 && t.tm_hour < 24) {
      longTermInc16(longTermSelectedWeekdayHour[t.tm_wday][t.tm_hour]);
    }
  }
  f.close();

  longTermSelectedWeekYear = selectedYear;
  longTermSelectedWeek = selectedWeek;
  longTermSelectedWeekCount = longTermHeader.count;
  longTermSelectedWeekWriteIndex = longTermHeader.writeIndex;
  return true;
}

static void longTermAppendUint16Array(String& json, const uint16_t* values, size_t count) {
  json += '[';
  for (size_t i = 0; i < count; i++) {
    if (i) json += ',';
    json += String(values[i]);
  }
  json += ']';
}

static String longTermAggregateJson(int selectedYear, int selectedWeekYear, int selectedWeek) {
  if (!longTermRebuildAggregates()) return "{\"ok\":false}";

  int selectedIndex = longTermCacheBaseYear - selectedYear;
  if (selectedIndex < 0 || selectedIndex >= LONGTERM_CACHE_YEARS) selectedIndex = 0;
  selectedYear = longTermCacheBaseYear - selectedIndex;

  int currentIsoYear = 0, currentIsoWeek = 0;
  longTermCurrentIsoYearWeek(currentIsoYear, currentIsoWeek);
  if (selectedWeekYear < 2000 || selectedWeekYear > 2199) selectedWeekYear = currentIsoYear;
  if (selectedWeek < 1 || selectedWeek > 53) selectedWeek = currentIsoWeek;
  if (!longTermRebuildSelectedWeek(selectedWeekYear, selectedWeek)) return "{\"ok\":false}";

  String json;
  json.reserve(9400);
  json = "{\"ok\":true";
  json += ",\"stored\":" + String(longTermHeader.count);
  json += ",\"capacity\":" + String(longTermHeader.capacity);
  json += ",\"baseYear\":" + String(longTermCacheBaseYear);
  json += ",\"selectedYear\":" + String(selectedYear);
  json += ",\"selectedWeekYear\":" + String(selectedWeekYear);
  json += ",\"selectedWeek\":" + String(selectedWeek);
  json += ",\"currentWeekYear\":" + String(currentIsoYear);
  json += ",\"currentWeek\":" + String(currentIsoWeek);

  json += ",\"years\":[";
  bool first = true;
  for (int i = 0; i < LONGTERM_CACHE_YEARS; i++) {
    if (!longTermYearUsed[i] && i != 0) continue;
    if (!first) json += ',';
    first = false;
    json += String(longTermCacheBaseYear - i);
  }
  if (currentIsoYear > longTermCacheBaseYear) {
    if (!first) json += ',';
    json += String(currentIsoYear);
  }
  json += ']';

  json += ",\"weekdayHour\":[";
  for (int d = 0; d < 7; d++) {
    if (d) json += ',';
    longTermAppendUint16Array(json, longTermSelectedWeekdayHour[d], 24);
  }
  json += ']';

  json += ",\"monthWeek\":[";
  for (int m = 0; m < 12; m++) {
    if (m) json += ',';
    longTermAppendUint16Array(json, longTermMonthWeek[selectedIndex][m], LONGTERM_WEEKS);
  }
  json += ']';

  json += ",\"yearMonth\":[";
  for (int y = 0; y < 5; y++) {
    if (y) json += ',';
    longTermAppendUint16Array(json, longTermYearMonth[y], 12);
  }
  json += "]}";
  return json;
}

static void longTermArchiveTick() {
  if (!longTermInitialized) {
    if (!fsOk || !ringOk) return;
    longTermReady = longTermInit();
    longTermInitialized = true;
    longTermLastActionSeq = uiActionSeq;
    Serial.printf("[LANGZEIT] %s | %lu/%lu Eintraege | ca. %.1f KiB Rohdaten\n",
                  longTermReady ? "OK" : "FEHLER",
                  (unsigned long)longTermHeader.count,
                  (unsigned long)LONGTERM_CAPACITY,
                  (double)(LONGTERM_CAPACITY * sizeof(uint32_t)) / 1024.0);
    return;
  }

  if (!longTermReady || uiActionSeq == longTermLastActionSeq) return;
  longTermLastActionSeq = uiActionSeq;

  // Autark events have relative time and remain in their dedicated autark ring.
  if (autarkMode) return;

  if (uiActionKind == 1) {
    uint32_t ts = getLastEvent();
    if (ts > 1700000000UL) {
      if (!longTermAppend(ts)) Serial.println("[LANGZEIT] FEHLER beim Speichern eines Eintrags.");
    }
  } else if (uiActionKind == 2) {
    if (!longTermDeleteLast()) Serial.println("[LANGZEIT] Hinweis: letzter Langzeit-Eintrag konnte nicht geloescht werden.");
  }
}

class LongTermApiRegistrar {
public:
  LongTermApiRegistrar() {
    server.on("/api/aggregate", HTTP_GET, []() {
      int year = server.hasArg("year") ? server.arg("year").toInt() : longTermDetermineBaseYear();
      int weekYear = server.hasArg("weekYear") ? server.arg("weekYear").toInt() : 0;
      int week = server.hasArg("week") ? server.arg("week").toInt() : 0;
      server.sendHeader("Cache-Control", "no-store");
      server.send(200, "application/json", longTermAggregateJson(year, weekYear, week));
    });
  }
};

LongTermApiRegistrar longTermApiRegistrar;
