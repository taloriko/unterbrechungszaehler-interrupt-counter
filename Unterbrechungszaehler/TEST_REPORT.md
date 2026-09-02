# Testbericht – Unterbrechungszähler 0.2.0

Auditdatum: **02.09.2026**  
Basis: **ESP32 UI Base FINAL 1.6.0**  
Ziel: **ESP32 Dev Module / klassischer ESP32-WROOM-32, Arduino-ESP32**

## Release-Status

**PASS für Quellcode-, Architektur-, Storage- und Frontend-Releasechecks.**

Der reale Arduino-ESP32-Link/Upload und die elektrische Hardwareprüfung müssen auf dem Zielboard erfolgen. In der Prüfumgebung ist kein installierter Arduino-ESP32-Core/`arduino-cli` vorhanden; deshalb werden **keine erfundene Firmwaregröße und keine erfundene reale App-Reserve** angegeben.

## 1. Automatischer Releasecheck

Ausgeführt:

```text
python3 tools/release_check.py
```

Ergebnis:

```text
PASS portable release checks
```

Geprüft werden unter anderem:

- Projektname und Version 0.2.0
- Raw-Ring-Kapazität 100.000
- 9-Byte-Raw-Record
- Tagesaggregat-Retention
- feste 64er Persistenzqueue
- DI1 Active-Edge-Interrupt-Latch
- Custom-LittleFS-Partition
- i18n-Parität DE/EN/SWG
- Gerätekarten-Reihenfolge
- genau ein permanenter Frontend-Intervalltimer
- keine unsicheren Bulk-DOM-HTML-Schreiboperationen
- keine externen HTML-Abhängigkeiten
- alle Projekt-API-Routen
- JavaScript-Syntax
- deterministisches Webbundle / ETag / gzip-Roundtrip

### Frontend-Messwerte

```text
Readable bundle: 183.133 Byte
Gzip / PROGMEM:   41.044 Byte
ETag:              8c41f19010df7d95
Übersetzungen:     305 Schlüssel je Sprache
```

Sprachen besitzen identische Key-Sets:

- Deutsch
- Englisch
- Schwäbisch

## 2. Strenger C++-Compilecheck

Alle **24** `.cpp`-/`.ino`-Übersetzungseinheiten wurden gegen die lokalen Arduino-/ESP32-API-Teststubs geprüft mit:

```text
-std=c++17
-Wall
-Wextra
-Werror
-Wpedantic
-Wshadow
-Wconversion
-Wsign-conversion
```

Ergebnis:

```text
PASS strict host C++ syntax for 24 translation units
```

Dieser Check findet C++-Typ-, Signatur-, Shadowing- und Warnungsprobleme, ersetzt aber ausdrücklich **nicht** den finalen Build gegen den echten installierten Arduino-ESP32-Core.

## 3. Storage-/100.000er-Simulation

Ausgeführt:

```text
python3 tools/test_interruption_storage.py
```

Ergebnis: **14/14 PASS**.

Getestet:

1. Partition budget
2. 9-Byte-Recordlayout und CRC
3. 100.000er Ring-Wrap
4. Tagesdelta und Aggregate
5. Europe/Berlin / DST-Kalenderverhalten
6. alle drei Heatmap-Sichten aus Tagesaggregaten
7. Queue-Überlauf: Live-Zähler bleibt wahr, Persistenzverlust wird sichtbar
8. transaktionaler Retry ohne Doppelereignis
9. rückwärts korrigierte Uhr → Delta wird `unknown`, nicht negativ erfunden
10. Kalenderanker überlebt nachfolgende relative Events
11. Raw-Sequenz wird nach Recovery gegen Aggregate-Checkpoint angehoben
12. dasselbe bei leerem Raw-Ring + vorhandenem Aggregate-Checkpoint
13. voller Raw-Ring: Metadatenfehler stellt verdrängten ältesten Record wieder her
14. voller Tagesring: Metadatenfehler rollt verdrängten Tagesrecord zurück

Der 100.000er Test endet mit:

```text
retained raw events=100,000
```

## 4. Flash-/Datenbudget

Custom-Partitionierung für 4 MiB Flash:

| Bereich | Größe |
|---|---:|
| NVS | 20 KiB |
| OTA Data | 8 KiB |
| App 0 | 1.441.792 B / 1,375 MiB |
| App 1 | 1.441.792 B / 1,375 MiB |
| LittleFS | 1.245.184 B / 1,1875 MiB |

Maximales projektiertes Datenbudget:

| Daten | Byte |
|---|---:|
| 100.000 Raw-Events × 9 | 900.000 |
| 2.300 Tagesaggregate × 64 | 147.200 |
| Raw-Metadaten 2 × 44 | 88 |
| Daily-Metadaten 2 × 40 | 80 |
| **Summe** | **1.047.368** |
| **Reserve vor LittleFS-Verwaltung** | **197.816** |

Das entspricht ca. **84,1 %** projektierten Nutzdaten und **15,9 %** Reserve innerhalb der LittleFS-Partition vor Dateisystemoverhead.

Wichtig: Der reale Arduino-Build muss noch beweisen, dass die komplette Firmware in den **1.441.792-Byte-OTA-Slot** passt.

## 5. RAM-/Datenmodell

Der ESP32 hält **nicht** 100.000 Events im RAM.

Statisch wichtige Projektobjekte:

```text
CapturedEvent (Host-ABI): 24 Byte
PendingQueue:             64 × 24 = 1.536 Byte
Summary (Host-ABI):       64 Byte
```

Zusätzlich besitzen die Analytics einen kleinen statischen Arbeitskontext. Der kombinierte Analytics-Endpunkt baut die drei Heatmaps in einem Tagesring-Durchlauf auf und hält nur die Ergebnismatrizen, nicht die Raw-History, im RAM.

CSV wird mit einem **2-KiB-Ausgabechunk** erzeugt; es gibt keinen 100.000-Zeilen-CSV-String im Speicher.

## 6. Ereignispfad / Reaktionspriorität

Der zeitkritische Pfad ist absichtlich kurz:

```text
DI1 FALLING Interrupt
  -> ISR setzt nur Flag
  -> GPIO-update außerhalb ISR + Debounce
  -> InterruptionService::capture()
  -> TimeService Snapshot (kein NTP-/RTC-I/O)
  -> RAM-Summary erhöhen
  -> OLED-/Audiofeedback anstoßen
  -> feste RAM-Persistenzqueue
  -> Flashschreiben / Aggregate nachgelagert
```

Die ISR enthält **keine**:

- Serial-Ausgabe
- Projektlogik
- Dateisystemoperation
- Netzwerkoperation
- Audio-/Displayoperation

Bei vollem Pending-RAM-Ring wird ein echter Tastendruck weiterhin im Live-Zähler und Feedback erfasst. Er wird aber nicht fälschlich als dauerhaft gespeichert markiert: `droppedCount` steigt und der Datenstatus geht auf Fehler.

## 7. Blocking-/Effizienzaudit

### Normaler Event-Capture

Im Capture-Pfad gibt es keine:

- NTP-Abfrage
- RTC-Leseoperation
- Heatmapberechnung
- Raw-History-Suche
- CSV-Erzeugung
- zeitgesteuerte `delay()`-Warteschleife

Audio-Kommandos werden gesendet und anschließend deadline-/`update()`-basiert verifiziert. Das Displayflash arbeitet ebenfalls über eine Deadline.

### Bewusste kurze Delays/Yields

Im Projektstand gefunden:

- `serial_log.cpp`: einmalig 50 ms bei Serial-Start
- `ota_module.cpp`: 20 ms unmittelbar vor geplantem OTA-Neustart
- Hauptloop: `delay(2)` als Scheduler-/WiFi-Yield
- Statistik/CSV: `delay(0)` als Scheduler/WDT-Yield

Keine dieser Stellen liegt als lange Warteoperation im eigentlichen Taster-Capturepfad.

### Bekannte synchrone Grenzen

Folgende Operationen können aufgrund der verwendeten Arduino-/Flash-/TCP-Schicht zeitweise synchron sein:

- `WebServer::handleClient()` / TCP-Ausgabe
- DNS (`WiFi.hostByName`) während einer bewusst ausgelösten NTP-Prüfung
- LittleFS-Flashschreiben/-flush
- OTA-Schreibvorgänge
- I2C-Bustransaktionen mit begrenztem Wire-Timeout

Gegenmaßnahmen im Projekt:

- physischer DI1 besitzt einen minimalen Interrupt-Latch
- Hauptloop bedient Hardware/InterruptionService vor dem WebServer
- CSV wird gechunkt und bedient zwischen Chunks kritische lokale Services
- Recovery/Rebuild läuft kooperativ in kleinen Batches
- Statistiken nutzen Tagesaggregate statt Raw-Vollscan

Der DI1-Latch ist für **menschliche Tastendrücke**, nicht für einen Hochfrequenz-Pulszähler. Mehrere vollständige extrem schnelle Drück-/Loslasszyklen während einer einzigen langen synchronen Operation können auf ein Flag zusammenfallen.

## 8. Statistik-Audit

Normaler Statistikstart unter **Auswertung**:

- ein kombinierter HTTP-Request
- Storageinfo + alle drei Heatmaps
- bei gültiger Systemzeit ein Durchlauf durch maximal 2.300 Tagesrecords
- kein Scan durch 100.000 Raw-Events

Manuelle Filteränderungen laden nur die betroffene Heatmap.

Heatmaps:

- 7 × 24 Wochentag/Stunde
- 12 × 53 Monat/Kalenderwoche
- 5 × 12 Jahr/Monat

Zellwerte bleiben numerisch sichtbar; Farbe ist Zusatzinformation. Breite Matrizen werden responsiv transponiert.

## 9. CSV-Audit

CSV wird chronologisch aus dem Raw-Ring dekodiert und gestreamt.

Eigenschaften:

- kein permanentes CSV-Duplikat
- keine 100.000 Events im Browser/RAM
- 2-KiB-Chunks
- absolute Events enthalten UTC + lokale Europe/Berlin-Zeit
- relative Events erhalten keine erfundene 1970-Zeit
- TimeSource und EventSource bleiben pro Datensatz erhalten

## 10. Persistenz-/Recovery-Audit

Raw- und Daily-Ring verwenden getrennte alternierende Metadatenkopien mit CRC.

Geprüfte Fehlerfälle:

- Strom-/Schreibabbruch nach Recordwrite vor Meta-Commit
- Retry ohne Duplikat
- voller Ring + fehlgeschlagener Meta-Commit
- beschädigte Raw-Metadaten
- beschädigte Daily-Metadaten
- Sequenzkontinuität zwischen Raw- und Aggregate-Store

Ein normaler Boot führt keinen 100.000er Vollscan aus. Vollscans gibt es nur als Recovery und dann kooperativ in kleinen Batches.

## 11. Frontend-Audit

Geprüft:

- kein React/Vue/Angular/Bootstrap/jQuery
- keine CDN-Abhängigkeit
- kein Webfont
- kein Service Worker
- kein unsicheres `innerHTML` für Gerätedaten
- genau ein permanenter 1-s-Frontendtimer
- Livepoll nur bei sichtbarem Home oder sichtbarer Auswertung
- Gerät/Einstellungen erzeugen kein Projekt-Livepolling
- Webbutton übernimmt direkte Serverantwort
- Statistik wird erst bei Bedarf geladen

## 12. Hardware-/Realtest – noch auf Zielgerät auszuführen

Diese Punkte können Hosttests nicht ersetzen:

- Arduino-IDE-Compile mit installiertem ESP32-Core
- reale Sketchgröße < 1.441.792 Byte
- LittleFS mountet mit der Custom-Partition
- OTA erhält NVS + LittleFS
- DI1/GPIO13: ein Tastendruck = genau ein Event
- Debounce bei absichtlichem Tasterprellen
- kurzer Tastendruck während CSV-Download
- OLED: Zahl / letzter Abstand / WiFi / Zeitstatus / Flashfeedback
- Audio: Boot-Track 1 getrennt; Unterbrechung fester Track >=2 oder Rotation 2…N; Soundtoggle AUS/EIN
- Offlinebetrieb ohne WLAN
- RTC-Fallback
- NTP-basierter Event mit Quelle `ntp`
- RTC-basierter Event mit Quelle `rtc`
- CSV-Download mit echten Events
- Heatmapdarstellung Desktop + ca. 320 px
- Langzeittest mit wiederholten Schreibvorgängen und Reboots

## Freigabe

**Quellcode/Architektur/Hosttests: FREIGEGEBEN für den ersten Hardware-Integrationsstand 0.2.0.**

Der nächste Gate ist der reale Arduino-Build auf dem ESP32 Dev Module. Erst dessen Compiler-/Linker-Ausgabe darf als reale Firmwaregröße und reale OTA-App-Reserve dokumentiert werden.


## Ergänzungsprüfung 0.2.0

Automatisiert geprüft:

- 305 identische i18n-Schlüssel je Sprache
- Heatmap-KW-Header bestehen nur aus Zahlen
- gezielter Rerender nach manueller Stunden-/KW-Filterabfrage
- Home-Projekteinstellungskarte vorhanden
- Soundmodus `fixed` / `rotate` persistent vorhanden
- Track 1 wird im Rotationspfad nicht verwendet
- Display-Flash, Anzeigeart, Helligkeit und Dimmparameter laufen über die bestehende `ProjectPreferences`-Schicht
- Webbundle/ETag/Gzip-Roundtrip konsistent
- 14/14 Storage-/Recovery-/Heatmap-Hosttests weiterhin PASS

Zusätzlich wurden die in 0.2.0 geänderten C++-Module `project_preferences.cpp` und `interruption_service.cpp` mit `g++ -std=c++17 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wsign-conversion` gegen minimale Arduino-/Preferences-API-Stubs syntaxgeprüft: **PASS**. Der reale Arduino-ESP32-Build auf Zielhardware bleibt weiterhin der verbindliche Integrationsbuild.
