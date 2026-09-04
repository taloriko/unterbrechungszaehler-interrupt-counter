#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent


def read(rel: str) -> str:
    return (REPO / rel).read_text(encoding="utf-8")


def write(rel: str, text: str) -> None:
    (REPO / rel).write_text(text, encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"missing patch anchor: {label}")
    if text.count(old) != 1:
        raise RuntimeError(f"patch anchor not unique: {label} ({text.count(old)})")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# Source registry compile hardening
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/source_registry.cpp"
text = read(path)
text = text.replace("uint16_t size = sizeof(StoredRegistry);", "uint16_t size = 0;")
write(path, text)


# ---------------------------------------------------------------------------
# Self-describing mixed v2/v3 9-byte raw codec
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/interruption_store.cpp"
text = read(path)
start = text.index("void encodeRecord(")
end = text.index("bool readRecordAt", start)
codec = r'''// The 7 header bits that v2 used for 3-bit TimeSource, 3-bit EventSource
// and absolute-valid have 128 possible values. Only 60 are valid v2 states
// (TimeSource 0..4, EventSource 0..5). v3 maps its 16 source IDs x 4 persisted
// time states into 64 of the 68 patterns that are impossible for v2. Every
// 9-byte record is therefore self-describing; no ring rewrite/version cutoff
// is needed and old/new records can coexist through a complete ring turnover.
constexpr uint8_t V3_HEADER_ENCODE[64] = {
    5,6,7,13,14,15,21,22,23,29,30,31,37,38,39,45,
    46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
    62,63,69,70,71,77,78,79,85,86,87,93,94,95,101,102,
    103,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123
};

constexpr int8_t V3_HEADER_DECODE[128] = {
    -1,-1,-1,-1,-1,0,1,2,-1,-1,-1,-1,-1,3,4,5,
    -1,-1,-1,-1,-1,6,7,8,-1,-1,-1,-1,-1,9,10,11,
    -1,-1,-1,-1,-1,12,13,14,-1,-1,-1,-1,-1,15,16,17,
    18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,
    -1,-1,-1,-1,-1,34,35,36,-1,-1,-1,-1,-1,37,38,39,
    -1,-1,-1,-1,-1,40,41,42,-1,-1,-1,-1,-1,43,44,45,
    -1,-1,-1,-1,-1,46,47,48,-1,-1,-1,-1,-1,49,50,51,
    52,53,54,55,56,57,58,59,60,61,62,63,-1,-1,-1,-1
};

uint8_t persistedTimeCode(TimeTypes::Source source) {
  switch (source) {
    case TimeTypes::Source::Ntp: return 1;
    case TimeTypes::Source::Rtc: return 2;
    case TimeTypes::Source::Browser: return 3;
    case TimeTypes::Source::None:
    case TimeTypes::Source::Relative:
    default: return 0;
  }
}

TimeTypes::Source timeSourceFromV3Code(uint8_t code) {
  switch (code & 0x03U) {
    case 1: return TimeTypes::Source::Ntp;
    case 2: return TimeTypes::Source::Rtc;
    case 3: return TimeTypes::Source::Browser;
    case 0:
    default: return TimeTypes::Source::Relative;
  }
}

void encodeRecord(const InterruptionTypes::CapturedEvent &event, uint64_t sequence, uint8_t out[RECORD_SIZE]) {
  put32(out, event.timeValueSeconds);
  const uint8_t sourceId = event.sourceId <= 15U ? event.sourceId : 0U;
  const uint8_t ordinal = static_cast<uint8_t>(sourceId * 4U + persistedTimeCode(event.timeSource));
  const uint8_t header = V3_HEADER_ENCODE[ordinal];
  uint32_t packed = event.deltaSeconds & 0x1FFFFUL;
  packed |= static_cast<uint32_t>(header) << 17;
  out[4] = static_cast<uint8_t>(packed);
  out[5] = static_cast<uint8_t>(packed >> 8);
  out[6] = static_cast<uint8_t>(packed >> 16);
  out[7] = static_cast<uint8_t>(sequence & 0xFFU);
  out[8] = crc8(out, 8);
}

bool decodeRecord(const uint8_t in[RECORD_SIZE], InterruptionTypes::RawEvent &out) {
  if (crc8(in, 8) != in[8]) return false;
  const uint32_t packed = static_cast<uint32_t>(in[4]) |
                          (static_cast<uint32_t>(in[5]) << 8) |
                          (static_cast<uint32_t>(in[6]) << 16);
  const uint8_t header = static_cast<uint8_t>((packed >> 17) & 0x7FU);
  const uint8_t legacyTime = static_cast<uint8_t>(header & 0x07U);
  const uint8_t legacyEvent = static_cast<uint8_t>((header >> 3) & 0x07U);

  out = InterruptionTypes::RawEvent{};
  out.timeValueSeconds = get32(in);
  out.deltaSeconds = packed & 0x1FFFFUL;
  out.sequenceTag = in[7];

  // v2: preserve exact historic decoding. EventSource::Radio must NOT be
  // accepted here because legacy code 6 is deliberately an impossible-v2
  // pattern used by the self-identifying v3 codec.
  if (legacyTime <= static_cast<uint8_t>(TimeTypes::Source::Relative) &&
      legacyEvent <= static_cast<uint8_t>(InterruptionTypes::EventSource::Hardware)) {
    out.timeSource = static_cast<TimeTypes::Source>(legacyTime);
    out.eventSource = static_cast<InterruptionTypes::EventSource>(legacyEvent);
    out.sourceId = legacyEvent;
    out.absoluteValid = (header & 0x40U) != 0U;
    out.formatVersion = 2;
    return true;
  }

  const int8_t ordinal = V3_HEADER_DECODE[header];
  if (ordinal < 0) return false;
  out.sourceId = static_cast<uint8_t>(ordinal) >> 2;
  const uint8_t timeCode = static_cast<uint8_t>(ordinal) & 0x03U;
  out.timeSource = timeSourceFromV3Code(timeCode);
  out.absoluteValid = timeCode != 0U;
  out.eventSource = out.sourceId <= 5U
                        ? static_cast<InterruptionTypes::EventSource>(out.sourceId)
                        : InterruptionTypes::EventSource::Radio;
  out.formatVersion = 3;
  return true;
}

'''
text = text[:start] + codec + text[end:]
write(path, text)


# ---------------------------------------------------------------------------
# Interruption service carries stable source id without changing capture timing
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/interruption_service.cpp"
text = read(path)
text = replace_once(text, '#include "status_registry.h"\n', '#include "status_registry.h"\n#include "source_registry.h"\n', "service include")
text = replace_once(text,
                    '  currentSummary.lastEventSource = raw.eventSource;\n  currentSummary.lastDeltaSeconds = raw.deltaSeconds;',
                    '  currentSummary.lastEventSource = raw.eventSource;\n  currentSummary.lastSourceId = raw.sourceId;\n  currentSummary.lastDeltaSeconds = raw.deltaSeconds;',
                    "load last source id")
text = replace_once(text,
                    'void onGpioChanged(const char *channelId, bool logicalState) {\n  if (!logicalState || !channelId || strcmp(channelId, ProjectConfig::INTERRUPTION_INPUT_ID) != 0) return;\n  capture(InterruptionTypes::EventSource::PhysicalButton);\n}',
                    '''uint8_t defaultSourceId(InterruptionTypes::EventSource source) {
  switch (source) {
    case InterruptionTypes::EventSource::PhysicalButton: return SourceRegistry::SOURCE_ID_MASTER;
    case InterruptionTypes::EventSource::WebButton: return SourceRegistry::SOURCE_ID_WEB;
    case InterruptionTypes::EventSource::Software: return SourceRegistry::SOURCE_ID_SOFTWARE;
    case InterruptionTypes::EventSource::Api: return SourceRegistry::SOURCE_ID_API;
    case InterruptionTypes::EventSource::Hardware: return SourceRegistry::SOURCE_ID_HARDWARE;
    case InterruptionTypes::EventSource::Radio:
    case InterruptionTypes::EventSource::Unknown:
    default: return SourceRegistry::SOURCE_ID_UNKNOWN;
  }
}

void onGpioChanged(const char *channelId, bool logicalState) {
  if (!logicalState || !channelId || strcmp(channelId, ProjectConfig::INTERRUPTION_INPUT_ID) != 0) return;
  capture(InterruptionTypes::EventSource::PhysicalButton, SourceRegistry::SOURCE_ID_MASTER);
}''',
                    "physical source id")
text = replace_once(text,
                    'bool capture(InterruptionTypes::EventSource source) {\n  const TimeTypes::Snapshot snapshot = TimeService::eventTimestamp();',
                    'bool capture(InterruptionTypes::EventSource source, uint8_t sourceId) {\n  if (sourceId == SourceRegistry::SOURCE_ID_UNKNOWN) sourceId = defaultSourceId(source);\n  if (sourceId > SourceRegistry::SOURCE_ID_MAX) return false;\n  const TimeTypes::Snapshot snapshot = TimeService::eventTimestamp();',
                    "capture signature")
text = replace_once(text,
                    '  event.timeSource = snapshot.source;\n  event.eventSource = source;\n  event.absoluteValid = snapshot.valid;',
                    '  event.timeSource = snapshot.source;\n  event.eventSource = source;\n  event.sourceId = sourceId;\n  event.absoluteValid = snapshot.valid;',
                    "captured source id")
text = replace_once(text,
                    '  currentSummary.lastEventSource = source;\n  currentSummary.lastDeltaSeconds = event.deltaSeconds;',
                    '  currentSummary.lastEventSource = source;\n  currentSummary.lastSourceId = sourceId;\n  currentSummary.lastDeltaSeconds = event.deltaSeconds;',
                    "summary source id")
text = replace_once(text,
                    'SerialLog::infof("INTERRUPT", "Persisted | sequence=%llu | source=%s | time=%s | delta=%s | pending=%u",\n                   static_cast<unsigned long long>(sequence), InterruptionTypes::eventSourceName(event.eventSource),\n                   TimeTypes::sourceName(event.timeSource), deltaText, static_cast<unsigned int>(queueCount));',
                    'SerialLog::infof("INTERRUPT", "Persisted | sequence=%llu | source=%u:%s | kind=%s | time=%s | delta=%s | pending=%u",\n                   static_cast<unsigned long long>(sequence), static_cast<unsigned int>(event.sourceId),\n                   SourceRegistry::sourceName(event.sourceId), InterruptionTypes::eventSourceName(event.eventSource),\n                   TimeTypes::sourceName(event.timeSource), deltaText, static_cast<unsigned int>(queueCount));',
                    "persist log")
text = replace_once(text,
                    '  if (source == InterruptionTypes::EventSource::PhysicalButton) serviceUrgent();',
                    '  if (source == InterruptionTypes::EventSource::PhysicalButton || source == InterruptionTypes::EventSource::Radio) serviceUrgent();',
                    "radio immediate feedback")
text = replace_once(text,
                    'bool captureWeb() { return capture(InterruptionTypes::EventSource::WebButton); }',
                    'bool captureWeb() { return capture(InterruptionTypes::EventSource::WebButton, SourceRegistry::SOURCE_ID_WEB); }',
                    "web source id")
write(path, text)


# ---------------------------------------------------------------------------
# Start and service RF alongside the existing cooperative project loop
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/Unterbrechungszaehler.ino"
text = read(path)
text = replace_once(text, '#include "project_preferences.h"\n', '#include "project_preferences.h"\n#include "rf433_service.h"\n', "ino rf include")
text = replace_once(text,
                    '  InterruptionService::begin();\n\n  beginWebServer();',
                    '  InterruptionService::begin();\n  Rf433Service::begin();\n\n  beginWebServer();',
                    "ino rf begin")
text = replace_once(text,
                    '  HardwareRegistry::update();\n  InterruptionService::update();',
                    '  HardwareRegistry::update();\n  Rf433Service::update();\n  InterruptionService::update();',
                    "ino rf update")
write(path, text)


# ---------------------------------------------------------------------------
# API summary/CSV: persist numeric source id; names stay in the registry only
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/interruption_api.cpp"
text = read(path)
text = replace_once(text, '#include "project_time.h"\n', '#include "project_time.h"\n#include "rf433_service.h"\n#include "source_registry.h"\n', "api rf includes")
text = replace_once(text,
                    '    fieldString(out, "eventSource", InterruptionTypes::eventSourceName(summary.lastEventSource));',
                    '    fieldString(out, "eventSource", InterruptionTypes::eventSourceName(summary.lastEventSource));\n    fieldUInt(out, "sourceId", summary.lastSourceId);\n    fieldString(out, "sourceName", SourceRegistry::sourceName(summary.lastSourceId));',
                    "summary source metadata")
text = replace_once(text,
                    '  HardwareRegistry::update();\n  InterruptionService::serviceUrgent();',
                    '  HardwareRegistry::update();\n  Rf433Service::update();\n  InterruptionService::serviceUrgent();',
                    "analytics rf service")
text = replace_once(text,
                    '      "time_source,event_source,delta_previous_same_day_seconds,relative_seconds,time_valid\\r\\n";',
                    '      "time_source,event_source,source_id,delta_previous_same_day_seconds,relative_seconds,time_valid\\r\\n";',
                    "csv source header")
text = replace_once(text,
                    '    HardwareRegistry::update();\n    InterruptionService::update();',
                    '    HardwareRegistry::update();\n    Rf433Service::update();\n    InterruptionService::update();',
                    "csv rf service")
text = replace_once(text,
                    '        "%llu,%s,%s,%s,%s,%d,%d,%s,%s,%s,%lu,%s\\r\\n",',
                    '        "%llu,%s,%s,%s,%s,%d,%d,%s,%s,%u,%s,%lu,%s\\r\\n",',
                    "csv format")
text = replace_once(text,
                    '        TimeTypes::sourceName(raw.timeSource),\n        InterruptionTypes::eventSourceName(raw.eventSource),\n        delta,',
                    '        TimeTypes::sourceName(raw.timeSource),\n        InterruptionTypes::eventSourceName(raw.eventSource),\n        static_cast<unsigned int>(raw.sourceId),\n        delta,',
                    "csv source argument")
write(path, text)


# ---------------------------------------------------------------------------
# HTTP endpoints for pairing and source maintenance
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/web_server.cpp"
text = read(path)
text = replace_once(text, '#include "display_views.h"\n', '#include "display_views.h"\n#include "rf433_api.h"\n#include "rf433_service.h"\n#include "source_registry.h"\n', "web rf includes")
anchor = '''void handleInterruptionStorage() {
  sendJson(InterruptionApi::buildStorageJson());
}
'''
handlers = r'''void sendRfError(int status, const char *error) {
  String json;
  json.reserve(96);
  json += F("{\"ok\":false,\"error\":\"");
  json += error ? error : "rf_error";
  json += F("\"}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(status, "application/json; charset=utf-8", json);
}

void handleRfSources() {
  sendJson(Rf433Api::buildSourcesJson());
}

void handleRfLearn() {
  uint32_t sourceId = 0;
  if (server.hasArg("sourceId") && server.arg("sourceId").length() &&
      !parseUnsignedArg(server.arg("sourceId"), 0, SourceRegistry::SOURCE_ID_RADIO_LAST, sourceId)) {
    sendRfError(400, "invalid_source_id");
    return;
  }
  if (sourceId != 0U && sourceId < SourceRegistry::SOURCE_ID_RADIO_FIRST) {
    sendRfError(400, "invalid_source_id");
    return;
  }
  const String name = server.arg("name");
  if (!Rf433Service::startLearn(name.c_str(), static_cast<uint8_t>(sourceId))) {
    sendRfError(409, SourceRegistry::learnState().error);
    return;
  }
  sendJson(Rf433Api::buildSourcesJson());
}

void handleRfCancel() {
  Rf433Service::cancelLearn();
  sendJson(Rf433Api::buildSourcesJson());
}

void handleRfRename() {
  uint32_t sourceId = 0;
  const String name = server.arg("name");
  if (!parseUnsignedArg(server.arg("sourceId"), SourceRegistry::SOURCE_ID_RADIO_FIRST,
                        SourceRegistry::SOURCE_ID_RADIO_LAST, sourceId) || !name.length()) {
    sendRfError(400, "invalid_source");
    return;
  }
  if (!Rf433Service::renameSource(static_cast<uint8_t>(sourceId), name.c_str())) {
    sendRfError(409, "rename_failed");
    return;
  }
  sendJson(Rf433Api::buildSourcesJson());
}

void handleRfUnbind() {
  uint32_t sourceId = 0;
  if (!parseUnsignedArg(server.arg("sourceId"), SourceRegistry::SOURCE_ID_RADIO_FIRST,
                        SourceRegistry::SOURCE_ID_RADIO_LAST, sourceId)) {
    sendRfError(400, "invalid_source");
    return;
  }
  if (!Rf433Service::unbindSource(static_cast<uint8_t>(sourceId))) {
    sendRfError(409, "unbind_failed");
    return;
  }
  sendJson(Rf433Api::buildSourcesJson());
}

void handleInterruptionStorage() {
  sendJson(InterruptionApi::buildStorageJson());
}
'''
text = replace_once(text, anchor, handlers, "rf handlers")
text = replace_once(text,
                    '  server.on("/api/interruptions/preferences", HTTP_POST, handleProjectPreferences);\n  server.on("/api/interruptions/storage", HTTP_GET, handleInterruptionStorage);',
                    '  server.on("/api/interruptions/preferences", HTTP_POST, handleProjectPreferences);\n  server.on("/api/interruptions/sources", HTTP_GET, handleRfSources);\n  server.on("/api/interruptions/rf/learn", HTTP_POST, handleRfLearn);\n  server.on("/api/interruptions/rf/cancel", HTTP_POST, handleRfCancel);\n  server.on("/api/interruptions/sources/rename", HTTP_POST, handleRfRename);\n  server.on("/api/interruptions/sources/unbind", HTTP_POST, handleRfUnbind);\n  server.on("/api/interruptions/storage", HTTP_GET, handleInterruptionStorage);',
                    "rf routes")
write(path, text)


# ---------------------------------------------------------------------------
# Mark build clearly as test/dev OTA, not a release
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/config.h"
text = read(path)
text = replace_once(text, 'constexpr char SOFTWARE_VERSION[] = "3.2.0";', 'constexpr char SOFTWARE_VERSION[] = "3.3.0-dev433";', "dev version")
write(path, text)


# ---------------------------------------------------------------------------
# Web UI: lightweight source manager; no additional permanent timer
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/ui-src/app.js"
text = read(path)
text = replace_once(text,
                    "          { id: 'project-settings', titleKey: 'project.settings.title', descriptionKey: 'project.settings.desc', icon: 'settings', width: 'full', components: [\n            { type: 'projectSettings' }\n          ] }",
                    "          { id: 'project-settings', titleKey: 'project.settings.title', descriptionKey: 'project.settings.desc', icon: 'settings', width: 'full', components: [\n            { type: 'projectSettings' }\n          ] },\n          { id: 'rf433-sources', titleKey: 'card.rf433', descriptionKey: 'card.rf433.desc', icon: 'hardware', width: 'full', components: [\n            { type: 'rfSources' }\n          ] }",
                    "rf home card")

i18n = r'''
  const RF433_I18N = {
    de: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Funk', 'card.rf433': 'Funkbuttons / 433 MHz',
      'card.rf433.desc': 'CC1101-Testbetrieb: feste Funkcodes werden stabilen Source-IDs zugeordnet. Namen stehen nur in der Konfiguration, nie im Event.',
      'rf433.ready': 'Empfänger bereit', 'rf433.offline': 'Empfänger nicht bereit', 'rf433.learning': 'Anlernen aktiv',
      'rf433.newName': 'Name des neuen Buttons', 'rf433.learnNew': 'Neuen Button anlernen', 'rf433.cancel': 'Abbrechen', 'rf433.refresh': 'Aktualisieren',
      'rf433.pressButton': 'Jetzt den gewünschten Funkbutton mehrfach drücken.', 'rf433.noSources': 'Noch kein Funkbutton angelernt.',
      'rf433.sourceId': 'Source-ID', 'rf433.retained': 'Rohereignisse im Ring', 'rf433.bound': 'Gebunden', 'rf433.unbound': 'Nicht gebunden',
      'rf433.rename': 'Umbenennen', 'rf433.replace': 'Sender ersetzen', 'rf433.unbind': 'Sender lösen', 'rf433.lastFrame': 'Letzter Funkcode',
      'rf433.error': 'Funkaktion fehlgeschlagen', 'rf433.capacity': '{n} von 10 Funkquellen vergeben'
    },
    en: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Radio', 'card.rf433': 'Radio buttons / 433 MHz',
      'card.rf433.desc': 'CC1101 test mode: fixed radio codes are mapped to stable source IDs. Names live only in configuration, never in each event.',
      'rf433.ready': 'Receiver ready', 'rf433.offline': 'Receiver not ready', 'rf433.learning': 'Learn mode active',
      'rf433.newName': 'Name of new button', 'rf433.learnNew': 'Learn new button', 'rf433.cancel': 'Cancel', 'rf433.refresh': 'Refresh',
      'rf433.pressButton': 'Press the desired radio button several times now.', 'rf433.noSources': 'No radio button learned yet.',
      'rf433.sourceId': 'Source ID', 'rf433.retained': 'Raw events retained', 'rf433.bound': 'Bound', 'rf433.unbound': 'Not bound',
      'rf433.rename': 'Rename', 'rf433.replace': 'Replace transmitter', 'rf433.unbind': 'Unbind transmitter', 'rf433.lastFrame': 'Last radio code',
      'rf433.error': 'Radio action failed', 'rf433.capacity': '{n} of 10 radio sources assigned'
    },
    it: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Radio', 'card.rf433': 'Pulsanti radio / 433 MHz',
      'card.rf433.desc': 'Test CC1101: i codici radio fissi vengono associati a ID sorgente stabili. I nomi restano solo nella configurazione.',
      'rf433.ready': 'Ricevitore pronto', 'rf433.offline': 'Ricevitore non pronto', 'rf433.learning': 'Apprendimento attivo',
      'rf433.newName': 'Nome del nuovo pulsante', 'rf433.learnNew': 'Apprendi nuovo pulsante', 'rf433.cancel': 'Annulla', 'rf433.refresh': 'Aggiorna',
      'rf433.pressButton': 'Premere ora più volte il pulsante radio desiderato.', 'rf433.noSources': 'Nessun pulsante radio appreso.',
      'rf433.sourceId': 'ID sorgente', 'rf433.retained': 'Eventi grezzi nel ring', 'rf433.bound': 'Associato', 'rf433.unbound': 'Non associato',
      'rf433.rename': 'Rinomina', 'rf433.replace': 'Sostituisci trasmettitore', 'rf433.unbind': 'Scollega trasmettitore', 'rf433.lastFrame': 'Ultimo codice radio',
      'rf433.error': 'Azione radio non riuscita', 'rf433.capacity': '{n} di 10 sorgenti radio assegnate'
    },
    fr: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Radio', 'card.rf433': 'Boutons radio / 433 MHz',
      'card.rf433.desc': 'Mode test CC1101 : les codes radio fixes sont associés à des ID source stables. Les noms restent uniquement dans la configuration.',
      'rf433.ready': 'Récepteur prêt', 'rf433.offline': 'Récepteur indisponible', 'rf433.learning': 'Apprentissage actif',
      'rf433.newName': 'Nom du nouveau bouton', 'rf433.learnNew': 'Apprendre un bouton', 'rf433.cancel': 'Annuler', 'rf433.refresh': 'Actualiser',
      'rf433.pressButton': 'Appuyer maintenant plusieurs fois sur le bouton radio souhaité.', 'rf433.noSources': 'Aucun bouton radio appris.',
      'rf433.sourceId': 'ID source', 'rf433.retained': 'Événements bruts conservés', 'rf433.bound': 'Associé', 'rf433.unbound': 'Non associé',
      'rf433.rename': 'Renommer', 'rf433.replace': 'Remplacer émetteur', 'rf433.unbind': 'Dissocier émetteur', 'rf433.lastFrame': 'Dernier code radio',
      'rf433.error': 'Action radio échouée', 'rf433.capacity': '{n} sur 10 sources radio attribuées'
    },
    swg: {
      'status.rf433': '433 MHz', 'event.source.radio': 'Funk', 'card.rf433': 'Funkknöpf / 433 MHz',
      'card.rf433.desc': 'CC1101-Test: feste Funkcodes krieget stabile Source-IDs. Dr Name steht bloß in dr Konfiguration.',
      'rf433.ready': 'Empfänger bereit', 'rf433.offline': 'Empfänger net bereit', 'rf433.learning': 'Anlerna läuft',
      'rf433.newName': 'Name vom neia Knopf', 'rf433.learnNew': 'Neia Knopf anlerna', 'rf433.cancel': 'Abbrecha', 'rf433.refresh': 'Neu lada',
      'rf433.pressButton': 'Jetzt dr gewünschte Funkknopf mehrafach drucka.', 'rf433.noSources': 'No koi Funkknopf anglernt.',
      'rf433.sourceId': 'Source-ID', 'rf433.retained': 'Rohereignis em Ring', 'rf433.bound': 'Gebunda', 'rf433.unbound': 'Net gebunda',
      'rf433.rename': 'Umbenenna', 'rf433.replace': 'Sender ersetza', 'rf433.unbind': 'Sender lösa', 'rf433.lastFrame': 'Letschter Funkcode',
      'rf433.error': 'Funkaktion isch schief ganga', 'rf433.capacity': '{n} vo 10 Funkquella vergebba'
    }
  };
  Object.assign(I18N.de, RF433_I18N.de);
  Object.assign(I18N.en, RF433_I18N.en);
  Object.assign(I18N.it, RF433_I18N.it);
  Object.assign(I18N.fr, RF433_I18N.fr);
  Object.assign(I18N.swg, RF433_I18N.swg);
  Object.assign(I18N['swg-alb'], RF433_I18N.swg);
  Object.assign(I18N['swg-ob'], RF433_I18N.swg);

'''
text = replace_once(text, "  const state = {\n", i18n + "  const state = {\n", "rf i18n")
text = replace_once(text,
                    "    projectSettings: { soundEnabled: true, soundVolume: 100, soundMode: 'rotate', soundTrack: 2, soundTrackCount: 0, language: 'en', languageStored: false, displayEnabled: true, displayRotation180: false, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 65, displayDimAfterMinutes: 10, displayDimBrightness: 5 },\n    analytics:",
                    "    projectSettings: { soundEnabled: true, soundVolume: 100, soundMode: 'rotate', soundTrack: 2, soundTrackCount: 0, language: 'en', languageStored: false, displayEnabled: true, displayRotation180: false, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 65, displayDimAfterMinutes: 10, displayDimBrightness: 5 },\n    rf433: { rf: { ready: false }, learn: { active: false }, sources: [], assignedRadio: 0, radioCapacity: 10 },\n    analytics:",
                    "rf state")
text = replace_once(text,
                    "      lastMeta.textContent = last.available ? `${t(`time.source.${last.timeSource || 'relative'}`)} · ${t(eventSourceKey)}` : '';",
                    "      const sourceLabel = last.sourceName || t(eventSourceKey);\n      lastMeta.textContent = last.available ? `${t(`time.source.${last.timeSource || 'relative'}`)} · ${sourceLabel}` : '';",
                    "home source name")

rf_renderer = r'''
  function renderRfSources() {
    const root = el('div', 'rf433-panel');
    const head = el('div', 'rf433-head');
    const status = el('strong', 'rf433-status');
    const capacity = el('span', 'rf433-capacity');
    head.append(status, capacity);

    const learnInfo = el('div', 'rf433-learn-info');
    const toolbar = el('div', 'rf433-toolbar');
    const nameInput = el('input', 'project-setting-input');
    nameInput.type = 'text'; nameInput.maxLength = 23; nameInput.placeholder = t('rf433.newName'); nameInput.dataset.rfNewName = '1';
    const learnButton = el('button', 'button'); learnButton.type = 'button'; learnButton.dataset.rfAction = 'learn-new'; learnButton.textContent = t('rf433.learnNew');
    const cancelButton = el('button', 'button'); cancelButton.type = 'button'; cancelButton.dataset.rfAction = 'cancel'; cancelButton.textContent = t('rf433.cancel');
    const refreshButton = el('button', 'button'); refreshButton.type = 'button'; refreshButton.dataset.rfAction = 'refresh'; refreshButton.textContent = t('rf433.refresh');
    toolbar.append(nameInput, learnButton, cancelButton, refreshButton);

    const lastFrame = el('div', 'form-note rf433-last-frame');
    const list = el('div', 'rf433-source-list');
    const message = el('div', 'project-setting-message'); message.setAttribute('aria-live', 'polite'); message.dataset.rfMessage = '1';
    root.append(head, learnInfo, toolbar, lastFrame, list, message);

    const update = () => {
      const data = state.rf433 || {};
      const rf = data.rf || {};
      const learn = data.learn || {};
      status.textContent = t(rf.ready ? 'rf433.ready' : 'rf433.offline');
      status.dataset.state = rf.ready ? 'ok' : 'error';
      capacity.textContent = t('rf433.capacity').replace('{n}', String(Number(data.assignedRadio || 0)));
      learnInfo.textContent = learn.active
        ? `${t('rf433.learning')} · ${t('rf433.pressButton')} (${Math.ceil(Number(learn.remainingMs || 0) / 1000)} s)`
        : '';
      cancelButton.hidden = !learn.active;
      learnButton.disabled = !rf.ready || !!learn.active || Number(data.assignedRadio || 0) >= Number(data.radioCapacity || 10);
      nameInput.disabled = !rf.ready || !!learn.active;
      if (Number(rf.lastCode || 0) > 0) {
        lastFrame.textContent = `${t('rf433.lastFrame')}: 0x${Number(rf.lastCode).toString(16).toUpperCase().padStart(8, '0')} · ${Number(rf.lastBits || 0)} bit`;
      } else lastFrame.textContent = '';

      list.replaceChildren();
      const sources = Array.isArray(data.sources) ? data.sources.filter(source => source.kind === 'radio') : [];
      if (!sources.length) {
        const empty = el('div', 'form-note'); empty.textContent = t('rf433.noSources'); list.append(empty); return;
      }
      for (const source of sources) {
        const row = el('article', 'rf433-source-row'); row.dataset.rfSourceId = String(source.id);
        const meta = el('div', 'rf433-source-meta');
        const id = el('strong'); id.textContent = `${t('rf433.sourceId')} ${source.id}`;
        const retained = el('span'); retained.textContent = `${t('rf433.retained')}: ${Number(source.retainedCount || 0)}`;
        const bound = el('span'); bound.textContent = t(source.bound ? 'rf433.bound' : 'rf433.unbound');
        meta.append(id, retained, bound);
        const edit = el('div', 'rf433-source-edit');
        const input = el('input', 'project-setting-input'); input.type = 'text'; input.maxLength = 23; input.value = source.name || ''; input.dataset.rfSourceName = String(source.id);
        const rename = el('button', 'button'); rename.type = 'button'; rename.dataset.rfAction = 'rename'; rename.textContent = t('rf433.rename');
        const replace = el('button', 'button'); replace.type = 'button'; replace.dataset.rfAction = 'replace'; replace.textContent = t('rf433.replace');
        const unbind = el('button', 'button'); unbind.type = 'button'; unbind.dataset.rfAction = 'unbind'; unbind.textContent = t('rf433.unbind'); unbind.disabled = !source.bound;
        edit.append(input, rename, replace, unbind);
        row.append(meta, edit); list.append(row);
      }
    };
    Bindings.add('rf433', update);
    queueMicrotask(() => Transport.loadRfSources());
    return root;
  }

'''
text = replace_once(text, "  function localeForLabels() {", rf_renderer + "  function localeForLabels() {", "rf renderer")
text = replace_once(text,
                    "projectSettings: renderProjectSettings, heatmapHourly:",
                    "projectSettings: renderProjectSettings, rfSources: renderRfSources, heatmapHourly:",
                    "rf component renderer")

transport_methods = r'''
    async loadRfSources() {
      if (this._rfBusy) return;
      this._rfBusy = true;
      try {
        const data = await this.requestResult('/api/interruptions/sources');
        patchState({ rf433: data });
        Bindings.notify('rf433');
      } catch (error) {
        console.warn('RF433 source load failed:', error);
        const message = document.querySelector('[data-rf-message]');
        if (message) { message.textContent = t('rf433.error'); message.dataset.state = 'error'; }
      } finally { this._rfBusy = false; }
    },
    async startRfLearn(sourceId, name) {
      const parts = [`name=${encodeURIComponent(String(name || '').trim())}`];
      if (Number(sourceId) > 0) parts.push(`sourceId=${encodeURIComponent(sourceId)}`);
      try {
        const data = await this.requestResult(`/api/interruptions/rf/learn?${parts.join('&')}`, { method: 'POST' });
        patchState({ rf433: data }); Bindings.notify('rf433');
      } catch (error) { console.warn('RF433 learn failed:', error); alert(`${t('rf433.error')}: ${error.code || 'unknown'}`); }
    },
    async cancelRfLearn() {
      try { const data = await this.requestResult('/api/interruptions/rf/cancel', { method: 'POST' }); patchState({ rf433: data }); Bindings.notify('rf433'); }
      catch (error) { console.warn('RF433 cancel failed:', error); }
    },
    async renameRfSource(sourceId, name) {
      try {
        const data = await this.requestResult(`/api/interruptions/sources/rename?sourceId=${encodeURIComponent(sourceId)}&name=${encodeURIComponent(String(name || '').trim())}`, { method: 'POST' });
        patchState({ rf433: data }); Bindings.notify('rf433');
      } catch (error) { console.warn('RF433 rename failed:', error); alert(t('rf433.error')); }
    },
    async unbindRfSource(sourceId) {
      try {
        const data = await this.requestResult(`/api/interruptions/sources/unbind?sourceId=${encodeURIComponent(sourceId)}`, { method: 'POST' });
        patchState({ rf433: data }); Bindings.notify('rf433');
      } catch (error) { console.warn('RF433 unbind failed:', error); alert(t('rf433.error')); }
    },
'''
text = replace_once(text, "    async liveInterruptionTick() {", transport_methods + "    async liveInterruptionTick() {", "rf transport")
text = replace_once(text,
                    "      if (state.activeView === 'analytics' && state.analytics.dirty && !state.analytics.loading) this.loadAnalytics(true);\n    },",
                    "      if (state.activeView === 'analytics' && state.analytics.dirty && !state.analytics.loading) this.loadAnalytics(true);\n      if (state.activeView === 'home' && state.rf433?.learn?.active && (!this._lastRfTick || now - this._lastRfTick >= 1000)) {\n        this._lastRfTick = now; this.loadRfSources();\n      }\n    },",
                    "rf learn poll")
click_anchor = "    const hardwareActionButton = event.target.closest('[data-hardware-action]');\n"
click_code = r'''    const rfButton = event.target.closest('[data-rf-action]');
    if (rfButton) {
      const action = rfButton.dataset.rfAction || '';
      const row = rfButton.closest('[data-rf-source-id]');
      const sourceId = row ? Number(row.dataset.rfSourceId || 0) : 0;
      const sourceNameInput = row ? row.querySelector('[data-rf-source-name]') : null;
      const root = rfButton.closest('.rf433-panel');
      if (action === 'learn-new') await Transport.startRfLearn(0, root?.querySelector('[data-rf-new-name]')?.value || '');
      else if (action === 'cancel') await Transport.cancelRfLearn();
      else if (action === 'refresh') await Transport.loadRfSources();
      else if (action === 'rename') await Transport.renameRfSource(sourceId, sourceNameInput?.value || '');
      else if (action === 'replace') await Transport.startRfLearn(sourceId, sourceNameInput?.value || '');
      else if (action === 'unbind') await Transport.unbindRfSource(sourceId);
      return;
    }
'''
text = replace_once(text, click_anchor, click_code + click_anchor, "rf click actions")
write(path, text)


# Minimal responsive styling appended to existing toolkit CSS.
path = "Unterbrechungszaehler/ui-src/app.css"
text = read(path)
if ".rf433-panel" not in text:
    text += r'''

/* RF433 prototype source manager: intentionally reuses existing controls. */
.rf433-panel { display:grid; gap:12px; }
.rf433-head { display:flex; gap:12px; justify-content:space-between; align-items:center; flex-wrap:wrap; }
.rf433-status[data-state="ok"] { color:var(--success); }
.rf433-status[data-state="error"] { color:var(--danger); }
.rf433-toolbar { display:grid; grid-template-columns:minmax(180px,1fr) auto auto auto; gap:8px; align-items:center; }
.rf433-learn-info:empty,.rf433-last-frame:empty { display:none; }
.rf433-source-list { display:grid; gap:8px; }
.rf433-source-row { display:grid; grid-template-columns:minmax(180px,.7fr) minmax(280px,1.3fr); gap:12px; padding:10px; border:1px solid var(--border); border-radius:10px; }
.rf433-source-meta { display:flex; gap:10px; align-items:center; flex-wrap:wrap; }
.rf433-source-edit { display:grid; grid-template-columns:minmax(140px,1fr) auto auto auto; gap:8px; align-items:center; }
@media (max-width:760px) {
  .rf433-toolbar,.rf433-source-row,.rf433-source-edit { grid-template-columns:1fr; }
}
'''
write(path, text)


# ---------------------------------------------------------------------------
# Host invariants for collision-free mixed codec and exact 10 RF IDs
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/tools/test_interruption_storage.py"
text = read(path)
insert_before = "\ndef main() -> None:\n"
v3_tests = r'''

V3_HEADER_ENCODE = [
    5,6,7,13,14,15,21,22,23,29,30,31,37,38,39,45,
    46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
    62,63,69,70,71,77,78,79,85,86,87,93,94,95,101,102,
    103,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,
]
V3_HEADER_DECODE = {header: ordinal for ordinal, header in enumerate(V3_HEADER_ENCODE)}


def legacy_header_valid(header: int) -> bool:
    time_source = header & 0x07
    event_source = (header >> 3) & 0x07
    return time_source <= 4 and event_source <= 5


def encode_record_v3(time_value: int, delta: int, source_id: int, time_code: int, sequence: int) -> bytes:
    assert 0 <= source_id <= 15 and 0 <= time_code <= 3
    header = V3_HEADER_ENCODE[source_id * 4 + time_code]
    packed = (delta & 0x1FFFF) | (header << 17)
    payload = bytearray(RAW_RECORD_SIZE)
    payload[0:4] = int(time_value).to_bytes(4, "little")
    payload[4:7] = int(packed).to_bytes(3, "little")
    payload[7] = sequence & 0xFF
    payload[8] = crc8(payload[:8])
    return bytes(payload)


def decode_mixed(record: bytes) -> tuple[int, int, int, int, int]:
    assert len(record) == RAW_RECORD_SIZE and crc8(record[:8]) == record[8]
    packed = int.from_bytes(record[4:7], "little")
    header = (packed >> 17) & 0x7F
    legacy_time = header & 0x07
    legacy_source = (header >> 3) & 0x07
    if legacy_header_valid(header):
        return (2, legacy_source, legacy_time, packed & 0x1FFFF, record[7])
    ordinal = V3_HEADER_DECODE[header]
    return (3, ordinal >> 2, ordinal & 0x03, packed & 0x1FFFF, record[7])


def test_multisource_v3_self_describing_codec() -> None:
    assert len(V3_HEADER_ENCODE) == 64
    assert len(set(V3_HEADER_ENCODE)) == 64
    assert all(0 <= header < 128 for header in V3_HEADER_ENCODE)
    assert all(not legacy_header_valid(header) for header in V3_HEADER_ENCODE)
    assert 15 - 6 + 1 == 10

    for source_id in range(16):
        for time_code in range(4):
            record = encode_record_v3(1_788_339_733, 1234, source_id, time_code, 0x3456)
            assert len(record) == 9
            assert decode_mixed(record) == (3, source_id, time_code, 1234, 0x56)

    # Existing v2 bytes remain byte-for-byte decodable with their legacy source.
    legacy = encode_record(1_788_339_733, 900, 1, 5, True, 0x77)
    version, source_id, time_source, delta, tag = decode_mixed(legacy)
    assert (version, source_id, time_source, delta, tag) == (2, 5, 1, 900, 0x77)

'''
text = replace_once(text, insert_before, v3_tests + insert_before, "v3 host tests")
text = replace_once(text,
                    "        test_record_layout,\n        test_ring_wrap_100k,",
                    "        test_record_layout,\n        test_multisource_v3_self_describing_codec,\n        test_ring_wrap_100k,",
                    "v3 test list")
write(path, text)


# ---------------------------------------------------------------------------
# Portable checks adapted for the dev OTA and RF prototype
# ---------------------------------------------------------------------------
path = "Unterbrechungszaehler/tools/release_check.py"
text = read(path)
text = text.replace("Unterbrechungszaehler 3.2.0", "Unterbrechungszaehler 3.3.0-dev433")
text = replace_once(text, 'check(\'SOFTWARE_VERSION[] = "3.2.0"\' in config, "project version 3.2.0")',
                    'check(\'SOFTWARE_VERSION[] = "3.3.0-dev433"\' in config, "prototype version 3.3.0-dev433")', "release check version")
needle = '    check("PENDING_EVENT_CAPACITY = 64" in project, "64-event fixed persistence queue")\n'
addition = '''    check("PENDING_EVENT_CAPACITY = 64" in project, "64-event fixed persistence queue")
    source_registry = (ROOT / "source_registry.h").read_text(encoding="utf-8")
    raw_store = (ROOT / "interruption_store.cpp").read_text(encoding="utf-8")
    check("SOURCE_ID_RADIO_FIRST = 6" in source_registry and "SOURCE_ID_RADIO_LAST = 15" in source_registry, "exactly ten RF logical source ids")
    check("V3_HEADER_ENCODE[64]" in raw_store and "V3_HEADER_DECODE[128]" in raw_store, "self-describing mixed v2/v3 raw codec")
    check("out.sourceId" in raw_store and "RAW_RECORD_SIZE = 9" in project, "source id stored without growing raw records")
    check("RF433_SCK_PIN = 14" in hardware and "RF433_MISO_PIN = 32" in hardware and "RF433_MOSI_PIN = 23" in hardware and "RF433_CS_PIN = 25" in hardware and "RF433_GDO0_PIN = 26" in hardware and "RF433_GDO2_PIN = 27" in hardware, "CC1101 pin map")
    check((ROOT / "rf433_cc1101.cpp").exists() and (ROOT / "source_registry.cpp").exists(), "RF receiver and source registry modules")
'''
text = replace_once(text, needle, addition, "rf release checks")
text = replace_once(text,
                    '        "/api/interruptions/preferences",\n        "/api/interruptions/storage",',
                    '        "/api/interruptions/preferences",\n        "/api/interruptions/sources",\n        "/api/interruptions/rf/learn",\n        "/api/interruptions/rf/cancel",\n        "/api/interruptions/sources/rename",\n        "/api/interruptions/sources/unbind",\n        "/api/interruptions/storage",',
                    "rf API route checks")
text = replace_once(text,
                    '    check("projectSettings: renderProjectSettings" in JS, "Home project settings card")',
                    '    check("projectSettings: renderProjectSettings" in JS, "Home project settings card")\n    check("rfSources: renderRfSources" in JS and "card.rf433" in JS, "RF source manager in Home UI")',
                    "rf UI check")
write(path, text)


# ---------------------------------------------------------------------------
# Test/wiring documentation
# ---------------------------------------------------------------------------
rf_doc = r'''# 433-MHz-/CC1101-Prototyp testen

> **Entwicklungsstand `3.3.0-dev433` – kein Release.** Der Code liegt nur im Draft-PR #12. Die OTA aus GitHub Actions ist ausschließlich für Hardwaretests gedacht.

## Ziel

Der ESP32 bleibt Master. Der lokale DI1-Taster auf GPIO13 funktioniert weiter. Ein CC1101/RF1100SE empfängt zusätzlich 433,92-MHz-OOK-Festcode-Taster. Jeder angelernte Sender wird einer **stabilen Source-ID** zugeordnet; der frei vergebene Name steht nur einmal in der SourceRegistry und wird nicht in jedem Event dupliziert.

## Verdrahtung CC1101 / RF1100SE

| CC1101 | ESP32 |
|---|---:|
| VCC | **3,3 V** |
| GND | GND |
| SCK | GPIO14 |
| MISO / SO | GPIO32 |
| MOSI / SI | GPIO23 |
| CSN / SS | GPIO25 |
| GDO0 | GPIO26 |
| GDO2 | GPIO27 |

**CC1101 nur mit 3,3 V versorgen.** Eine passende 433-MHz-Antenne verbessert den Test erheblich.

## Welche Taster unterstützt der erste Prototyp?

Bewusst eng: gängige **433,92-MHz-ASK/OOK-Festcode-Sender** mit etwa 20–32 Bit und kurzen/langen Pulspaaren. Das ist noch kein universeller 433-MHz-Decoder. Rolling Code, Keeloq und unbekannte/proprietäre Protokolle sind nicht zugesichert.

Die Firmware verlangt zwei übereinstimmende empfangene Frames, bevor ein Tastendruck akzeptiert wird, und unterdrückt die Wiederholungen eines einzelnen Funk-Tastendrucks anschließend kurz. Damit soll ein typischer Sender mit mehreren identischen Wiederholtelegrammen genau eine Unterbrechung erzeugen.

## Anlernen

1. `3.3.0-dev433` OTA installieren.
2. Weboberfläche → **Home → Funkbuttons / 433 MHz**.
3. Namen eingeben, z. B. `Anna`.
4. **Neuen Button anlernen** wählen.
5. Gewünschten Funkbutton innerhalb von 30 Sekunden mehrfach drücken.
6. Der Anlerndruck wird absichtlich **nicht** als Unterbrechung gezählt.
7. Danach normal drücken. Der Event läuft über denselben `InterruptionService` wie Master/Web und bekommt seine stabile Source-ID.

Es stehen im ersten Prototyp genau **10 Funk-IDs (6–15)** zur Verfügung. IDs 0–5 bleiben für Unknown/Master/Web/Software/API/Technik reserviert.

## Sender ersetzen ohne Historie umzubenennen

Bei einem defekten Sender in derselben Quellenzeile **Sender ersetzen** wählen und den neuen Sender drücken. Die Source-ID und damit die Zuordnung aller historischen Events bleibt gleich; nur die physische RF-Bindung wird aktualisiert.

**Sender lösen** entfernt nur die Funkbindung. Die Source-ID und der Name werden absichtlich nicht automatisch freigegeben oder wiederverwendet.

## Speicherung

Das Raw-Event bleibt **9 Byte** groß, der Ring bleibt bei **100.000 Events**. Neue Records tragen eine 4-Bit-Source-ID in einem selbstidentifizierenden v3-Bitlayout. Alte v2-Records werden unverändert weitergelesen. Namen/RF-Codes liegen in einer kleinen CRC-geschützten NVS-Registry.

CSV enthält die numerische `source_id`; Namen werden nicht pro Event gespeichert. Die Weboberfläche löst Namen über die SourceRegistry auf.

## Was beim Hardwaretest beobachten?

- Bootlog: `CC1101 ready ...`
- Headerstatus `433 MHz` sollte OK sein.
- Beim Anlernen sollte die neue Quelle erscheinen.
- Ein einzelner menschlicher Tastendruck darf trotz RF-Wiederholtelegrammen nur **ein** Event ergeben.
- Master-DI1 und Webbutton müssen parallel weiter funktionieren.
- Nach Neustart müssen Name, Source-ID und Senderbindung erhalten bleiben.
- Sender ersetzen muss dieselbe Source-ID behalten.

Wenn ein Sender nicht erkannt wird, sind `lastCode`, `lastBits`, `rejectedFrames` und `overflowFrames` über `/api/interruptions/sources` als Diagnosewerte verfügbar.
'''
write("Unterbrechungszaehler/RF433_TEST.md", rf_doc)

path = "Unterbrechungszaehler/ADR_433MHZ_MULTI_SOURCE.md"
text = read(path)
marker = "\n"
refinement = r'''
> **Prototyp-Verfeinerung nach der ersten Analyse:** Für die konkrete Team-Zielgröße werden nicht 5, sondern **4 Bit Source-ID** verwendet. Das ergibt 16 IDs insgesamt: 0–5 für bestehende/technische Quellen und 6–15 für exakt 10 Funkquellen. Die 64 Kombinationen aus 16 Source-IDs × 4 persistierten Zeitquellen werden in 64 Headerwerte gelegt, die im alten v2-Format ungültig sind. Dadurch erkennt jeder 9-Byte-Record sein Format selbst. Ein separater Sequenz-Cutover und ein Raw-Ring-Rewrite sind nicht nötig. Diese Variante ist für den Testprototyp maßgeblich und ersetzt die weiter unten dokumentierte frühere 5-Bit-/Cutover-Idee.

'''
first_newline = text.find("\n") + 1
if "Prototyp-Verfeinerung" not in text:
    text = text[:first_newline] + refinement + text[first_newline:]
write(path, text)

print("RF433 prototype patches applied")
