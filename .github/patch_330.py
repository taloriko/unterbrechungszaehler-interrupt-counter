from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Unterbrechungszaehler"


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    (ROOT / rel).write_text(text, encoding="utf-8")


def replace_once(rel, old, new):
    text = read(rel)
    if old not in text:
        raise SystemExit(f"missing replacement anchor in {rel}: {old[:100]!r}")
    text = text.replace(old, new, 1)
    write(rel, text)


def insert_before(rel, marker, addition):
    text = read(rel)
    if addition.strip() in text:
        return
    pos = text.find(marker)
    if pos < 0:
        raise SystemExit(f"missing insert marker in {rel}: {marker!r}")
    text = text[:pos] + addition + text[pos:]
    write(rel, text)


# ---------------------------------------------------------------------------
# Version
# ---------------------------------------------------------------------------
replace_once("Unterbrechungszaehler/config.h", 'SOFTWARE_VERSION[] = "3.2.0"', 'SOFTWARE_VERSION[] = "3.3.0"')

# ---------------------------------------------------------------------------
# Destructive database reset primitives. Delete aggregate files first, then raw
# files. The HTTP handler restarts immediately afterwards so every in-memory
# cache/summary is reconstructed from the new empty database on boot.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/interruption_store.h",
    "bool alignSequenceAtLeast(uint64_t durableHint);\n",
    "bool alignSequenceAtLeast(uint64_t durableHint);\n// Destructive maintenance operation used only after explicit project-name confirmation.\nbool eraseAll();\n",
)

insert_before(
    "Unterbrechungszaehler/interruption_store.cpp",
    "uint64_t oldestSequence() {",
    r'''bool eraseAll() {
  if (!mounted) {
    setStatus(StatusRegistry::State::Error, "LittleFS not mounted");
    return false;
  }

  recovery = RecoveryState{};
  readyFlag = false;
  rawFile.close();
  metaFile.close();

  const bool rawOk = !LittleFS.exists(ProjectConfig::RAW_DATA_PATH) || LittleFS.remove(ProjectConfig::RAW_DATA_PATH);
  const bool metaOk = !LittleFS.exists(ProjectConfig::RAW_META_PATH) || LittleFS.remove(ProjectConfig::RAW_META_PATH);
  meta = Meta{};
  updateInfo();
  refreshFsUsage(true);

  if (!rawOk || !metaOk) {
    setStatus(StatusRegistry::State::Error, "raw database delete failed");
    return false;
  }
  setStatus(StatusRegistry::State::Warning, "database erased; restart pending");
  SerialLog::warning("STORE", "Raw interruption database erased by explicit user request");
  return true;
}

''',
)

replace_once(
    "Unterbrechungszaehler/interruption_aggregates.h",
    "uint32_t countForDay(uint16_t dayIndex);\n",
    "uint32_t countForDay(uint16_t dayIndex);\n// Destructive maintenance operation; the caller restarts the device afterwards.\nbool eraseAll();\n",
)

insert_before(
    "Unterbrechungszaehler/interruption_aggregates.cpp",
    "bool find(uint16_t dayIndex, DailyRecord &recordOut) {",
    r'''bool eraseAll() {
  rebuildActive = false;
  readyFlag = false;
  cacheValid = false;
  dataFile.close();
  metaFile.close();

  const bool dataOk = !LittleFS.exists(ProjectConfig::DAILY_DATA_PATH) || LittleFS.remove(ProjectConfig::DAILY_DATA_PATH);
  const bool metaOk = !LittleFS.exists(ProjectConfig::DAILY_META_PATH) || LittleFS.remove(ProjectConfig::DAILY_META_PATH);
  meta = Meta{};
  rebuildSequence = 0;
  updateInfo();

  if (!dataOk || !metaOk) {
    setState("aggregate database delete failed");
    return false;
  }
  setState("database erased; restart pending");
  SerialLog::warning("STATS", "Daily aggregate database erased by explicit user request");
  return true;
}

''',
)

# ---------------------------------------------------------------------------
# Analytics API: source filter is based on the source already stored in every
# 9-byte raw record. `all` keeps the long-term aggregate path for count mode;
# a concrete source uses the retained raw ring and reports raw coverage.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/interruption_api.h",
    "void appendProjectPreferencesObject(String &out);\n",
    "void appendProjectPreferencesObject(String &out);\nvoid appendStorageObject(String &out);\n",
)
replace_once(
    "Unterbrechungszaehler/interruption_api.h",
    "                                uint16_t monthWeekYear,\n                                bool &validRequest);",
    "                                uint16_t monthWeekYear,\n                                const char *source,\n                                bool &validRequest);",
)

api_path = "Unterbrechungszaehler/interruption_api.cpp"
api = read(api_path)
api = api.replace("void appendStorageObject(String &out) {", "void appendStorageObjectInternal(String &out) {", 1)
api = api.replace("  fieldBool(out, \"recovering\", raw.recovering || daily.rebuilding, false);\n  out += '}';\n}", r'''  fieldBool(out, "recovering", raw.recovering || daily.rebuilding);
  fieldBool(out, "rawMounted", raw.mounted);
  fieldBool(out, "rawReady", raw.ready);
  fieldBool(out, "aggregateReady", daily.ready);
  if (raw.error && strcmp(raw.error, "none") != 0) fieldString(out, "rawError", raw.error);
  if (daily.error && strcmp(daily.error, "none") != 0) fieldString(out, "aggregateError", daily.error);

  const char *problemComponent = nullptr;
  const char *problem = nullptr;
  if (!raw.mounted) {
    problemComponent = "filesystem";
    problem = "LittleFS not mounted";
  } else if (raw.error && strcmp(raw.error, "none") != 0) {
    problemComponent = "raw";
    problem = raw.error;
  } else if (daily.error && strcmp(daily.error, "none") != 0) {
    problemComponent = "aggregate";
    problem = daily.error;
  } else if (summary.droppedCount > 0U) {
    problemComponent = "queue";
    problem = "events were not durably persisted in this boot";
  }
  if (problem) {
    fieldString(out, "problemComponent", problemComponent);
    fieldString(out, "problem", problem, false);
  } else {
    removeTrailingComma(out);
  }
  out += '}';
}''', 1)
if "problemComponent" not in api:
    raise SystemExit("storage diagnostics patch failed")

api = api.replace(
    "struct AnalyticsBundleContext {",
    r'''enum class AnalyticsSourceFilter : uint8_t {
  All,
  PhysicalButton,
  WebButton,
  Software,
  Api,
  Hardware,
  Unknown
};

bool parseAnalyticsSourceFilter(const char *text, AnalyticsSourceFilter &filter) {
  if (!text || !*text || strcmp(text, "all") == 0) { filter = AnalyticsSourceFilter::All; return true; }
  if (strcmp(text, "physical_button") == 0) { filter = AnalyticsSourceFilter::PhysicalButton; return true; }
  if (strcmp(text, "web_button") == 0) { filter = AnalyticsSourceFilter::WebButton; return true; }
  if (strcmp(text, "software") == 0) { filter = AnalyticsSourceFilter::Software; return true; }
  if (strcmp(text, "api") == 0) { filter = AnalyticsSourceFilter::Api; return true; }
  if (strcmp(text, "hardware") == 0) { filter = AnalyticsSourceFilter::Hardware; return true; }
  if (strcmp(text, "unknown") == 0) { filter = AnalyticsSourceFilter::Unknown; return true; }
  return false;
}

bool sourceMatches(AnalyticsSourceFilter filter, InterruptionTypes::EventSource source) {
  switch (filter) {
    case AnalyticsSourceFilter::All: return true;
    case AnalyticsSourceFilter::PhysicalButton: return source == InterruptionTypes::EventSource::PhysicalButton;
    case AnalyticsSourceFilter::WebButton: return source == InterruptionTypes::EventSource::WebButton;
    case AnalyticsSourceFilter::Software: return source == InterruptionTypes::EventSource::Software;
    case AnalyticsSourceFilter::Api: return source == InterruptionTypes::EventSource::Api;
    case AnalyticsSourceFilter::Hardware: return source == InterruptionTypes::EventSource::Hardware;
    case AnalyticsSourceFilter::Unknown: return source == InterruptionTypes::EventSource::Unknown;
  }
  return false;
}

const char *analyticsSourceFilterName(AnalyticsSourceFilter filter) {
  switch (filter) {
    case AnalyticsSourceFilter::PhysicalButton: return "physical_button";
    case AnalyticsSourceFilter::WebButton: return "web_button";
    case AnalyticsSourceFilter::Software: return "software";
    case AnalyticsSourceFilter::Api: return "api";
    case AnalyticsSourceFilter::Hardware: return "hardware";
    case AnalyticsSourceFilter::Unknown: return "unknown";
    case AnalyticsSourceFilter::All:
    default: return "all";
  }
}

struct AnalyticsBundleContext {''',
    1,
)
api = api.replace(
    "  bool averageInterval = false;\n",
    "  bool averageInterval = false;\n  bool rawCountScan = false;\n  AnalyticsSourceFilter sourceFilter = AnalyticsSourceFilter::All;\n",
    1,
)

count_helper = r'''void addCountSample(AnalyticsBundleContext &ctx,
                    const ProjectTime::LocalDateTime &eventLocal) {
  bool includeHourly = false;
  if (ctx.hourlyWeekMode) {
    includeHourly = eventLocal.isoYear == ctx.hourlyYear && eventLocal.isoWeek == ctx.hourlyWeek;
  } else {
    includeHourly = eventLocal.dayIndex >= ctx.hourlyFrom && eventLocal.dayIndex <= ctx.hourlyTo;
  }
  if (includeHourly) {
    const size_t index = static_cast<size_t>(eventLocal.weekday) * 24U + eventLocal.hour;
    ++ctx.hourly[index];
  }

  if (eventLocal.year == ctx.monthWeekYear && eventLocal.month >= 1U && eventLocal.month <= 12U &&
      eventLocal.isoWeek >= 1U && eventLocal.isoWeek <= 53U) {
    const size_t index = static_cast<size_t>(eventLocal.month - 1U) * 53U + static_cast<size_t>(eventLocal.isoWeek - 1U);
    ++ctx.monthWeek[index];
  }

  if (eventLocal.year >= ctx.yearMonthStart && eventLocal.year <= ctx.yearMonthEnd &&
      eventLocal.month >= 1U && eventLocal.month <= 12U) {
    const size_t index = static_cast<size_t>(eventLocal.year - ctx.yearMonthStart) * 12U +
                         static_cast<size_t>(eventLocal.month - 1U);
    ++ctx.yearMonth[index];
  }
}

'''
api = api.replace("void addIntervalSample(AnalyticsBundleContext &ctx,", count_helper + "void addIntervalSample(AnalyticsBundleContext &ctx,", 1)
api = api.replace("bool scanIntervalAnalytics(AnalyticsBundleContext &ctx) {", "bool scanRawAnalytics(AnalyticsBundleContext &ctx) {", 1)
api = api.replace(
    "      ctx.newestRawDayIndex = currentLocal.dayIndex;\n      ctx.newestRawEpochSeconds = current.timeValueSeconds;\n    }\n\n    if (previousUsable",
    "      ctx.newestRawDayIndex = currentLocal.dayIndex;\n      ctx.newestRawEpochSeconds = current.timeValueSeconds;\n      if (ctx.rawCountScan && sourceMatches(ctx.sourceFilter, current.eventSource)) addCountSample(ctx, currentLocal);\n    }\n\n    if (previousUsable",
    1,
)
api = api.replace(
    "        current.timeValueSeconds > previous.timeValueSeconds) {",
    "        current.timeValueSeconds > previous.timeValueSeconds &&\n        sourceMatches(ctx.sourceFilter, previous.eventSource)) {",
    1,
)

api = api.replace(
    "void appendProjectPreferencesObject(String &out) {\n  appendProjectPreferencesObjectInternal(out);\n}\n",
    "void appendProjectPreferencesObject(String &out) {\n  appendProjectPreferencesObjectInternal(out);\n}\n\nvoid appendStorageObject(String &out) {\n  appendStorageObjectInternal(out);\n}\n",
    1,
)
api = api.replace("  appendStorageObject(out);", "  appendStorageObjectInternal(out);", 1)

api = api.replace(
    "                                uint16_t monthWeekYear,\n                                bool &validRequest) {",
    "                                uint16_t monthWeekYear,\n                                const char *source,\n                                bool &validRequest) {",
    1,
)
api = api.replace(
    "  if (!averageInterval && !countMetric) return String();\n\n  static AnalyticsBundleContext ctx;",
    "  if (!averageInterval && !countMetric) return String();\n\n  AnalyticsSourceFilter sourceFilter = AnalyticsSourceFilter::All;\n  if (!parseAnalyticsSourceFilter(source, sourceFilter)) return String();\n\n  static AnalyticsBundleContext ctx;",
    1,
)
api = api.replace(
    "  ctx.averageInterval = averageInterval;\n  ctx.monthWeekYear = monthWeekYear;",
    "  ctx.averageInterval = averageInterval;\n  ctx.sourceFilter = sourceFilter;\n  ctx.rawCountScan = countMetric && sourceFilter != AnalyticsSourceFilter::All;\n  ctx.monthWeekYear = monthWeekYear;",
    1,
)
api = api.replace(
    "  const bool scanOk = averageInterval ? scanIntervalAnalytics(ctx)\n                                      : InterruptionAggregates::forEach(visitAnalyticsBundle, &ctx);",
    "  const bool rawBacked = averageInterval || ctx.rawCountScan;\n  const bool scanOk = rawBacked ? scanRawAnalytics(ctx)\n                                : InterruptionAggregates::forEach(visitAnalyticsBundle, &ctx);",
    1,
)
if "scanIntervalAnalytics" in api:
    raise SystemExit("old interval scan name still present")

# Add source to all three heatmap payloads and report raw coverage for filtered counts too.
api = api.replace(
    '  fieldString(out, "unit", averageInterval ? "seconds" : "count");\n  fieldUInt(out, "rows", 7);',
    '  fieldString(out, "unit", averageInterval ? "seconds" : "count");\n  fieldString(out, "source", analyticsSourceFilterName(ctx.sourceFilter));\n  fieldUInt(out, "rows", 7);',
    1,
)
api = api.replace(
    '  fieldString(out, "unit", averageInterval ? "seconds" : "count");\n  fieldUInt(out, "rows", 12);',
    '  fieldString(out, "unit", averageInterval ? "seconds" : "count");\n  fieldString(out, "source", analyticsSourceFilterName(ctx.sourceFilter));\n  fieldUInt(out, "rows", 12);',
    1,
)
api = api.replace(
    '  fieldString(out, "unit", averageInterval ? "seconds" : "count");\n  fieldUInt(out, "rows", 5);',
    '  fieldString(out, "unit", averageInterval ? "seconds" : "count");\n  fieldString(out, "source", analyticsSourceFilterName(ctx.sourceFilter));\n  fieldUInt(out, "rows", 5);',
    1,
)
# Coverage blocks are currently nested under averageInterval. Replace the three
# exact blocks with samples-only for averages plus coverage for every raw-backed result.
coverage_old = '''  if (averageInterval) {\n    out += ',';\n    JsonUtils::appendKey(out, "samples");\n    appendValues(out, ctx.{samples}, {count});\n    out += ',';\n    JsonUtils::appendKey(out, "coverage");\n    appendIntervalCoverage(out, ctx, intervalCoverageComplete(ctx, {start}));\n  }'''
coverage_new = '''  if (averageInterval) {\n    out += ',';\n    JsonUtils::appendKey(out, "samples");\n    appendValues(out, ctx.{samples}, {count});\n  }\n  if (rawBacked) {\n    out += ',';\n    JsonUtils::appendKey(out, "coverage");\n    appendIntervalCoverage(out, ctx, intervalCoverageComplete(ctx, {start}));\n  }'''
for samples, count, start in [
    ("hourlyIntervalSamples", "7U * 24U", "hourlyCoverageStart"),
    ("monthWeekIntervalSamples", "12U * 53U", "monthWeekCoverageStart"),
    ("yearMonthIntervalSamples", "5U * 12U", "yearMonthCoverageStart"),
]:
    old = coverage_old.format(samples=samples, count=count, start=start)
    new = coverage_new.format(samples=samples, count=count, start=start)
    if old not in api:
        raise SystemExit(f"coverage block missing for {samples}")
    api = api.replace(old, new, 1)

write(api_path, api)

# ---------------------------------------------------------------------------
# Device JSON includes project-storage diagnostics.
# ---------------------------------------------------------------------------
api_cpp = read("Unterbrechungszaehler/api.cpp")
old = '''appendUIntField(out, "usedPercent", otaUsedPercent, false);\n  out += '}';\n\n  out += '}';\n  return out;'''
new = '''appendUIntField(out, "usedPercent", otaUsedPercent, false);\n  out += '}';\n  out += ',';\n\n  JsonUtils::appendKey(out, "storage");\n  InterruptionApi::appendStorageObject(out);\n\n  out += '}';\n  return out;'''
if old not in api_cpp:
    raise SystemExit("api.cpp device storage anchor missing")
api_cpp = api_cpp.replace(old, new, 1)
write("Unterbrechungszaehler/api.cpp", api_cpp)

# ---------------------------------------------------------------------------
# Web routes: analytics source + password-confirmed database reset.
# ---------------------------------------------------------------------------
web_path = "Unterbrechungszaehler/web_server.cpp"
web = read(web_path)
web = web.replace(
    '  const String metric = server.arg("metric");\n  const String mode = server.arg("hourlyMode");',
    '  const String metric = server.arg("metric");\n  const String source = server.arg("source");\n  const String mode = server.arg("hourlyMode");',
    1,
)
web = web.replace(
    '      from.c_str(), to.c_str(), monthWeekYear, valid);',
    '      from.c_str(), to.c_str(), monthWeekYear, source.length() ? source.c_str() : "all", valid);',
    1,
)
reset_handler = r'''void restartAfterDatabaseMaintenance() {
  delay(250);
  ESP.restart();
}

void handleInterruptionStorageReset() {
  const String password = server.arg("password");
  if (password != AppConfig::PROJECT_NAME) {
    server.send(403, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"invalid_database_password\"}");
    return;
  }

  // Remove derived data first. If raw deletion then fails, a reboot can rebuild
  // the derived store from the still-preserved raw ring instead of leaving a
  // stale aggregate database behind.
  if (!InterruptionAggregates::eraseAll()) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"aggregate_database_delete_failed\"}");
    restartAfterDatabaseMaintenance();
    return;
  }
  if (!InterruptionStore::eraseAll()) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"raw_database_delete_failed\"}");
    restartAfterDatabaseMaintenance();
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"restarting\":true}");
  restartAfterDatabaseMaintenance();
}

'''
marker = "void sendProjectDataUnavailable(const char *reason) {"
if reset_handler.strip() not in web:
    pos = web.find(marker)
    if pos < 0:
        raise SystemExit("storage reset insert marker missing")
    web = web[:pos] + reset_handler + web[pos:]
web = web.replace(
    '  server.on("/api/interruptions/storage", HTTP_GET, handleInterruptionStorage);',
    '  server.on("/api/interruptions/storage", HTTP_GET, handleInterruptionStorage);\n  server.on("/api/interruptions/storage/reset", HTTP_POST, handleInterruptionStorageReset);',
    1,
)
write(web_path, web)

# ---------------------------------------------------------------------------
# Web UI: source selector, raw-coverage notice, exact storage diagnostics and
# explicit project-name/password database erase UI.
# ---------------------------------------------------------------------------
app_path = "Unterbrechungszaehler/ui-src/app.js"
js = read(app_path)
js = js.replace(
    "analytics: { loaded: false, loading: false, dirty: false, error: '', storage: null, hourly: null, monthWeek: null, yearMonth: null, hourlyMode: 'week', metric: 'count' }",
    "analytics: { loaded: false, loading: false, dirty: false, error: '', storage: null, hourly: null, monthWeek: null, yearMonth: null, hourlyMode: 'week', metric: 'count', source: 'all' }",
    1,
)
# Device state gets storage from /api/device.
js = js.replace(
    "memory: { heapTotal: null, heapFree: null, heapMin: null, heapUsedPercent: null },",
    "memory: { heapTotal: null, heapFree: null, heapMin: null, heapUsedPercent: null },\n    storage: {},",
    1,
)

translations = r'''
  const I18N_330 = {
    de: {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Der Herkunftsfilter basiert auf den noch vorhandenen Rohereignissen.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdaten-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löschen', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum vollständigen Löschen aller Ereignisse und Statistiken den Projektname eingeben.',
      'analytics.databaseDeleteAction': 'Datenbank vollständig löschen', 'analytics.databaseDeleteConfirm': 'Alle gespeicherten Ereignisse und Statistiken wirklich löschen?',
      'analytics.databaseDeleteWrong': 'Passwort stimmt nicht. Erwartet wird der Projektname.', 'analytics.databaseDeleteFailed': 'Datenbank konnte nicht vollständig gelöscht werden.',
      'analytics.databaseDeleteSuccess': 'Datenbank gelöscht. Gerät startet neu.'
    },
    en: {
      'analytics.source': 'Source', 'analytics.source.all': 'Both', 'analytics.source.physical_button': 'Button / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'The source filter is based on the raw events still retained on the device.',
      'analytics.storageState': 'Data storage', 'analytics.rawProblem': 'Raw-data problem', 'analytics.aggregateProblem': 'Statistics problem',
      'analytics.databaseDeleteTitle': 'Delete database', 'analytics.databaseDeletePassword': 'Password / project name',
      'analytics.databaseDeleteHint': 'Enter the project name to permanently delete all events and statistics.',
      'analytics.databaseDeleteAction': 'Delete complete database', 'analytics.databaseDeleteConfirm': 'Really delete all stored events and statistics?',
      'analytics.databaseDeleteWrong': 'Wrong password. The project name is required.', 'analytics.databaseDeleteFailed': 'The database could not be deleted completely.',
      'analytics.databaseDeleteSuccess': 'Database deleted. Device is restarting.'
    },
    it: {
      'analytics.source': 'Origine', 'analytics.source.all': 'Entrambi', 'analytics.source.physical_button': 'Pulsante / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Il filtro origine usa gli eventi grezzi ancora disponibili sul dispositivo.',
      'analytics.storageState': 'Memoria dati', 'analytics.rawProblem': 'Problema dati grezzi', 'analytics.aggregateProblem': 'Problema statistiche',
      'analytics.databaseDeleteTitle': 'Elimina database', 'analytics.databaseDeletePassword': 'Password / nome progetto',
      'analytics.databaseDeleteHint': 'Inserire il nome del progetto per eliminare definitivamente eventi e statistiche.',
      'analytics.databaseDeleteAction': 'Elimina tutto il database', 'analytics.databaseDeleteConfirm': 'Eliminare davvero tutti gli eventi e le statistiche?',
      'analytics.databaseDeleteWrong': 'Password errata. È richiesto il nome del progetto.', 'analytics.databaseDeleteFailed': 'Impossibile eliminare completamente il database.',
      'analytics.databaseDeleteSuccess': 'Database eliminato. Il dispositivo si riavvia.'
    },
    fr: {
      'analytics.source': 'Origine', 'analytics.source.all': 'Les deux', 'analytics.source.physical_button': 'Bouton / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Le filtre d’origine utilise les événements bruts encore conservés sur l’appareil.',
      'analytics.storageState': 'Stockage des données', 'analytics.rawProblem': 'Problème données brutes', 'analytics.aggregateProblem': 'Problème statistiques',
      'analytics.databaseDeleteTitle': 'Supprimer la base', 'analytics.databaseDeletePassword': 'Mot de passe / nom du projet',
      'analytics.databaseDeleteHint': 'Saisir le nom du projet pour supprimer définitivement tous les événements et statistiques.',
      'analytics.databaseDeleteAction': 'Supprimer toute la base', 'analytics.databaseDeleteConfirm': 'Supprimer vraiment tous les événements et statistiques ?',
      'analytics.databaseDeleteWrong': 'Mot de passe incorrect. Le nom du projet est requis.', 'analytics.databaseDeleteFailed': 'La base n’a pas pu être entièrement supprimée.',
      'analytics.databaseDeleteSuccess': 'Base supprimée. L’appareil redémarre.'
    },
    swg: {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Dr Herkunftsfilter nimmt bloß no die Rohereignis, wo no em Speicher send.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdata-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löscha', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum komplette Löscha vom ganze Zeug dr Projektname eigeba.',
      'analytics.databaseDeleteAction': 'Datenbank komplett löscha', 'analytics.databaseDeleteConfirm': 'Wirklich alle Ereignis ond Statistika löscha?',
      'analytics.databaseDeleteWrong': 'Passwort passt net. Dr Projektname wird braucht.', 'analytics.databaseDeleteFailed': 'D Datenbank hot sich net komplett löscha lassa.',
      'analytics.databaseDeleteSuccess': 'Datenbank isch weg. S Gerät startet neu.'
    },
    'swg-alb': {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Dr Herkunftsfilter nimmt dia Rohereignis, wo no em Speicher send.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdata-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löscha', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum komplette Löscha dr Projektname eigeba.',
      'analytics.databaseDeleteAction': 'Datenbank komplett löscha', 'analytics.databaseDeleteConfirm': 'Wirklich alles löscha?',
      'analytics.databaseDeleteWrong': 'Passwort passt net. Dr Projektname wird braucht.', 'analytics.databaseDeleteFailed': 'D Datenbank hot sich net komplett löscha lassa.',
      'analytics.databaseDeleteSuccess': 'Datenbank isch weg. S Gerät startet neu.'
    },
    'swg-ob': {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Dr Herkunftsfilter nimmt bloß no dia Rohereignis, wo no gspeichert send.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdata-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löscha', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum komplette Löscha dr Projektname eigeba.',
      'analytics.databaseDeleteAction': 'Datenbank komplett löscha', 'analytics.databaseDeleteConfirm': 'Wirklich alle Ereignis ond Statistika löscha?',
      'analytics.databaseDeleteWrong': 'Passwort passt it. Dr Projektname wird braucht.', 'analytics.databaseDeleteFailed': 'D Datenbank hot sich it komplett löscha lassa.',
      'analytics.databaseDeleteSuccess': 'Datenbank isch weg. S Gerät startet neu.'
    }
  };
  Object.entries(I18N_330).forEach(([code, labels]) => Object.assign(I18N[code], labels));

'''
lang_marker = "  const LANGUAGE_LABELS = {"
if "const I18N_330" not in js:
    pos = js.find(lang_marker)
    if pos < 0:
        raise SystemExit("language labels marker missing")
    js = js[:pos] + translations + js[pos:]

# Device memory card: storage status plus exact backend diagnostics (hidden when absent).
js = js.replace(
    "            { type: 'meter', labelKey: 'label.heapUsed', path: 'memory.heapUsedPercent', policy: 'heapUsed', format: 'percent' }\n          ] },",
    "            { type: 'meter', labelKey: 'label.heapUsed', path: 'memory.heapUsedPercent', policy: 'heapUsed', format: 'percent' },\n            { type: 'status', labelKey: 'analytics.storageState', path: 'storage.state' },\n            { type: 'kv', items: [\n              { labelKey: 'analytics.rawProblem', path: 'storage.rawError' },\n              { labelKey: 'analytics.aggregateProblem', path: 'storage.aggregateError' }\n            ] }\n          ] },",
    1,
)

source_field = r'''  function createAnalyticsSourceField() {
    const wrap = el('label', 'analytics-filter-field');
    const label = el('span'); label.textContent = t('analytics.source');
    const select = el('select'); select.dataset.analyticsSource = '1';
    for (const [value, key] of [['all','analytics.source.all'],['physical_button','analytics.source.physical_button'],['web_button','analytics.source.web_button']]) {
      const option = el('option'); option.value = value; option.textContent = t(key); select.append(option);
    }
    select.value = state.analytics.source || 'all';
    wrap.append(label, select);
    return { wrap, select };
  }

'''
js = js.replace("  function renderHeatmapHourly() {", source_field + "  function renderHeatmapHourly() {", 1)
js = js.replace(
    "    const metricField = createAnalyticsMetricField();\n    const modeWrap",
    "    const metricField = createAnalyticsMetricField();\n    const sourceField = createAnalyticsSourceField();\n    const modeWrap",
    1,
)
js = js.replace("    controls.append(metricField.wrap,modeWrap,yearField.wrap,weekField.wrap,fromField.wrap,toField.wrap,button);", "    controls.append(metricField.wrap,sourceField.wrap,modeWrap,yearField.wrap,weekField.wrap,fromField.wrap,toField.wrap,button);", 1)
js = js.replace(
    "    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); const year=createFilterField",
    "    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); const sourceField=createAnalyticsSourceField(); const year=createFilterField",
    1,
)
js = js.replace("controls.append(metricField.wrap,year.wrap,button);", "controls.append(metricField.wrap,sourceField.wrap,year.wrap,button);", 1)
js = js.replace(
    "    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); controls.append(metricField.wrap);",
    "    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); const sourceField=createAnalyticsSourceField(); controls.append(metricField.wrap,sourceField.wrap);",
    1,
)

# Raw coverage note applies to average intervals and to source-filtered counts.
js = js.replace(
    "    if (isAverage && data.coverage?.complete === false) {\n      const coverage = el('div', 'form-note heatmap-coverage');\n      coverage.textContent = t('analytics.coveragePartial');",
    "    if (data.coverage?.complete === false) {\n      const coverage = el('div', 'form-note heatmap-coverage');\n      coverage.textContent = data.source && data.source !== 'all' ? t('analytics.sourceCoveragePartial') : t('analytics.coveragePartial');",
    1,
)

# Replace analytics storage renderer with diagnostics + password-protected erase.
start = js.find("  function renderAnalyticsStorage() {")
end = js.find("\n  function renderComponent(def) {", start)
if start < 0 or end < 0:
    raise SystemExit("analytics storage renderer anchors missing")
new_storage_renderer = r'''  function renderAnalyticsStorage() {
    const root=el('div','analytics-storage');
    const dl=el('dl','kv-list');
    const rows={};
    for(const [key,labelKey] of [['raw','analytics.rawEvents'],['daily','analytics.dailyRecords'],['used','analytics.storageUsed'],['unassigned','analytics.unassigned'],['dropped','analytics.dropped']]){
      const row=el('div','kv-row'); const dt=el('dt');dt.textContent=t(labelKey); const dd=el('dd'); row.append(dt,dd); dl.append(row); rows[key]=dd;
    }
    const problem=el('div','notice'); problem.dataset.kind='warning'; problem.hidden=true; problem.append(icon('warning')); const problemText=el('p'); problem.append(problemText);
    const hint=el('div','form-note');hint.textContent=t('analytics.ringHint');
    const actions=el('div','action-row'); const button=el('button','button primary-button');button.type='button';button.dataset.analyticsDownload='1';button.append(icon('download'));const bt=el('span');bt.textContent=t('analytics.download');button.append(bt);actions.append(button);

    const danger=el('section','time-section');
    const dangerTitle=el('h3'); dangerTitle.textContent=t('analytics.databaseDeleteTitle');
    const dangerNote=el('div','form-note'); dangerNote.textContent=t('analytics.databaseDeleteHint');
    const field=el('label','form-control'); const fieldLabel=el('span'); fieldLabel.textContent=t('analytics.databaseDeletePassword');
    const password=el('input','text-input'); password.type='password'; password.autocomplete='off'; password.spellcheck=false; password.dataset.databasePassword='1'; field.append(fieldLabel,password);
    const dangerActions=el('div','action-row'); const erase=el('button','button danger'); erase.type='button'; erase.append(icon('trash')); const eraseText=el('span'); eraseText.textContent=t('analytics.databaseDeleteAction'); erase.append(eraseText); dangerActions.append(erase);
    const eraseMessage=el('div','project-setting-message'); eraseMessage.setAttribute('aria-live','polite');
    erase.addEventListener('click', async()=>{
      if(!confirm(t('analytics.databaseDeleteConfirm'))) return;
      erase.disabled=true; password.disabled=true; eraseMessage.textContent='';
      try{
        await Transport.eraseDatabase(password.value);
        eraseMessage.dataset.state='ok'; eraseMessage.textContent=t('analytics.databaseDeleteSuccess');
      }catch(error){
        eraseMessage.dataset.state='error';
        eraseMessage.textContent=error?.code==='invalid_database_password'?t('analytics.databaseDeleteWrong'):t('analytics.databaseDeleteFailed');
        erase.disabled=false; password.disabled=false;
      }
    });
    danger.append(dangerTitle,dangerNote,field,dangerActions,eraseMessage);
    root.append(dl,problem,hint,actions,danger);
    Bindings.add('analytics.storage',()=>{
      const s=state.analytics.storage||{};
      rows.raw.textContent=`${Number(s.rawCount||0).toLocaleString()} / ${Number(s.rawCapacity||0).toLocaleString()}`;
      rows.daily.textContent=`${Number(s.dailyCount||0).toLocaleString()} / ${Number(s.dailyCapacity||0).toLocaleString()}`;
      rows.used.textContent=s.fsTotalBytes?`${Formats.bytes(s.fsUsedBytes)} / ${Formats.bytes(s.fsTotalBytes)}`:t('common.none');
      rows.unassigned.textContent=String(s.unassignedCount||0); rows.dropped.textContent=String(s.droppedCount||0);
      const detail=s.problem||s.rawError||s.aggregateError||'';
      problem.hidden=!detail; problemText.textContent=detail?`${t('analytics.storageState')}: ${detail}`:'';
    });
    return root;
  }
'''
js = js[:start] + new_storage_renderer + js[end:]

# Transport: source in bundle query; storage reset method; device storage payload.
js = js.replace(
    "      const metric = state.analytics.metric || 'count';\n      const parts = [`metric=${encodeURIComponent(metric)}`, `hourlyMode=${encodeURIComponent(mode)}`];",
    "      const metric = state.analytics.metric || 'count';\n      const source = state.analytics.source || 'all';\n      const parts = [`metric=${encodeURIComponent(metric)}`, `source=${encodeURIComponent(source)}`, `hourlyMode=${encodeURIComponent(mode)}`];",
    1,
)
js = js.replace(
    "    async loadAnalyticsStorage() {\n      const data = await this.request('/api/interruptions/storage');\n      patchState({ analytics: { storage: data.storage || null } });\n      Bindings.notify('analytics.storage');\n    },",
    r'''    async loadAnalyticsStorage() {
      const data = await this.request('/api/interruptions/storage');
      patchState({ analytics: { storage: data.storage || null } });
      Bindings.notify('analytics.storage');
    },
    async eraseDatabase(password) {
      const body = new URLSearchParams({ password: String(password || '') }).toString();
      const data = await this.requestResult('/api/interruptions/storage/reset', {
        method: 'POST',
        headers: { Accept: 'application/json', 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' },
        body
      });
      setTimeout(() => location.reload(), 3500);
      return data;
    },''',
    1,
)
js = js.replace(
    "patchState({ device: data.device || {}, wifi: data.wifi || {}, memory: data.memory || {}, hardware: data.hardware || { checking: false, modules: [] }, ota: data.ota || {}, status: statusPatch });",
    "patchState({ device: data.device || {}, wifi: data.wifi || {}, memory: data.memory || {}, storage: data.storage || {}, hardware: data.hardware || { checking: false, modules: [] }, ota: data.ota || {}, status: statusPatch });",
    1,
)

# Source filtered count must use bundle/raw path; unfiltered count keeps compact targeted legacy endpoints.
js = js.replace(
    "      if ((state.analytics.metric || 'count') === 'averageInterval') await Transport.loadAnalytics(true);",
    "      if ((state.analytics.metric || 'count') === 'averageInterval' || (state.analytics.source || 'all') !== 'all') await Transport.loadAnalytics(true);",
    1,
)

source_listener = r'''    const sourceControl = event.target.closest('[data-analytics-source]');
    if (sourceControl) {
      const allowed = ['all','physical_button','web_button'];
      state.analytics.source = allowed.includes(sourceControl.value) ? sourceControl.value : 'all';
      for (const control of document.querySelectorAll('[data-analytics-source]')) control.value = state.analytics.source;
      state.analytics.dirty = true;
      Transport.loadAnalytics(true);
      return;
    }
'''
js = js.replace("    const projectSetting = event.target.closest('[data-project-setting]');", source_listener + "    const projectSetting = event.target.closest('[data-project-setting]');", 1)
write(app_path, js)

# ---------------------------------------------------------------------------
# Release checks and documentation
# ---------------------------------------------------------------------------
rc_path = "Unterbrechungszaehler/tools/release_check.py"
rc = read(rc_path)
rc = rc.replace("Unterbrechungszaehler 3.2.0", "Unterbrechungszaehler 3.3.0", 1)
rc = rc.replace('SOFTWARE_VERSION[] = "3.2.0"', 'SOFTWARE_VERSION[] = "3.3.0"', 1)
rc = rc.replace('"project version 3.2.0"', '"project version 3.3.0"', 1)
rc = rc.replace('f"3.2.0 UI additions present for {language}"', 'f"3.3.0 UI additions present for {language}"', 1)
extra_checks = r'''
    check("/api/interruptions/storage/reset" in web_server and "AppConfig::PROJECT_NAME" in web_server, "project-name protected database reset route")
    store_cpp = (ROOT / "interruption_store.cpp").read_text(encoding="utf-8")
    aggregates_cpp = (ROOT / "interruption_aggregates.cpp").read_text(encoding="utf-8")
    check("bool eraseAll()" in store_cpp and "raw database delete failed" in store_cpp, "raw database destructive reset")
    check("bool eraseAll()" in aggregates_cpp and "aggregate database delete failed" in aggregates_cpp, "aggregate database destructive reset")
    check("event.eventSource" in store_cpp and "<< 20" in store_cpp, "event source remains persisted in 9-byte raw record")
    check("physical_button" in interruption_api and "web_button" in interruption_api and "sourceMatches" in interruption_api, "raw event-source analytics filter")
    check("rawCountScan" in interruption_api and "addCountSample" in interruption_api, "source-filtered count heatmaps use retained raw ring")
    check("previous.eventSource" in interruption_api, "average interval source filter follows interval start event")
    check("rawError" in interruption_api and "aggregateError" in interruption_api and "problemComponent" in interruption_api, "exact storage diagnostics exposed")
    check("analytics.source.physical_button" in JS and "analytics.source.web_button" in JS and "data-analytics-source" in JS, "heatmap source selector")
    check("databaseDeletePassword" in JS and "eraseDatabase" in JS, "password-confirmed database erase UI")
'''
anchor = '    check(all(route in web_server for route in expected_routes), "project API routes")\n'
if anchor not in rc:
    raise SystemExit("release check route anchor missing")
rc = rc.replace(anchor, anchor + extra_checks, 1)
rc = rc.replace('        "/api/interruptions/storage",\n', '        "/api/interruptions/storage",\n        "/api/interruptions/storage/reset",\n', 1)
write(rc_path, rc)

# Changelog
chg_path = "CHANGELOG.md"
chg = read(chg_path)
entry = '''## 3.3.0\n\n- Datenbank kann nach Eingabe des exakten Projektnamens vollständig gelöscht werden; Rohdaten und Tagesaggregate werden entfernt und das Gerät startet sauber neu\n- Heatmaps erhalten einen Herkunftsfilter: **Beides** (Standard), **Knopf / GPIO** oder **Web**; das API-Modell bleibt für weitere Quellen wie API/Software/Hardware erweiterbar\n- Herkunft ist weiterhin Bestandteil jedes kompakten 9-Byte-Rohdatensatzes; gefilterte Heatmaps werden ehrlich aus dem retained Raw-Ring berechnet und melden unvollständige Abdeckung\n- Ø-Abstand behält die 3.1-Regel bei: der Filter bezieht sich auf die Start-Unterbrechung, der letzte Druck des Tages bleibt ausgeschlossen und es wird nie über Mitternacht gerechnet\n- Gerät → Speicher zeigt bei Fehlern die konkrete Rohdaten-/Statistik-Ursache statt nur einen allgemeinen Fehlerzustand\n- Webbundle, API-Routen und Releasechecks auf 3.3.0 erweitert\n\n'''
if "## 3.3.0" not in chg:
    chg = chg.replace("# Changelog\n\n", "# Changelog\n\n" + entry, 1)
write(chg_path, chg)

# Root + language READMEs: current version and concise 3.3 feature section.
readme_sections = {
    "README.md": '''\n## Datenpflege & Herkunftsfilter in 3.3.0\n\nDie Heatmaps lassen sich zusätzlich nach der Herkunft filtern: **Beides** (Standard), **Knopf / GPIO** oder **Web**. Die Herkunft steckt bereits im kompakten Rohdatensatz. Sobald nach einer einzelnen Herkunft gefiltert wird, wertet das Gerät deshalb den noch vorhandenen Roh-Ringspeicher aus und weist auf eine eventuell unvollständige Abdeckung hin. Das Datenformat wird dafür nicht aufgebläht und bleibt offen für spätere Quellen wie API.\n\nUnter **Daten & Export** kann die komplette Ereignisdatenbank bewusst gelöscht werden. Als Schutz gegen versehentliches Löschen muss exakt der Projektname `Unterbrechungszähler` eingegeben werden. Danach werden Rohereignisse und Tagesaggregate entfernt und das Gerät startet neu. Unter **Gerät → Speicher** wird bei einem Speicherproblem zusätzlich die konkrete interne Fehlerursache angezeigt.\n''',
    "docs/de/README.md": '''\n## Neu in 3.3.0\n\nHeatmaps können nach **Beides**, **Knopf / GPIO** oder **Web** gefiltert werden. Einzelne Herkunftsfilter arbeiten aus dem noch vorhandenen Roh-Ringspeicher und zeigen eine unvollständige Abdeckung offen an. Die komplette Datenbank kann nach Eingabe des Projektnamens `Unterbrechungszähler` gelöscht werden; danach startet das Gerät neu. Gerät → Speicher zeigt bei Fehlern die konkrete Ursache für Rohdaten oder Tagesstatistik.\n''',
    "docs/en/README.md": '''\n## New in 3.3.0\n\nHeatmaps can be filtered by **Both**, **Button / GPIO**, or **Web**. A single-source filter is calculated from the retained raw-event ring and explicitly reports incomplete coverage. The complete event database can be erased after entering the exact project name `Unterbrechungszähler`; the device then restarts. Device → Memory also shows the concrete raw-data or aggregate-storage error when storage is unhealthy.\n''',
    "docs/swg/README.md": '''\n## Neu in 3.3.0\n\nBei de Heatmaps kannsch jetzt **Beides**, **Knopf / GPIO** oder **Web** auswähla. Wenn bloß oine Herkunft gfiltert wird, nimmt s Gerät die Rohereignis, wo no em Ringspeicher send, ond zeigt ehrlich an, wenn dr Zeitraum nimme komplett drin isch. D komplette Datenbank kannsch mit em exakta Projektname `Unterbrechungszähler` löscha; danach startet s Gerät neu. Bei Gerät → Speicher steht bei Fehler jetzt au dr konkrete Grund dabei.\n''',
}
for rel, section in readme_sections.items():
    text = read(rel)
    text = text.replace("`3.2.0`", "`3.3.0`", 1)
    if "3.3.0" not in text or section.strip() not in text:
        text += section
    write(rel, text)

# Release notes prepend 3.3.0 summary without destroying historical notes.
rn_path = "Unterbrechungszaehler/RELEASE_NOTES.md"
rn = read(rn_path)
if "3.3.0" not in rn[:500]:
    rn = "# Release 3.3.0\n\n- Passwortgeschütztes vollständiges Löschen der Ereignisdatenbank (Bestätigung = Projektname)\n- Heatmap-Herkunftsfilter für Beides / Knopf-GPIO / Web, mit offenem API-Modell für spätere Quellen\n- konkrete Speicherfehler in Gerät → Speicher\n- bestehende 3.1/3.2-Regeln für Ø-Abstand, Raw-Ring-Coverage, Embedded-Effizienz und nicht blockierende Bedienung bleiben erhalten\n\n" + rn
write(rn_path, rn)

# Storage format documentation: source was already stored; document how 3.3 uses it.
sf_path = "Unterbrechungszaehler/STORAGE_FORMAT.md"
sf = read(sf_path)
if "## 3.3.0 Herkunftsfilter und Löschfunktion" not in sf:
    sf += '''\n## 3.3.0 Herkunftsfilter und Löschfunktion\n\nDer bereits im 9-Byte-RawEvent gespeicherte `eventSource` wird nun direkt für Heatmap-Filter verwendet. **Beides** kann weiterhin die kompakten Langzeit-Tagesaggregate nutzen. Ein einzelner Herkunftsfilter (z. B. GPIO oder Web) wird aus dem retained Raw-Ring berechnet; dadurch wird das bestehende Tagesformat nicht vergrößert. Ist der angefragte Zeitraum älter als die Rohdatenabdeckung, meldet die API `coverage.complete=false` statt fehlende Daten als Null zu erfinden.\n\nDie manuelle Datenbank-Löschung entfernt zuerst die abgeleiteten Tagesaggregate und danach Rohdaten samt Metadaten. Anschließend startet der ESP32 neu und erzeugt leere, konsistente Datenstrukturen. Die Aktion wird serverseitig nur akzeptiert, wenn die Bestätigung exakt dem Projektnamen entspricht.\n'''
write(sf_path, sf)

print("3.3.0 patch applied")
