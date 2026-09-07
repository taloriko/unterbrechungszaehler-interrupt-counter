#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "Unterbrechungszaehler"


def text(path):
    return path.read_text(encoding="utf-8")


def write(path, value):
    path.write_text(value, encoding="utf-8")


def replace_once(path, old, new):
    value = text(path)
    count = value.count(old)
    if count != 1:
        raise RuntimeError(f"expected one anchor in {path}, got {count}: {old[:80]!r}")
    write(path, value.replace(old, new, 1))


# Version.
replace_once(FW / "config.h", 'constexpr char SOFTWARE_VERSION[] = "3.4.1";', 'constexpr char SOFTWARE_VERSION[] = "3.5.0";')

# Central constants for the bounded cache and OLED page rotation.
replace_once(
    FW / "project_config.h",
    'constexpr uint32_t LIVE_POLL_INTERVAL_MS = 1000;\n',
    'constexpr uint32_t LIVE_POLL_INTERVAL_MS = 1000;\n\n'
    '// Focus & Insights scans only the retained horizon needed for the current\n'
    '// week, the 120-minute trend and up to 30 completed local days. Results\n'
    '// are cached in RAM; no additional persistent data is written.\n'
    'constexpr uint32_t FOCUS_INSIGHTS_CACHE_MAX_AGE_MS = 60000;\n'
    'constexpr uint8_t FOCUS_PATTERN_DAYS = 30;\n'
    'constexpr uint8_t FOCUS_PATTERN_MIN_COVERED_DAYS = 5;\n'
    'constexpr uint32_t DISPLAY_INSIGHTS_PAGE_MS = 4000;\n'
)

# Use central constants in the new cache module.
replace_once(FW / "focus_insights.cpp", '#include "project_time.h"\n', '#include "project_config.h"\n#include "project_time.h"\n')
replace_once(FW / "focus_insights.cpp", 'constexpr uint32_t CACHE_MAX_AGE_MS = 60000U;\nconstexpr uint8_t PATTERN_DAYS = 30U;\nconstexpr uint8_t MIN_COVERED_DAYS = 5U;', 'constexpr uint32_t CACHE_MAX_AGE_MS = ProjectConfig::FOCUS_INSIGHTS_CACHE_MAX_AGE_MS;\nconstexpr uint8_t PATTERN_DAYS = ProjectConfig::FOCUS_PATTERN_DAYS;\nconstexpr uint8_t MIN_COVERED_DAYS = ProjectConfig::FOCUS_PATTERN_MIN_COVERED_DAYS;')

# Display modes are appended so all persisted existing numeric values remain unchanged.
replace_once(
    FW / "project_preferences.h",
    '  DayProgress = 3,\n  Focus = 4\n',
    '  DayProgress = 3,\n  Focus = 4,\n  QuietPhases = 5,\n  WorkPatterns = 6\n'
)
replace_once(
    FW / "project_preferences.cpp",
    '  return raw <= static_cast<uint8_t>(DisplayMode::Focus)\n',
    '  return raw <= static_cast<uint8_t>(DisplayMode::WorkPatterns)\n'
)
replace_once(
    FW / "project_preferences.cpp",
    '    case DisplayMode::Focus: return "focus";\n    case DisplayMode::Standard:',
    '    case DisplayMode::Focus: return "focus";\n    case DisplayMode::QuietPhases: return "quiet-phases";\n    case DisplayMode::WorkPatterns: return "work-patterns";\n    case DisplayMode::Standard:'
)
replace_once(
    FW / "project_preferences.cpp",
    '  else if (strcmp(value, "focus") == 0) parsed = DisplayMode::Focus;\n  else return false;',
    '  else if (strcmp(value, "focus") == 0) parsed = DisplayMode::Focus;\n  else if (strcmp(value, "quiet-phases") == 0) parsed = DisplayMode::QuietPhases;\n  else if (strcmp(value, "work-patterns") == 0) parsed = DisplayMode::WorkPatterns;\n  else return false;'
)

# Make the central cache part of normal service lifecycle, after priority feedback/persistence.
replace_once(FW / "interruption_service.cpp", '#include "display_views.h"\n', '#include "display_views.h"\n#include "focus_insights.h"\n')
replace_once(FW / "interruption_service.cpp", '  InterruptionStore::begin();\n', '  InterruptionStore::begin();\n  FocusInsights::begin();\n')
replace_once(FW / "interruption_service.cpp", '  popQueue();\n  refreshStorageState();\n', '  popQueue();\n  FocusInsights::markDirty();\n  DisplayViews::requestHomeRefresh();\n  refreshStorageState();\n')
replace_once(FW / "interruption_service.cpp", '  processPersistence();\n  refreshStorageState();\n}', '  processPersistence();\n  refreshStorageState();\n  FocusInsights::update();\n}')

# API: embed raw numeric insights in the existing live summary; frontend localizes formatting.
replace_once(FW / "interruption_api.cpp", '#include "hardware_registry.h"\n', '#include "hardware_registry.h"\n#include "focus_insights.h"\n')
replace_once(
    FW / "interruption_api.cpp",
    "  removeTrailingComma(out);\n  out += '}';\n  out += '}';\n}\n\nvoid appendValues",
    "  removeTrailingComma(out);\n  out += \"},\";\n\n"
    "  const auto &insights = FocusInsights::snapshot();\n"
    "  JsonUtils::appendKey(out, \"focus\");\n"
    "  out += '{';\n"
    "  fieldBool(out, \"timeValid\", insights.timeValid);\n"
    "  fieldBool(out, \"currentPhaseAvailable\", insights.currentPhaseAvailable);\n"
    "  fieldUInt(out, \"currentPhaseSeconds\", insights.currentPhaseSeconds);\n"
    "  fieldBool(out, \"longestTodayAvailable\", insights.longestTodayAvailable);\n"
    "  fieldUInt(out, \"longestTodaySeconds\", insights.longestTodaySeconds);\n"
    "  fieldBool(out, \"longestWeekAvailable\", insights.longestWeekAvailable);\n"
    "  fieldUInt(out, \"longestWeekSeconds\", insights.longestWeekSeconds);\n"
    "  fieldUInt(out, \"trendPrevious60\", insights.trendPrevious60);\n"
    "  fieldUInt(out, \"trendLast60\", insights.trendLast60);\n"
    "  fieldString(out, \"trendDirection\", FocusInsights::trendDirectionName(insights.trendDirection));\n"
    "  fieldUInt(out, \"generatedEpochSeconds\", insights.generatedEpochSeconds, false);\n"
    "  out += \"},\";\n\n"
    "  JsonUtils::appendKey(out, \"patterns\");\n"
    "  out += '{';\n"
    "  fieldUInt(out, \"evaluatedDays\", insights.evaluatedDays);\n"
    "  fieldBool(out, \"coverageComplete\", insights.patternsCoverageComplete);\n"
    "  fieldBool(out, \"quietSufficient\", insights.quietSufficient);\n"
    "  fieldUInt(out, \"quietStartHour\", insights.quietStartHour);\n"
    "  fieldUInt(out, \"quietEndHour\", insights.quietEndHour);\n"
    "  fieldUInt(out, \"quietCoveredDays\", insights.quietCoveredDays);\n"
    "  fieldBool(out, \"peakSufficient\", insights.peakSufficient);\n"
    "  fieldUInt(out, \"peakStartHour\", insights.peakStartHour);\n"
    "  fieldUInt(out, \"peakEndHour\", insights.peakEndHour);\n"
    "  fieldUInt(out, \"peakCoveredDays\", insights.peakCoveredDays, false);\n"
    "  out += '}';\n"
    "  out += '}';\n}\n\nvoid appendValues"
)

# OLED integration.
replace_once(FW / "display_views.cpp", '#include "display_sh1106.h"\n', '#include "display_sh1106.h"\n#include "focus_insights.h"\n')
replace_once(
    FW / "display_views.cpp",
    'bool manualTestWasActive = false;\n',
    'bool manualTestWasActive = false;\nuint8_t insightsPage = 0;\nuint32_t insightsPageNextMs = 0;\n'
)
replace_once(
    FW / "display_views.cpp",
    '  const char *tooFast;\n};',
    '  const char *tooFast;\n  const char *current;\n  const char *week;\n  const char *trend120;\n  const char *quietest;\n  const char *most;\n  const char *noData;\n};'
)
replace_once(
    FW / "display_views.cpp",
    '  static const Labels de{"HEUTE", "LETZTE", "JETZT", "FOKUS", "SCHNITT", "ZU SCHNELL!"};\n'
    '  static const Labels en{"TODAY", "LAST", "NOW", "FOCUS", "AVG", "TOO FAST!"};\n'
    '  static const Labels fr{"JOUR", "DERNIER", "MAINT", "FOCUS", "MOY", "TROP VITE!"};\n'
    '  static const Labels it{"OGGI", "ULTIMA", "ORA", "FOCUS", "MEDIA", "TROPPO PRESTO!"};\n'
    '  static const Labels swg{"HEIT", "LETSCHTE", "JETZT", "FOKUS", "SCHNITT", "NET SO HEKTISCH!"};',
    '  static const Labels de{"HEUTE", "LETZTE", "JETZT", "FOKUS", "SCHNITT", "ZU SCHNELL!", "AKTUELL", "WOCHE", "120 MIN", "RUHIGSTE", "MEISTE", "KEINE DATEN"};\n'
    '  static const Labels en{"TODAY", "LAST", "NOW", "FOCUS", "AVG", "TOO FAST!", "CURRENT", "WEEK", "120 MIN", "QUIETEST", "MOST", "NO DATA"};\n'
    '  static const Labels fr{"JOUR", "DERNIER", "MAINT", "FOCUS", "MOY", "TROP VITE!", "ACTUEL", "SEMAINE", "120 MIN", "PLUS CALME", "PLUS", "PAS DONNEES"};\n'
    '  static const Labels it{"OGGI", "ULTIMA", "ORA", "FOCUS", "MEDIA", "TROPPO PRESTO!", "ATTUALE", "SETTIMANA", "120 MIN", "PIU CALMA", "PIU", "NO DATI"};\n'
    '  static const Labels swg{"HEIT", "LETSCHTE", "JETZT", "FOKUS", "SCHNITT", "NET SO HEKTISCH!", "AKTUELL", "WOCHE", "120 MIN", "RUHIGSTE", "MEISTE", "KOIN DATEN"};'
)

insert_after_focus = r'''

void durationText(uint32_t seconds, bool available, char out[16]) {
  if (!available) { snprintf(out, 16, "--"); return; }
  if (seconds < 60U) snprintf(out, 16, "%lus", static_cast<unsigned long>(seconds));
  else if (seconds < 3600U) snprintf(out, 16, "%lum", static_cast<unsigned long>(seconds / 60U));
  else {
    const uint32_t hours = seconds / 3600U;
    const uint32_t minutes = (seconds % 3600U) / 60U;
    if (minutes) snprintf(out, 16, "%luh%02lum", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes));
    else snprintf(out, 16, "%luh", static_cast<unsigned long>(hours));
  }
}

bool renderQuietPhases(uint8_t page) {
  const auto &insights = FocusInsights::snapshot();
  DisplaySh1106::frameClear();
  char value[16] = "--";
  const char *heading = labels().current;
  if (page == 0U) {
    heading = labels().current;
    durationText(insights.currentPhaseSeconds, insights.currentPhaseAvailable, value);
  } else if (page == 1U) {
    heading = labels().today;
    durationText(insights.longestTodaySeconds, insights.longestTodayAvailable, value);
  } else if (page == 2U) {
    heading = labels().week;
    durationText(insights.longestWeekSeconds, insights.longestWeekAvailable, value);
  } else {
    heading = labels().trend120;
    const char *symbol = insights.trendDirection == FocusInsightsLogic::TrendDirection::Falling ? "v" :
                         insights.trendDirection == FocusInsightsLogic::TrendDirection::Rising ? "^" : "=";
    snprintf(value, sizeof(value), "%s", symbol);
  }
  DisplaySh1106::drawCenteredText(2, heading);
  DisplaySh1106::drawHLine(0, 127, 12);
  drawCenteredScaledAt(value, 19, page == 3U ? 5 : 4, 124);
  if (page == 3U) {
    char footer[24];
    snprintf(footer, sizeof(footer), "%u > %u", static_cast<unsigned int>(insights.trendPrevious60), static_cast<unsigned int>(insights.trendLast60));
    DisplaySh1106::drawCenteredText(53, footer);
  }
  return DisplaySh1106::present();
}

bool renderWorkPatterns(uint8_t page) {
  const auto &insights = FocusInsights::snapshot();
  const bool quiet = page == 0U;
  const bool available = quiet ? insights.quietSufficient : insights.peakSufficient;
  DisplaySh1106::frameClear();
  DisplaySh1106::drawCenteredText(2, quiet ? labels().quietest : labels().most);
  DisplaySh1106::drawHLine(0, 127, 12);
  if (!available) {
    drawCenteredScaledAt(labels().noData, 25, 1, 124);
    return DisplaySh1106::present();
  }
  char range[16];
  const uint8_t start = quiet ? insights.quietStartHour : insights.peakStartHour;
  const uint8_t end = quiet ? insights.quietEndHour : insights.peakEndHour;
  snprintf(range, sizeof(range), "%02u-%02u", static_cast<unsigned int>(start), static_cast<unsigned int>(end));
  drawCenteredScaledAt(range, 22, 3, 124);
  return DisplaySh1106::present();
}
'''
replace_once(
    FW / "display_views.cpp",
    'bool renderHome(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {',
    insert_after_focus + '\nbool renderHome(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {'
)
replace_once(
    FW / "display_views.cpp",
    '    case ProjectPreferences::DisplayMode::Focus: return renderFocus(summary, age);\n    case ProjectPreferences::DisplayMode::Standard:',
    '    case ProjectPreferences::DisplayMode::Focus: return renderFocus(summary, age);\n'
    '    case ProjectPreferences::DisplayMode::QuietPhases: return renderQuietPhases(static_cast<uint8_t>(insightsPage % 4U));\n'
    '    case ProjectPreferences::DisplayMode::WorkPatterns: return renderWorkPatterns(static_cast<uint8_t>(insightsPage % 2U));\n'
    '    case ProjectPreferences::DisplayMode::Standard:'
)
replace_once(
    FW / "display_views.cpp",
    '  renderRequested = true;\n  update(summary);\n}',
    '  renderRequested = true;\n  insightsPage = 0U;\n  insightsPageNextMs = millis() + ProjectConfig::DISPLAY_INSIGHTS_PAGE_MS;\n  update(summary);\n}'
)
replace_once(
    FW / "display_views.cpp",
    '  if (!renderRequested && !flashRequested && !flashActive &&\n      static_cast<uint32_t>(nowMs - lastIdleEvaluationMs) < 1000U) return;',
    '  const auto activeMode = ProjectPreferences::displayMode();\n'
    '  const bool rotatingInsights = activeMode == ProjectPreferences::DisplayMode::QuietPhases ||\n'
    '                                activeMode == ProjectPreferences::DisplayMode::WorkPatterns;\n'
    '  if (activeMode != lastMode) {\n'
    '    insightsPage = 0U;\n'
    '    insightsPageNextMs = nowMs + ProjectConfig::DISPLAY_INSIGHTS_PAGE_MS;\n'
    '    renderRequested = true;\n'
    '  } else if (rotatingInsights && due(nowMs, insightsPageNextMs)) {\n'
    '    const uint8_t pageCount = activeMode == ProjectPreferences::DisplayMode::QuietPhases ? 4U : 2U;\n'
    '    insightsPage = static_cast<uint8_t>((insightsPage + 1U) % pageCount);\n'
    '    insightsPageNextMs = nowMs + ProjectConfig::DISPLAY_INSIGHTS_PAGE_MS;\n'
    '    renderRequested = true;\n'
    '  }\n\n'
    '  if (!renderRequested && !flashRequested && !flashActive &&\n'
    '      static_cast<uint32_t>(nowMs - lastIdleEvaluationMs) < 1000U) return;'
)
replace_once(FW / "display_views.cpp", '  const auto mode = ProjectPreferences::displayMode();\n', '  const auto mode = activeMode;\n')

# Web UI translations are layered so the old base language-parity mechanism stays intact.
app = FW / "ui-src" / "app.js"
I18N_350 = r'''
  const I18N_350 = {
    de: {
      'focus.card.title': 'Fokus & Ruhe', 'focus.card.desc': 'Ungestörte Phasen und der dezente Trend der letzten 120 Minuten.',
      'focus.current': 'Aktuelle Ruhephase', 'focus.today': 'Längste heute', 'focus.week': 'Längste Woche', 'focus.trend': 'Trend 120 min',
      'focus.noBasis': 'Noch keine Basis', 'focus.trend.falling': 'fallend', 'focus.trend.stable': 'stabil', 'focus.trend.rising': 'steigend',
      'focus.explain': 'Ruhephase = Zeit seit bzw. zwischen gültigen Unterbrechungen am selben Tag. Der 120-min-Trend vergleicht die letzten 60 Minuten mit den 60 Minuten davor.',
      'focus.trendExplain': '↓ = mindestens 2 weniger · → = ungefähr gleich · ↑ = mindestens 2 mehr',
      'patterns.title': 'Arbeitsmuster', 'patterns.desc': 'Ruhigste beobachtete Zeit und Stunde mit den meisten Unterbrechungen.',
      'patterns.quiet': 'Ruhigstes Zeitfenster', 'patterns.peak': 'Meiste Unterbrechungen', 'patterns.basis': 'Basis',
      'patterns.days': '{n} ausgewertete Tage', 'patterns.insufficient': 'Noch nicht genug Daten',
      'patterns.explain': 'Basis sind bis zu 30 abgeschlossene Tage. Berücksichtigt werden nur Zeitfenster innerhalb der beobachteten Tagesaktivität. Ruhigstes Zeitfenster = niedrigster Durchschnitt in 2 Stunden, Meiste Unterbrechungen = höchster Durchschnitt in 1 Stunde.',
      'patterns.coveragePartial': 'Der Roh-Ringspeicher deckt den gesamten 30-Tage-Zeitraum nicht mehr vollständig ab.',
      'project.settings.title': 'Projekteinstellungen', 'project.settings.desc': 'Gerätebezogene Einstellungen für Display, Rückmeldung und DY-SV17F. Änderungen gelten sofort und bleiben im ESP32 gespeichert.',
      'view.settings.desc': 'Projekt- und Browserdarstellungseinstellungen an einer Stelle.',
      'project.displayMode.quietPhases': 'Ruhephasen – Aktuell / Heute / Woche / 120 min', 'project.displayMode.workPatterns': 'Arbeitsmuster – ruhigste / meiste'
    },
    en: {
      'focus.card.title': 'Focus & quiet time', 'focus.card.desc': 'Uninterrupted phases and a restrained trend for the last 120 minutes.',
      'focus.current': 'Current quiet phase', 'focus.today': 'Longest today', 'focus.week': 'Longest week', 'focus.trend': '120 min trend',
      'focus.noBasis': 'No basis yet', 'focus.trend.falling': 'falling', 'focus.trend.stable': 'stable', 'focus.trend.rising': 'rising',
      'focus.explain': 'Quiet phase = time since or between valid interruptions on the same day. The 120-minute trend compares the last 60 minutes with the 60 minutes before them.',
      'focus.trendExplain': '↓ = at least 2 fewer · → = roughly equal · ↑ = at least 2 more',
      'patterns.title': 'Work patterns', 'patterns.desc': 'Quietest observed window and the hour with the most interruptions.',
      'patterns.quiet': 'Quietest time window', 'patterns.peak': 'Most interruptions', 'patterns.basis': 'Basis',
      'patterns.days': '{n} evaluated days', 'patterns.insufficient': 'Not enough data yet',
      'patterns.explain': 'Uses up to 30 completed days. Only windows inside observed daily activity are considered. Quietest = lowest 2-hour average; most interruptions = highest 1-hour average.',
      'patterns.coveragePartial': 'The raw ring no longer fully covers the complete 30-day period.',
      'project.settings.title': 'Project settings', 'project.settings.desc': 'Device settings for display, feedback and DY-SV17F. Changes apply immediately and remain stored on the ESP32.',
      'view.settings.desc': 'Project and browser appearance settings in one place.',
      'project.displayMode.quietPhases': 'Quiet phases – current / today / week / 120 min', 'project.displayMode.workPatterns': 'Work patterns – quietest / most'
    },
    it: {
      'focus.card.title': 'Focus e quiete', 'focus.card.desc': 'Fasi senza interruzioni e tendenza discreta degli ultimi 120 minuti.',
      'focus.current': 'Fase attuale', 'focus.today': 'Più lunga oggi', 'focus.week': 'Più lunga settimana', 'focus.trend': 'Trend 120 min',
      'focus.noBasis': 'Ancora nessuna base', 'focus.trend.falling': 'in calo', 'focus.trend.stable': 'stabile', 'focus.trend.rising': 'in aumento',
      'focus.explain': 'Fase di quiete = tempo dall’ultima o tra due interruzioni valide nello stesso giorno. Il trend 120 min confronta gli ultimi 60 minuti con i 60 precedenti.',
      'focus.trendExplain': '↓ = almeno 2 in meno · → = circa uguale · ↑ = almeno 2 in più',
      'patterns.title': 'Schemi di lavoro', 'patterns.desc': 'Finestra osservata più tranquilla e ora con più interruzioni.',
      'patterns.quiet': 'Finestra più tranquilla', 'patterns.peak': 'Più interruzioni', 'patterns.basis': 'Base',
      'patterns.days': '{n} giorni analizzati', 'patterns.insufficient': 'Dati ancora insufficienti',
      'patterns.explain': 'Usa fino a 30 giorni completati. Considera solo finestre dentro l’attività giornaliera osservata. Più tranquilla = media minima su 2 ore; più interruzioni = media massima su 1 ora.',
      'patterns.coveragePartial': 'L’anello dei dati grezzi non copre più completamente i 30 giorni.',
      'project.settings.title': 'Impostazioni progetto', 'project.settings.desc': 'Impostazioni del dispositivo per display, feedback e DY-SV17F. Le modifiche sono immediate e salvate nell’ESP32.',
      'view.settings.desc': 'Impostazioni del progetto e dell’aspetto del browser in un unico punto.',
      'project.displayMode.quietPhases': 'Fasi di quiete – attuale / oggi / settimana / 120 min', 'project.displayMode.workPatterns': 'Schemi di lavoro – quiete / picco'
    },
    fr: {
      'focus.card.title': 'Focus & calme', 'focus.card.desc': 'Périodes sans interruption et tendance discrète sur les 120 dernières minutes.',
      'focus.current': 'Période actuelle', 'focus.today': 'Plus longue aujourd’hui', 'focus.week': 'Plus longue semaine', 'focus.trend': 'Tendance 120 min',
      'focus.noBasis': 'Pas encore de base', 'focus.trend.falling': 'en baisse', 'focus.trend.stable': 'stable', 'focus.trend.rising': 'en hausse',
      'focus.explain': 'Période calme = temps depuis ou entre des interruptions valides le même jour. La tendance 120 min compare les 60 dernières minutes aux 60 précédentes.',
      'focus.trendExplain': '↓ = au moins 2 de moins · → = environ égal · ↑ = au moins 2 de plus',
      'patterns.title': 'Rythmes de travail', 'patterns.desc': 'Créneau observé le plus calme et heure avec le plus d’interruptions.',
      'patterns.quiet': 'Créneau le plus calme', 'patterns.peak': 'Plus d’interruptions', 'patterns.basis': 'Base',
      'patterns.days': '{n} jours évalués', 'patterns.insufficient': 'Pas encore assez de données',
      'patterns.explain': 'Jusqu’à 30 jours terminés sont utilisés. Seuls les créneaux dans l’activité journalière observée comptent. Plus calme = moyenne minimale sur 2 h ; plus d’interruptions = moyenne maximale sur 1 h.',
      'patterns.coveragePartial': 'L’anneau brut ne couvre plus entièrement la période de 30 jours.',
      'project.settings.title': 'Paramètres du projet', 'project.settings.desc': 'Paramètres de l’appareil pour affichage, retour et DY-SV17F. Les modifications sont immédiates et enregistrées sur l’ESP32.',
      'view.settings.desc': 'Paramètres du projet et de l’affichage du navigateur au même endroit.',
      'project.displayMode.quietPhases': 'Périodes calmes – actuel / jour / semaine / 120 min', 'project.displayMode.workPatterns': 'Rythmes de travail – calme / maximum'
    },
    swg: {
      'focus.card.title': 'Fokus & Ruh', 'focus.card.desc': 'Ungstörte Phase ond dr dezente Trend vo de letschta 120 Minuta.',
      'focus.current': 'Aktuelle Ruhephase', 'focus.today': 'Längste heit', 'focus.week': 'Längste Woch', 'focus.trend': 'Trend 120 min',
      'focus.noBasis': 'No koi Basis', 'focus.trend.falling': 'fallend', 'focus.trend.stable': 'stabil', 'focus.trend.rising': 'steigend',
      'focus.explain': 'Ruhephase = Zeit seit oder zwischa gültige Unterbrechunga am selba Dag. Dr 120-min-Trend vergleicht dia letschta 60 Minuta mit de 60 davor.',
      'focus.trendExplain': '↓ = mindestens 2 weniger · → = ungefähr gleich · ↑ = mindestens 2 mehr',
      'patterns.title': 'Arbeitsmuster', 'patterns.desc': 'Ruhigschts beobachtets Zeitfenster ond d Stond mit de meischta Unterbrechunga.',
      'patterns.quiet': 'Ruhigschts Zeitfenster', 'patterns.peak': 'Meischte Unterbrechunga', 'patterns.basis': 'Basis',
      'patterns.days': '{n} ausgwertete Däg', 'patterns.insufficient': 'No net gnug Daten',
      'patterns.explain': 'Basis send bis zu 30 fertige Däg. Bloß Zeitfenster innerhalb dr beobachteta Tagesaktivität zählet. Ruhigschts = kleinschter 2-Stonda-Schnitt; meischte = größter 1-Stond-Schnitt.',
      'patterns.coveragePartial': 'Dr Roh-Ring deckt dia ganze 30 Däg nemme vollständig ab.',
      'project.settings.title': 'Projekteinstellungen', 'project.settings.desc': 'Geräteeinstellungen für Display, Rückmeldung ond DY-SV17F. Ändrunga geltet glei ond bleibet em ESP32 gspeichert.',
      'view.settings.desc': 'Projekt- ond Browserdarstellungseinstellungen an oim Platz.',
      'project.displayMode.quietPhases': 'Ruhephase – aktuell / heit / Woch / 120 min', 'project.displayMode.workPatterns': 'Arbeitsmuster – ruhig / meischte'
    },
    'swg-alb': {
      'focus.card.title': 'Fokus & Ruh', 'focus.card.desc': 'Ungstörte Phase ond dr Trend vo de letschta 120 Minuta.',
      'focus.current': 'Aktuelle Ruhephase', 'focus.today': 'Längste heit', 'focus.week': 'Längste Woch', 'focus.trend': 'Trend 120 min',
      'focus.noBasis': 'No koi Basis', 'focus.trend.falling': 'fallend', 'focus.trend.stable': 'stabil', 'focus.trend.rising': 'steigend',
      'focus.explain': 'Ruhephase = Zeit seit oder zwischa gültige Unterbrechunga am selba Dag. Dr Trend vergleicht 60 Minuta mit de 60 davor.',
      'focus.trendExplain': '↓ = mindestens 2 weniger · → = ungefähr gleich · ↑ = mindestens 2 mehr',
      'patterns.title': 'Arbeitsmuster', 'patterns.desc': 'Ruhigschts Zeitfenster ond d Stond mit de meischta Unterbrechunga.',
      'patterns.quiet': 'Ruhigschts Zeitfenster', 'patterns.peak': 'Meischte Unterbrechunga', 'patterns.basis': 'Basis',
      'patterns.days': '{n} ausgwertete Däg', 'patterns.insufficient': 'No net gnug Daten',
      'patterns.explain': 'Bis zu 30 fertige Däg. Bloß beobachtete Tageszeit zählt. Ruhigschts = 2-Stonda-Schnitt, meischte = 1-Stond-Schnitt.',
      'patterns.coveragePartial': 'Dr Roh-Ring deckt dia 30 Däg nemme ganz ab.',
      'project.settings.title': 'Projekteinstellungen', 'project.settings.desc': 'Geräteeinstellungen für Display, Rückmeldung ond DY-SV17F. Ändrunga bleibet gspeichert.',
      'view.settings.desc': 'Projekt- ond Browsereinstellungen an oim Platz.',
      'project.displayMode.quietPhases': 'Ruhephase – aktuell / heit / Woch / 120 min', 'project.displayMode.workPatterns': 'Arbeitsmuster – ruhig / meischte'
    },
    'swg-ob': {
      'focus.card.title': 'Fokus & Ruh', 'focus.card.desc': 'Ungstörte Phase ond dr Trend vo de letschta 120 Minuta.',
      'focus.current': 'Aktuelle Ruhephase', 'focus.today': 'Längste heit', 'focus.week': 'Längste Woch', 'focus.trend': 'Trend 120 min',
      'focus.noBasis': 'No koi Basis', 'focus.trend.falling': 'fallend', 'focus.trend.stable': 'stabil', 'focus.trend.rising': 'steigend',
      'focus.explain': 'Ruhephase = Zeit seit oder zwischa gültige Unterbrechunga am selba Dag. Dr Trend vergleicht 60 Minuta mit de 60 davor.',
      'focus.trendExplain': '↓ = mindestens 2 weniger · → = ungefähr gleich · ↑ = mindestens 2 mehr',
      'patterns.title': 'Arbeitsmuster', 'patterns.desc': 'Ruhigschts Zeitfenster ond d Stond mit de meischta Unterbrechunga.',
      'patterns.quiet': 'Ruhigschts Zeitfenster', 'patterns.peak': 'Meischte Unterbrechunga', 'patterns.basis': 'Basis',
      'patterns.days': '{n} ausgwertete Däg', 'patterns.insufficient': 'No it gnug Daten',
      'patterns.explain': 'Bis zu 30 fertige Däg. Bloß beobachtete Tageszeit zählt. Ruhigschts = 2-Stonda-Schnitt, meischte = 1-Stond-Schnitt.',
      'patterns.coveragePartial': 'Dr Roh-Ring deckt dia 30 Däg nimme ganz ab.',
      'project.settings.title': 'Projekteinstellungen', 'project.settings.desc': 'Geräteeinstellungen für Display, Rückmeldung ond DY-SV17F. Ändrunga bleibet gspeichert.',
      'view.settings.desc': 'Projekt- ond Browsereinstellungen an oim Platz.',
      'project.displayMode.quietPhases': 'Ruhephase – aktuell / heit / Woch / 120 min', 'project.displayMode.workPatterns': 'Arbeitsmuster – ruhig / meischte'
    }
  };
  Object.entries(I18N_350).forEach(([code, labels]) => Object.assign(I18N[code], labels));
'''
replace_once(
    app,
    '  Object.entries(I18N_340).forEach(([code, labels]) => Object.assign(I18N[code], labels));\n\n  const STORAGE_STATUS_LABELS',
    '  Object.entries(I18N_340).forEach(([code, labels]) => Object.assign(I18N[code], labels));\n' + I18N_350 + '\n  const STORAGE_STATUS_LABELS'
)

# Home is operational only; project settings move unchanged to Settings.
replace_once(
    app,
    "          { id: 'interruptions-home', titleKey: 'interruptions.title', descriptionKey: 'interruptions.desc', icon: 'interrupt', width: 'full', components: [\n"
    "            { type: 'interruptionHome' }\n"
    "          ] },\n"
    "          { id: 'project-settings', titleKey: 'project.settings.title', descriptionKey: 'project.settings.desc', icon: 'settings', width: 'full', components: [\n"
    "            { type: 'projectSettings' }\n"
    "          ] }",
    "          { id: 'interruptions-home', titleKey: 'interruptions.title', descriptionKey: 'interruptions.desc', icon: 'interrupt', width: 'full', components: [\n"
    "            { type: 'interruptionHome' }\n"
    "          ] },\n"
    "          { id: 'focus-insights', titleKey: 'focus.card.title', descriptionKey: 'focus.card.desc', icon: 'clock', width: 'full', components: [\n"
    "            { type: 'focusInsights' }\n"
    "          ] }"
)
replace_once(
    app,
    "          { id: 'heatmap-hourly', titleKey: 'analytics.hourly.title', descriptionKey: 'analytics.hourly.desc', icon: 'analytics', width: 'full', components: [{ type: 'heatmapHourly' }] },\n",
    "          { id: 'work-patterns', titleKey: 'patterns.title', descriptionKey: 'patterns.desc', icon: 'clock', width: 'full', components: [{ type: 'workPatterns' }] },\n"
    "          { id: 'heatmap-hourly', titleKey: 'analytics.hourly.title', descriptionKey: 'analytics.hourly.desc', icon: 'analytics', width: 'full', components: [{ type: 'heatmapHourly' }] },\n"
)
replace_once(
    app,
    "        cards: [\n          { id: 'language', titleKey: 'card.language'",
    "        cards: [\n"
    "          { id: 'project-settings', titleKey: 'project.settings.title', descriptionKey: 'project.settings.desc', icon: 'settings', width: 'full', components: [\n"
    "            { type: 'projectSettings' }\n"
    "          ] },\n"
    "          { id: 'language', titleKey: 'card.language'"
)
replace_once(
    app,
    "    interruptions: { todayCount: 0, unassignedCount: 0, sequence: 0, persistedSequence: 0, pendingCount: 0, droppedCount: 0, storageState: 'unavailable', soundEnabled: true, last: { available: false } },",
    "    interruptions: { todayCount: 0, unassignedCount: 0, sequence: 0, persistedSequence: 0, pendingCount: 0, droppedCount: 0, storageState: 'unavailable', soundEnabled: true, last: { available: false }, focus: { timeValid: false, currentPhaseAvailable: false, longestTodayAvailable: false, longestWeekAvailable: false, trendPrevious60: 0, trendLast60: 0, trendDirection: 'stable' }, patterns: { evaluatedDays: 0, coverageComplete: true, quietSufficient: false, peakSufficient: false } },"
)

render_insights = r'''
  function insightDuration(seconds, available) {
    return available ? formatIntervalSeconds(Number(seconds) || 0, true) : t('focus.noBasis');
  }

  function renderFocusInsights() {
    const root = el('div', 'focus-insights');
    const grid = el('div', 'focus-insight-grid');
    const entries = {};
    const add = (id, labelKey) => {
      const item = el('div', 'focus-insight-item');
      const label = el('span', 'focus-insight-label'); label.textContent = t(labelKey);
      const value = el('strong', 'focus-insight-value');
      item.append(label, value); grid.append(item); entries[id] = value;
    };
    add('current', 'focus.current'); add('today', 'focus.today'); add('week', 'focus.week'); add('trend', 'focus.trend');
    const explanation = el('p', 'insight-explanation');
    const trendExplanation = el('p', 'insight-explanation compact');
    explanation.textContent = t('focus.explain'); trendExplanation.textContent = t('focus.trendExplain');
    root.append(grid, explanation, trendExplanation);

    const update = () => {
      const focus = state.interruptions.focus || {};
      let current = null;
      if (focus.currentPhaseAvailable) {
        const live = interruptionAgeSeconds();
        current = live == null ? Number(focus.currentPhaseSeconds || 0) : live;
      }
      entries.current.textContent = insightDuration(current, current != null);
      const longestToday = current == null ? Number(focus.longestTodaySeconds || 0) : Math.max(Number(focus.longestTodaySeconds || 0), current);
      const longestWeek = current == null ? Number(focus.longestWeekSeconds || 0) : Math.max(Number(focus.longestWeekSeconds || 0), current);
      entries.today.textContent = insightDuration(longestToday, !!focus.longestTodayAvailable || current != null);
      entries.week.textContent = insightDuration(longestWeek, !!focus.longestWeekAvailable || current != null);
      const direction = ['falling','stable','rising'].includes(focus.trendDirection) ? focus.trendDirection : 'stable';
      const symbol = direction === 'falling' ? '↓' : direction === 'rising' ? '↑' : '→';
      entries.trend.textContent = `${symbol} ${Number(focus.trendPrevious60 || 0)} → ${Number(focus.trendLast60 || 0)} · ${t(`focus.trend.${direction}`)}`;
    };
    Bindings.add(['interruptions.focus.currentPhaseAvailable','interruptions.focus.currentPhaseSeconds','interruptions.focus.longestTodayAvailable','interruptions.focus.longestTodaySeconds','interruptions.focus.longestWeekAvailable','interruptions.focus.longestWeekSeconds','interruptions.focus.trendPrevious60','interruptions.focus.trendLast60','interruptions.focus.trendDirection','interruptions.last','clock.tick'], update);
    return root;
  }

  function patternRange(start, end) {
    return `${String(Number(start) || 0).padStart(2, '0')}:00 – ${String(Number(end) || 0).padStart(2, '0')}:00`;
  }

  function renderWorkPatterns() {
    const root = el('div', 'work-patterns');
    const grid = el('div', 'focus-insight-grid patterns-grid');
    const quiet = el('div', 'focus-insight-item');
    const quietLabel = el('span', 'focus-insight-label'); quietLabel.textContent = t('patterns.quiet');
    const quietValue = el('strong', 'focus-insight-value'); quiet.append(quietLabel, quietValue);
    const peak = el('div', 'focus-insight-item');
    const peakLabel = el('span', 'focus-insight-label'); peakLabel.textContent = t('patterns.peak');
    const peakValue = el('strong', 'focus-insight-value'); peak.append(peakLabel, peakValue);
    const basis = el('div', 'focus-insight-item');
    const basisLabel = el('span', 'focus-insight-label'); basisLabel.textContent = t('patterns.basis');
    const basisValue = el('strong', 'focus-insight-value'); basis.append(basisLabel, basisValue);
    grid.append(quiet, peak, basis);
    const explanation = el('p', 'insight-explanation'); explanation.textContent = t('patterns.explain');
    const coverage = el('p', 'insight-explanation compact'); coverage.hidden = true;
    root.append(grid, explanation, coverage);

    const update = () => {
      const patterns = state.interruptions.patterns || {};
      quietValue.textContent = patterns.quietSufficient ? patternRange(patterns.quietStartHour, patterns.quietEndHour) : t('patterns.insufficient');
      peakValue.textContent = patterns.peakSufficient ? patternRange(patterns.peakStartHour, patterns.peakEndHour) : t('patterns.insufficient');
      basisValue.textContent = t('patterns.days').replace('{n}', String(Number(patterns.evaluatedDays || 0)));
      coverage.hidden = patterns.coverageComplete !== false;
      coverage.textContent = coverage.hidden ? '' : t('patterns.coveragePartial');
    };
    Bindings.add(['interruptions.patterns.evaluatedDays','interruptions.patterns.coverageComplete','interruptions.patterns.quietSufficient','interruptions.patterns.quietStartHour','interruptions.patterns.quietEndHour','interruptions.patterns.peakSufficient','interruptions.patterns.peakStartHour','interruptions.patterns.peakEndHour'], update);
    return root;
  }

'''
replace_once(app, '  function renderProjectSettings() {\n', render_insights + '  function renderProjectSettings() {\n')
replace_once(
    app,
    "      ['day-progress','project.displayMode.dayProgress'], ['focus','project.displayMode.focus']\n",
    "      ['day-progress','project.displayMode.dayProgress'], ['focus','project.displayMode.focus'],\n"
    "      ['quiet-phases','project.displayMode.quietPhases'], ['work-patterns','project.displayMode.workPatterns']\n"
)
replace_once(
    app,
    'interruptionHome: renderInterruptionHome, projectSettings: renderProjectSettings, heatmapHourly:',
    'interruptionHome: renderInterruptionHome, focusInsights: renderFocusInsights, workPatterns: renderWorkPatterns, projectSettings: renderProjectSettings, heatmapHourly:'
)

# Subdued layout, deliberately reusing the existing theme variables.
css = FW / "ui-src" / "app.css"
write(css, text(css) + r'''

/* Focus & Insights: compact numerical summaries without a second chart layer. */
.focus-insight-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 10px;
}
.focus-insight-item {
  min-width: 0;
  padding: 12px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: var(--surface-soft);
}
.focus-insight-label {
  display: block;
  font-size: .78rem;
  color: var(--muted);
  margin-bottom: 6px;
}
.focus-insight-value {
  display: block;
  font-size: clamp(1.05rem, 2.4vw, 1.55rem);
  line-height: 1.15;
  font-variant-numeric: tabular-nums;
}
.insight-explanation {
  margin: 12px 0 0;
  color: var(--muted);
  font-size: .82rem;
  line-height: 1.45;
}
.insight-explanation.compact { margin-top: 5px; font-size: .76rem; }
.patterns-grid { grid-template-columns: repeat(3, minmax(0, 1fr)); }
@media (max-width: 760px) {
  .focus-insight-grid, .patterns-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
@media (max-width: 480px) {
  .focus-insight-grid, .patterns-grid { grid-template-columns: 1fr; }
}
''')

# Release checks: new version, seven display modes, central cache and moved settings.
rc = FW / "tools" / "release_check.py"
value = text(rc)
value = value.replace('Portable release checks for Unterbrechungszaehler 3.4.1.', 'Portable release checks for Unterbrechungszaehler 3.5.0.')
value = value.replace('SOFTWARE_VERSION[] = "3.4.1"', 'SOFTWARE_VERSION[] = "3.5.0"')
value = value.replace('"project version 3.4.1"', '"project version 3.5.0"')
value = value.replace('check("day-progress" in JS and "project.displayMode.focus" in JS, "five OLED display modes exposed")', 'check("day-progress" in JS and "quiet-phases" in JS and "work-patterns" in JS, "seven OLED display modes exposed")')
value = value.replace('check("projectSettings: renderProjectSettings" in JS, "Home project settings card")', 'check("focusInsights: renderFocusInsights" in JS and "workPatterns: renderWorkPatterns" in JS, "Focus & Insights web renderers")\n    home_block = JS.split("home: {", 1)[1].split("analytics: {", 1)[0]\n    settings_block = JS.split("settings: {", 1)[1].split("}\\n    }\\n  };", 1)[0]\n    check("projectSettings" not in home_block and "projectSettings" in settings_block, "project settings moved from Home to Settings without duplication")\n    check("FOCUS_INSIGHTS_CACHE_MAX_AGE_MS = 60000" in project and "FOCUS_PATTERN_DAYS = 30" in project and "FOCUS_PATTERN_MIN_COVERED_DAYS = 5" in project, "bounded Focus & Insights cache policy")\n    insights_cpp = (ROOT / "focus_insights.cpp").read_text(encoding="utf-8")\n    check("InterruptionStore::readSequence" in insights_cpp and "local.dayIndex < earliestNeeded" in insights_cpp, "bounded newest-to-oldest raw insights scan")\n    check("windowCovered" in insights_cpp and "MIN_COVERED_DAYS" in insights_cpp, "observed-activity coverage rule for work patterns")\n    check("trendPrevious60" in JS and "trendLast60" in JS and "focus.explain" in JS, "explained 120-minute trend on Home")')
needle = '    guard_binary.unlink(missing_ok=True)'
if needle not in value:
    raise RuntimeError('guard test anchor missing')
value = value.replace(needle, needle + '\n\n    insights_binary = ROOT / "tools" / ".test_focus_insights"\n    subprocess.run(["g++", "-std=c++17", "-I", str(ROOT), str(ROOT / "tools" / "test_focus_insights.cpp"), "-o", str(insights_binary)], check=True)\n    subprocess.run([str(insights_binary)], check=True)\n    insights_binary.unlink(missing_ok=True)\n    check(True, "Focus & Insights host rules")')
write(rc, value)

# README is current-state documentation; version history remains only in CHANGELOG.
readme = ROOT / "README.md"
value = text(readme).replace('> **Aktueller Stand:** `3.4.1`', '> **Aktueller Stand:** `3.5.0`')
anchor = '## Versionsverlauf\n\nDer vollständige technische Versionsverlauf steht im [Changelog](CHANGELOG.md).\n'
addition = '''## Fokus & Ruhe\n\nDie Weboberfläche wertet die bereits gespeicherten gültigen Unterbrechungen zusätzlich als Ruhephasen aus. Angezeigt werden die aktuelle ungestörte Phase, der längste Wert des Tages, der längste Wert der laufenden Woche und ein zurückhaltender 120-Minuten-Trend. Der Trend vergleicht die letzten 60 Minuten mit den 60 Minuten davor; eine Differenz von mindestens zwei Ereignissen ergibt steigend bzw. fallend, kleinere Abweichungen gelten als stabil.\n\nUnter **Auswertung → Arbeitsmuster** werden bis zu 30 abgeschlossene Tage betrachtet. Ein 2-Stunden-Fenster gilt nur dann als abgedeckt, wenn es vollständig zwischen erster und letzter gültiger Unterbrechung des jeweiligen Tages liegt. Ab mindestens fünf abgedeckten Tagen wird das ruhigste 2-Stunden-Fenster sowie die einzelne Stunde mit den meisten Unterbrechungen angezeigt. Dadurch werden unbeobachtete Nachtzeiten nicht automatisch als ruhige Arbeitszeit interpretiert.\n\nDie Berechnung verwendet den bestehenden 9-Byte-Roh-Ringspeicher und schreibt keine zusätzlichen Focus-/Insight-Daten dauerhaft. Die bisher auf Home angezeigten Geräteoptionen befinden sich unverändert unter **Einstellungen → Projekteinstellungen**.\n\n'''
if anchor not in value:
    raise RuntimeError('README version-history anchor missing')
value = value.replace(anchor, anchor + '\n' + addition, 1)
write(readme, value)

# Changelog and release notes.
changelog = ROOT / "CHANGELOG.md"
value = text(changelog)
entry = '''# Changelog\n\n## 3.5.0\n\n- neue Home-Karte **Fokus & Ruhe** mit aktueller Ruhephase, längster Ruhephase heute, längster Ruhephase der laufenden Woche und erklärtem 120-Minuten-Trend\n- Ruhephasen werden ausschließlich zwischen gültigen Unterbrechungen desselben lokalen Tages bzw. von der letzten heutigen Unterbrechung bis jetzt berechnet; keine künstlichen Nacht- oder Mitternachtsphasen\n- Trend vergleicht die letzten 60 Minuten mit den 60 Minuten davor; Differenzen ab ±2 werden als steigend/fallend, kleinere Abweichungen als stabil dargestellt\n- neue Auswertung **Arbeitsmuster**: ruhigstes vollständig beobachtetes 2-Stunden-Fenster und vollständig beobachtete Einzelstunde mit den meisten Unterbrechungen\n- Arbeitsmuster verwenden maximal 30 abgeschlossene Tage, mindestens fünf abgedeckte Tage pro Ergebnis und ausschließlich Zeitfenster innerhalb der beobachteten Tagesaktivität\n- zentrale RAM-Cache-Berechnung scannt den Raw-Ring nur bis zum benötigten Zeithorizont, wird nach neuen gültigen Ereignissen ungültig und spätestens nach 60 Sekunden aktualisiert\n- neue OLED-Modi **Ruhephasen** und **Arbeitsmuster** mit großen, nicht blockierend wechselnden Seiten; bestehende Displaymodi bleiben unverändert\n- bisherige Home-Projekteinstellungen unverändert nach **Einstellungen → Projekteinstellungen** verschoben; API, NVS-Keys, Wertebereiche und Persistenz bleiben gleich\n- neue Focus-/Insight-Texte in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch\n- RawEvent bleibt 9 Byte; kein neues persistentes Focus-/Insight-Format und keine zusätzlichen Flash-Schreibvorgänge\n\n'''
if not value.startswith('# Changelog\n'):
    raise RuntimeError('CHANGELOG heading missing')
write(changelog, entry + value[len('# Changelog\n\n'):])

notes = FW / "RELEASE_NOTES.md"
value = text(notes)
entry = '''# Release 3.5.0\n\n- Fokus-&-Ruhe-Karte auf Home mit aktueller/längster Tages-/Wochenphase und erklärtem 120-Minuten-Trend\n- Arbeitsmuster aus bis zu 30 abgeschlossenen Tagen: ruhigstes vollständig beobachtetes 2-h-Fenster und stärkste vollständig beobachtete 1-h-Stunde, jeweils ab fünf abgedeckten Tagen\n- zwei neue große OLED-Modi für Ruhephasen und Arbeitsmuster, nicht blockierend und sprachabhängig\n- Projekteinstellungen aus Home unverändert in eine eigene Kachel unter Einstellungen verschoben\n- zentrale, maximal 60 s alte RAM-Auswertung statt permanenter Raw-Ring-Scans; kein neues Speicherformat, RawEvent weiterhin 9 Byte\n- alle sieben vorhandenen UI-Sprachen nachgezogen\n\n'''
write(notes, entry + value)

# Small current-state notes in localized README files without introducing version histories.
localized = {
    ROOT / 'docs/de/README.md': '\n## Fokus & Ruhe\n\nDie aktuelle Ruhephase, Tages-/Wochenbestwerte und der 120-Minuten-Trend werden aus den bestehenden gültigen Rohereignissen berechnet. Arbeitsmuster verwenden bis zu 30 abgeschlossene Tage und nur vollständig beobachtete Zeitfenster. Die Projekteinstellungen befinden sich unter **Einstellungen → Projekteinstellungen**. Das 9-Byte-Raw-Format bleibt unverändert.\n',
    ROOT / 'docs/en/README.md': '\n## Focus & quiet time\n\nThe current quiet phase, daily/weekly longest phases and the 120-minute trend are calculated from existing valid raw events. Work patterns use up to 30 completed days and only fully observed time windows. Project settings are located under **Settings → Project settings**. The 9-byte raw format is unchanged.\n',
    ROOT / 'docs/swg/README.md': '\n## Fokus & Ruh\n\nAktuelle Ruhephase, Tages-/Wocha-Bestwert ond dr 120-Minuta-Trend kommet aus de vorhandena gültige Rohereignisse. Arbeitsmuster nehmet bis zu 30 fertige Däg ond bloß vollständig beobachtete Zeitfenster. D Projekteinstellungen send unter **Einstellungen → Projekteinstellungen**. S 9-Byte-Raw-Format bleibt wia s isch.\n',
}
for path, section in localized.items():
    value = text(path)
    if '## Fokus & Ruhe' not in value and '## Focus & quiet time' not in value and '## Fokus & Ruh' not in value:
        value += section
    write(path, value)

print('3.5.0 source integration applied')
