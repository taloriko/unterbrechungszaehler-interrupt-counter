# Projektarchitektur – Unterbrechungszähler 0.2.0

Basis: **ESP32 UI Base FINAL 1.6.0**. Die Basis bleibt Infrastruktur; die Bedeutung „Unterbrechung“ beginnt ausschließlich in der Projektschicht.

## Schichten

```text
GPIO DI1 ───────────────┐
Webbutton ──────────────┤
spätere Quellen ────────┤
                        ▼
               InterruptionService
                        │
              TimeService::eventTimestamp()
                        │
          ┌─────────────┼──────────────┐
          ▼             ▼              ▼
      RAM Summary   DisplayViews   AudioDySv17f
          │
          ▼
   feste PendingQueue (64)
          │
          ▼
   InterruptionStore (Raw Ring)
          │
          ▼
 InterruptionAggregates (Daily)
          │
          ├──────── Heatmap API
          └──────── CSV liest Raw Ring direkt
```

`GpioModule`, `TimeService`, `DisplaySh1106`, `AudioDySv17f` und `WebServer` kennen den Begriff „Unterbrechung“ nicht. Projektbedeutung, Zähler und Persistenz werden nicht in diese Basismodule geschoben.

## Zeitkritischer Capture-Pfad

`InterruptionService::capture()` erledigt nur kleine, begrenzte Operationen:

1. atomaren Snapshot aus dem bereits laufenden `TimeService`
2. lokale Kalenderableitung über `ProjectTime`, falls absolute Zeit gültig ist
3. Abstand zum letzten gültigen Event desselben lokalen Tages bestimmen
4. Event in eine feste 64er PendingQueue kopieren
5. RAM-Summary / Revisionsnummer aktualisieren
6. Display-/Audiofeedback vormerken

**Nicht** im Capture-Pfad: LittleFS, NTP, RTC-I2C, Heatmapscan, CSV oder Statistik-Neuberechnung.

Beim **physischen** Taster wird direkt nach erfolgreichem Capture `serviceUrgent()` aufgerufen. Das stößt zuerst die OLED-Rückmeldung und danach den Audiobefehl an; Persistenz folgt erst im regulären Projekt-`update()`. Beim Webbutton wird zuerst die HTTP-Antwort zurückgegeben und Feedback im nächsten Loopdurchlauf (typisch wenige Millisekunden später) angestoßen.

DI1 besitzt zusätzlich einen **minimalen aktiven Edge-Latch per Interrupt**. Die ISR setzt nur ein `volatile bool`; sie führt weder SerialLog, Netzwerk, Dateisystem noch Projektcallbacks aus. Im nächsten `GpioModule::update()` wird das Flag atomar übernommen und die normale Entprell-/Callbacklogik ausgeführt. Damit überlebt ein menschlicher kurzer Tastendruck auch einen vorübergehend blockierenden synchronen TCP-/CSV-Schreibabschnitt. Zwei vollständige extrem schnelle Drückzyklen, die beide komplett in demselben blockierenden Abschnitt liegen, können absichtlich zu einem Latch zusammenfallen – dies ist ein robuster Human-Button-Latch, kein Hochfrequenz-Pulszähler.

## Persistenzpipeline

```text
CAPTURED (RAM)
   ↓
PENDING (feste Queue)
   ↓
Raw-Record schreiben
   ↓
Raw-Metadaten committen
   ↓
PERSISTED
   ↓
Daily Aggregate aktualisieren
```

Der Raw-Ring ist Source of Truth. Ein Aggregatefehler kann keinen bereits persistenten Unterbrechungsdatensatz löschen. Details inklusive Transaktions-/Recoveryregeln stehen in `STORAGE_FORMAT.md`.

## Live-Summary und Revision

`liveSequence` zählt echte Unterbrechungsereignisse. Zusätzlich besitzt der Summary-State eine unabhängige `revision`.

Die Revision steigt auch bei sichtbaren Zustandsänderungen ohne neues Event, z. B.:

- Pending → persisted
- Storagezustand geändert
- Soundeinstellung geändert
- Tageswechsel / Heute-Zähler auf 0
- reparierte Aggregate werden wieder maßgeblich

Der konditionale Home-Endpunkt vergleicht diese Revision. Dadurch sieht ein offener Browser auch Mitternachtsreset und Persistenzstatus, obwohl keine neue Unterbrechungssequenz entstanden ist.

## Browser / Live-Transport

Home hält nur den kleinen Summary-State; keine Rohhistorie.

Der einzige wiederkehrende Frontendtimer bleibt der 1-s-UI-Tick. Nur wenn **Home oder Auswertung aktiv und das Dokument sichtbar** ist, wird der kleine Live-Endpunkt abgefragt. Home aktualisiert damit physische Tasterereignisse; die sichtbare Auswertung erkennt über dieselbe Revision neue Hardwareevents und lädt Heatmaps erst nach einem kurzen Deferred-Refresh neu. Unverändert → HTTP 204. Gerät/Einstellungen und unsichtbare Browser-Tabs erzeugen keine Live-Requests.

Das bleibt bewusst eine austauschbare Transportentscheidung: Die Projektlogik hängt nicht von Polling ab und könnte später hinter derselben Summary-Schnittstelle SSE/WebSocket nutzen.

## Auswertung

Die drei Heatmaps lesen den Tagesaggregatring, nicht den Raw-Ring. Beim initialen/automatischen Auswertungsrefresh werden Storageinfo und alle drei Matrizen über **einen kombinierten Endpunkt** geliefert; bei gültiger Systemzeit werden alle drei Heatmaps dabei in einem gemeinsamen Tagesring-Durchlauf aufgebaut. Nur gezielte Filterbuttons verwenden die kleineren Einzelendpunkte. Lange Aggregatbesuche bedienen alle 32 Records zusätzlich den generischen Hardwareinputpfad und `serviceUrgent()` und geben mit `delay(0)` an den Scheduler zurück. Damit kann ein physischer Tastendruck auch während einer größeren Statistikantwort zeitnah erfasst/feedbacket werden.

Die 53-KW-Matrix wird auch auf mittleren Breiten transponiert, damit nicht 53 winzige Spalten entstehen.

## CSV

CSV liest den Raw-Ring chronologisch und sendet 2-KiB-Chunks. Zwischen Chunks werden Hardware, Projektservice, Zeit, WLAN und OTA bedient und der Scheduler bekommt `delay(0)`. Es wird kein kompletter Export im RAM erzeugt.

Der eingebaute synchrone Arduino-`WebServer` kann während eines tatsächlichen TCP-Schreibaufrufs trotzdem keine harte Echtzeitgarantie liefern. Das ist eine dokumentierte Grenze des gewählten schlanken Serverstacks, kein Grund für einen zweiten parallelen Webserver.

## Fehlerentkopplung

- Audiofehler → Event bleibt gültig.
- Displayfehler → Event bleibt gültig.
- WLAN fehlt → Hardwaretaster, RTC/relative Zeit, Storage, OLED und Audio arbeiten weiter.
- Aggregatefehler → Raw-Event bleibt Source of Truth.
- LittleFS temporär nicht verfügbar → Event bleibt soweit möglich in der festen RAM-Queue und wird wiederholt.
- PendingQueue voll → der reale Tastendruck bleibt im RAM-Summary/Feedback sichtbar, wird aber als `droppedCount` ausdrücklich als **nicht dauerhaft gespeichert** markiert; der Datenstatus bleibt Fehler. Dauerhafte Speicherung wird nicht vorgetäuscht.

## Projekt-GPIO-Profil

Nur **DI1/GPIO13** ist für 0.2.0 aktiviert. DI2–DI4 und DO1–DO4 bleiben im generischen Basismodell definiert, sind aber deaktiviert. Dadurch verursachen sie weder Scanning noch Ausgangskonfiguration und ihre Pins stehen späteren Projektmodulen frei.

## Projektpräferenzen 0.2.0

`ProjectPreferences` ist die einzige persistente Quelle für Unterbrechungston und OLED-Projektanzeige. Die Home-UI schreibt jeweils genau ein Feld über `/api/interruptions/preferences`; es gibt keinen globalen Speichern-Button. `DisplayViews` liest nur diese Präferenzen und `InterruptionService` entscheidet beim Feedback zwischen festem Track und der ressourcenschonenden Rotation 2…N. Track 1 bleibt dem Bootpfad vorbehalten.

Die Heatmap-Filter verändern nur die jeweils betroffene Matrix. Das Frontend benachrichtigt nach einem Filterrequest gezielt die passende Bindung (`analytics.hourly` bzw. `analytics.monthWeek`), statt die ganze Auswertungsseite neu aufzubauen.
