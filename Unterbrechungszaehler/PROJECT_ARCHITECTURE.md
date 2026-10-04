# Projektarchitektur – Unterbrechungszähler 3.6.1

Basis: **ESP32 UI Base FINAL 1.6.0**. Die Basis bleibt Infrastruktur; die Bedeutung „Arbeitszyklus“ und „Unterbrechung“ beginnt ausschließlich in der Projektschicht.

## Schichten

```text
GPIO DI1
   │
   ▼
WorkCycle
   │
   ├── erster Kurzdruck ──────> START / Zykluszustand + cycles.log
   ├── langer Druck >= 2 s ──> END / Zykluszustand + cycles.log
   │
   └── gültiger Kurzdruck im aktiven Zyklus
                         │
                         ▼
Webbutton ───────────────┤
spätere Quellen ─────────┤
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

`WorkCycle` ist bewusst eine dünne Klassifikationsschicht vor dem vorhandenen physischen Eingabepfad. Es entscheidet nur **START / echte Unterbrechung / END**. Eine echte Unterbrechung wird anschließend unverändert über `InterruptionService::capture(PhysicalButton)` verarbeitet.

`GpioModule`, `TimeService`, `DisplaySh1106`, `AudioDySv17f` und `WebServer` kennen den Begriff „Arbeitszyklus“ nicht. Speicherung, Zähler und Auswertung echter Unterbrechungen bleiben im bestehenden `InterruptionService`-/Store-Pfad.

## Physischer Tasterpfad 3.6.1

DI1/GPIO13 liefert weiterhin entprellte Zustandsänderungen aus `GpioModule`. `WorkCycle` nutzt Drücken und Loslassen, um Kurz- und Langdruck zu unterscheiden:

```text
inaktiv + kurzer Druck
  -> START
  -> kein InterruptionService::capture()
  -> kein Ton
  -> keine 10-s-Sperre

aktiv + kurzer Druck
  -> WorkCycle-eigene 10-s-Anti-Spam-Prüfung
  -> akzeptiert: sofort InterruptionService::capture(PhysicalButton)
  -> verworfen: Track 2 + OLED-Anti-Spam

aktiv + Druck >= 2 s
  -> END
  -> kein InterruptionService::capture()
  -> 10 s FEIERABEND
```

Der in 3.6.0 erprobte verzögerte Pending-Kandidat existiert nicht mehr. Es gibt keine spätere Nachbuchung und keine rückwirkende Umklassifizierung einer bereits gespeicherten Unterbrechung.

Wird END vergessen, beendet `WorkCycle` den Zyklus beim nächsten erkannten lokalen Tageswechsel. Das verändert keine bereits gespeicherten Raw-Events.

## DI1 Edge-Latch und Entprellung

DI1 besitzt weiterhin den **minimalen aktiven Edge-Latch per Interrupt**. Die ISR setzt nur ein `volatile bool`; sie führt weder SerialLog, Netzwerk, Dateisystem noch Projektcallbacks aus. Im nächsten `GpioModule::update()` wird das Flag atomar übernommen und die normale Entprell-/Callbacklogik ausgeführt.

Damit überlebt ein menschlicher kurzer Tastendruck auch einen vorübergehend blockierenden synchronen TCP-/CSV-Schreibabschnitt. Zwei vollständige extrem schnelle Drückzyklen, die beide komplett in demselben blockierenden Abschnitt liegen, können absichtlich zu einem Latch zusammenfallen – dies ist ein robuster Human-Button-Latch, kein Hochfrequenz-Pulszähler.

## Zeitkritischer Unterbrechungs-Capture-Pfad

Nach der Klassifikation verwendet 3.6.1 wieder den bewährten `InterruptionService::capture()`-Pfad. Er erledigt nur kleine, begrenzte Operationen:

1. atomaren Snapshot aus dem bereits laufenden `TimeService`
2. lokale Kalenderableitung über `ProjectTime`, falls absolute Zeit gültig ist
3. Abstand zum letzten gültigen Event desselben lokalen Tages bestimmen
4. Event in eine feste 64er PendingQueue kopieren
5. RAM-Summary / Revisionsnummer aktualisieren
6. Display-/Audiofeedback vormerken
7. beim physischen Taster sofort `serviceUrgent()` ausführen

**Nicht** im Capture-Pfad: LittleFS-Persistenz, NTP, RTC-I2C, Heatmapscan, CSV oder Statistik-Neuberechnung.

Der Webbutton bleibt unabhängig vom Arbeitszyklus und ruft denselben Unterbrechungsservice direkt auf.

## Sound- und Anti-Spam-Verantwortung

Die 10-Sekunden-Sperre für den physischen Knopf bleibt erhalten, wird für den Arbeitszyklus aber erst bei einer **echten akzeptierten Unterbrechung** gestartet. START setzt die Sperre nicht.

- Track 1: Boot/Test
- Track 2: ausschließlich Anti-Spam-Feedback
- Track 3 und höher: normale Unterbrechungstöne

Ein langer END-Druck wird unabhängig vom Kurzdruck-Cooldown ausgewertet.

## Feierabend-Overlay

Nach manuellem END besitzt `WorkCycle` den lokalen Interaktions-/Displaypfad für 10 Sekunden exklusiv:

- weitere physische DI1-Eingaben werden ignoriert,
- die normale InterruptionService-Displaybedienung wird im Hauptloop in diesem kurzen Fenster nicht ausgeführt,
- `WorkCycle` zeichnet `FEIERABEND` plus heutige Unterbrechungszahl,
- danach wird einmal ein Home-Refresh angefordert.

WLAN, Webserver, Zeit und OTA laufen währenddessen weiter. Der automatische Tagesabschluss erzeugt kein Overlay.

## Persistenzpipeline echter Unterbrechungen

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

START und END laufen **nicht** durch diese Pipeline. Sie verwenden nur den kleinen NVS-Zykluszustand plus das separate `/cycles.log`.

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

Der einzige wiederkehrende Frontendtimer bleibt der 1-s-UI-Tick. Nur wenn **Home oder Auswertung aktiv und das Dokument sichtbar** ist, wird der kleine Live-Endpunkt abgefragt. Home aktualisiert damit physische Tasterereignisse; die sichtbare Auswertung erkennt über dieselbe Revision neue Hardwareevents und lädt Heatmaps erst nach einem kurzen Deferred-Refresh neu. Unverändert → HTTP 204.

## Auswertung

Die drei Heatmaps besitzen zwei Metriken. **Anzahl** liest weiterhin den Tagesaggregatring. **Ø Abstand** wird nur auf Anforderung aus den retained Rohereignissen aufgebaut, weil dort absolute Zeit und `deltaSeconds` vorhanden sind.

Dabei werden ausschließlich unmittelbar aufeinanderfolgende retained Events mit gültiger absoluter Zeit, demselben lokalen Kalendertag und plausibler positiver Differenz verwendet. Der Messwert wird der Start-Unterbrechung zugeordnet; dadurch hat der letzte **Unterbrechungsdatensatz** jedes Tages automatisch kein Sample. START/END des Arbeitszyklus sind nicht Teil dieser Rohdaten.

## CSV

CSV liest den Raw-Ring chronologisch und sendet 2-KiB-Chunks. Zwischen Chunks werden Hardware, Projektservice, Zeit, WLAN und OTA bedient und der Scheduler bekommt `delay(0)`. Es wird kein kompletter Export im RAM erzeugt.

## Fehlerentkopplung

- Audiofehler → Event bleibt gültig.
- Displayfehler → Event bleibt gültig.
- WLAN fehlt → Hardwaretaster, RTC/relative Zeit, Storage, OLED und Audio arbeiten weiter.
- Aggregatefehler → Raw-Event bleibt Source of Truth.
- LittleFS temporär nicht verfügbar → Event bleibt soweit möglich in der festen RAM-Queue und wird wiederholt.
- PendingQueue voll → der reale Tastendruck bleibt im RAM-Summary/Feedback sichtbar, wird aber als `droppedCount` ausdrücklich als **nicht dauerhaft gespeichert** markiert.
- ungültige lokale Zeit → Arbeitszyklusgrenzen werden nicht geraten; der physische Druck fällt auf die normale Unterbrechungserfassung zurück.

## Projekt-GPIO-Profil

Nur **DI1/GPIO13** ist im Projektprofil aktiviert. DI2–DI4 und DO1–DO4 bleiben im generischen Basismodell definiert, sind aber deaktiviert. Dadurch verursachen sie weder Scanning noch Ausgangskonfiguration und ihre Pins stehen späteren Projektmodulen frei.

## Projektpräferenzen

`ProjectPreferences` ist die persistente Quelle für Unterbrechungston und OLED-Projektanzeige einschließlich Display-Master-Schalter. Die UI schreibt jeweils gezielt über `/api/interruptions/preferences`; es gibt keinen globalen Speichern-Button.

`DisplayViews` liest diese Präferenzen und `InterruptionService` entscheidet beim normalen Feedback zwischen festem Track und ressourcenschonender Rotation ab Track 3. Track 1 bleibt Boot/Test, Track 2 Anti-Spam.
