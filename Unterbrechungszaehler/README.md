# Unterbrechungszähler 0.2.0

Projekt auf Basis von **ESP32 UI Base FINAL 1.6.0** für ein klassisches **ESP32 Dev Module / ESP32-WROOM-32** mit Arduino-ESP32.

Der eingefrorene UI-/Hardware-Unterbau bleibt Infrastruktur. Die eigentliche Bedeutung „Unterbrechung“ liegt ausschließlich in der neuen Projektschicht (`InterruptionService`, Storage, Aggregation, DisplayViews und Projekt-API).

## Funktion

Eine Unterbrechung wird ausgelöst durch:

- den physischen Taster an **DI1 / GPIO13** (Taster gegen GND), oder
- den großen **Unterbrechung**-Button auf Home.

Beide Wege laufen durch denselben `InterruptionService`. Ein Event wird sofort im RAM-Summary sichtbar und danach in eine feste 64er Persistenzqueue übernommen. Display- und Audiofeedback werden danach angestoßen; Flash-Persistenz und Tagesaggregate sind nachgelagert. NTP, RTC oder Statistiken werden **nicht** im Tastendruckpfad abgefragt.

Home zeigt:

- große Zahl **Unterbrechungen heute**,
- **Letzte Unterbrechung vor ...**,
- großen Unterbrechungsbutton,
- eigene Home-Kachel **Feedback & Display** mit Ton, Display-Flash, Displaymodus, Helligkeit und Dimmer.

Die Feedback-/Displayeinstellungen liegen in ESP32-NVS und gelten deshalb auch für den Hardwaretaster ohne Browser. **Track 1 ist ausschließlich der Boot-Ton.** Für Unterbrechungen kann ein fester Track ab **2** gewählt oder der Modus **Wechselnd** aktiviert werden. Wechselnd läuft deterministisch durch die vom DY-SV17F erkannten Tracks 2…N und wiederholt bei mehreren verfügbaren Unterbrechungstönen nicht unmittelbar denselben Track. Ist die Trackanzahl nicht verfügbar, dient der konfigurierte feste Track als Fallback.

Die Persistenzqueue fasst **64 Events** (ca. 1,5 KiB statischer RAM). Ist sie bei einem längeren Speicherfehler trotzdem voll, wird der Tastendruck weiterhin sofort gezählt und optisch/akustisch bestätigt. Der nicht dauerhaft speicherbare Event wird jedoch ausdrücklich als Verlust dieses Bootlaufs ausgewiesen (`droppedCount`) und der Datenstatus bleibt auf Fehler – dauerhafte Speicherung wird nicht vorgetäuscht.

## Ereignisdaten

Ein Rohereignis enthält logisch:

- Zeitpunkt oder relative Uptime,
- Zeitquelle (`ntp`, `rtc`, `browser`, `relative`),
- Eventquelle (`physical_button`, `web_button`, ...),
- Zeitgültigkeit,
- Abstand zum vorherigen Event **desselben lokalen Tages**.

Der lokale Kalender wird zentral mit `Europe/Berlin` / `CET-1CEST,M3.5.0,M10.5.0/3` gebildet. Absolute Zeit bleibt UTC. Bei fehlender absoluter Zeit wird kein erfundener Kalendertag erzeugt; solche Events werden als nicht zuordenbar gezählt.

## Persistenz und 100.000er Ringspeicher

Rohereignisse werden **nicht als CSV** gespeichert. Der primäre Datenspeicher ist ein kompakter binärer Ring in LittleFS:

- 100.000 Slots
- 9 Byte pro Raw-Record
- 900.000 Byte maximale Raw-Datei
- CRC8 je Record
- zwei alternierende Metadatenslots mit CRC32
- ältester Raw-Event wird erst nach Erreichen der Kapazität überschrieben

Zusätzlich existiert ein unabhängiger Tagesaggregatring:

- 2.300 Tage (> 6 Jahre)
- 64 Byte pro Tag
- Tagesgesamtwert + 24 Stundenwerte
- CRC16 je Tagesrecord
- zwei Metadatenslots mit CRC32

Die Heatmaps lesen ausschließlich diese Tagesaggregate. Normalerweise werden **niemals 100.000 Events** für eine Statistikansicht durchsucht.

Der Raw-Ring ist die Source of Truth. Falls Aggregatmetadaten beschädigt sind, können die Aggregate kooperativ aus den noch vorhandenen Raw-Events neu aufgebaut werden. Ein normaler Boot scannt den 100.000er Ring nicht. Beide Ringformate besitzen transaktionale Metadaten-Commits und Recoverypfade für abgebrochene Schreibvorgänge.

Details: [`STORAGE_FORMAT.md`](STORAGE_FORMAT.md)

## Flash-Partitionierung

Das Projekt enthält eine eigene `partitions.csv` für 4 MiB Flash:

| Partition | Größe |
|---|---:|
| NVS | 20 KiB |
| OTA Data | 8 KiB |
| App Slot 0 | 1.375 MiB |
| App Slot 1 | 1.375 MiB |
| LittleFS | 1.1875 MiB |

Die Partition heißt `littlefs`, verwendet in der ESP32-Partitionstabelle aber absichtlich den Daten-Subtype `spiffs`, den Arduino-LittleFS ebenfalls nutzt. Sie beginnt bei `0x2D0000` und endet exakt bei `0x400000`.

Für Projektdaten stehen 1.245.184 Byte zur Verfügung. Raw-Ring + Tagesring + beide Metadatenpaare benötigen 1.047.368 Byte; vor LittleFS-Verwaltung bleiben 197.816 Byte Reserve.

**Wichtig:** Ob die komplette Firmware in den 1.375-MiB-OTA-Slot passt, entscheidet der reale Arduino-Build. In dieser Entwicklungsumgebung war kein installierter Arduino-ESP32-Core verfügbar. Nach dem ersten realen Build die von Arduino gemeldete Sketchgröße prüfen. Die bestehende OTA-Karte zeigt zusätzlich die App-Reserve.

## Verhalten bei Flash/OTA

- Normales OTA schreibt nur die inaktive App-Partition: LittleFS und NVS bleiben erhalten.
- Ein normaler Sketch-Upload lässt die Datenpartition üblicherweise bestehen, sofern kein vollständiges Flash-Erase durchgeführt wird.
- **Erase All Flash** löscht die Projektdaten.
- Eine spätere Änderung der Partitionstabelle kann die Datenpartition verschieben/zerstören und muss deshalb als Datenmigration behandelt werden.

LittleFS wird nicht blind bei jedem Mountfehler formatiert. Ein einmal initialisiertes Projekt unterdrückt automatisches Formatieren, damit ein vorübergehender Mountfehler nicht zu Datenverlust führt.

## CSV-Export

Unter **Auswertung → Daten & Export** kann der Raw-Ring als CSV heruntergeladen werden. CSV wird erst beim HTTP-Download chronologisch erzeugt und nicht als zweite Datei gespeichert.

Spalten:

```text
sequence
 timestamp_utc
 timestamp_local
 date_local
 time_local
 weekday
 iso_week
 time_source
 event_source
 delta_previous_same_day_seconds
 relative_seconds
 time_valid
```

Der Export wird in 2-KiB-Chunks ausgegeben. Zwischen den Chunks werden GPIO-/Audio-, Projekt-, Zeit- und WLAN-Updatepfade weiter bedient. Der ESP32 hält deshalb keinen 100.000-Zeilen-CSV-String im RAM.

## Auswertung

Neuer Tab **Auswertung** mit drei Heatmaps:

1. **Wochentage / Stunden**
   - Filter Kalenderwoche oder Von/Bis
   - 7 × 24 Werte
   - aktueller Wochentag / aktuelle Stunde als Fadenkreuz
2. **Monate / Kalenderwochen**
   - Jahr wählbar
   - Monat × ISO-KW
   - aktueller Monat / aktuelle KW als Fadenkreuz, wenn das aktuelle Jahr gewählt ist
3. **Letzte 5 Jahre / Monate**
   - fünf dynamische Kalenderjahre × 12 Monate
   - aktuelles Jahr / aktueller Monat als Fadenkreuz

Auf schmalen Geräten werden große Matrizen transponiert, damit kein horizontaler Seitenscroll nötig ist. Zahlen bleiben in jeder Zelle sichtbar; Farbe ist nur zusätzliche Intensitätsinformation.

Statistikdaten werden erst beim Öffnen von **Auswertung** geladen. Der initiale bzw. automatische Refresh nutzt einen kombinierten API-Endpunkt: Storageinfo und alle drei Heatmaps kommen in **einem Request**, und der Tagesring wird bei gültiger Systemzeit nur einmal durchlaufen. Die manuellen Filterbuttons laden weiterhin nur die jeweils betroffene Heatmap und lösen jetzt einen gezielten DOM-Refresh der Matrix aus; geänderte Jahr/KW/Von-Bis-Werte werden dadurch unmittelbar sichtbar. Kalenderwochenspalten zeigen nur die Zahl – die Einheit ergibt sich aus der Kartenüberschrift. Ein neuer Event markiert Statistikdaten zunächst nur als dirty; Home/Display/Sound haben Vorrang.

## Live-Home

Ein Hardwaretaster soll auch in einem bereits geöffneten Browser sichtbar werden. Der bestehende synchrone ESP32-`WebServer` wird dafür nicht mit einer dauerhaften SSE-Verbindung belastet. Stattdessen nutzt Home einen sehr kleinen konditionalen Live-Endpunkt:

```text
GET /api/interruptions/live?since=<sequence>
```

Nur wenn **Home oder Auswertung aktiv** und das Browserdokument sichtbar ist, prüft der einzige vorhandene 1-s-UI-Timer diesen Endpunkt. Home braucht ihn für physische Tasterereignisse; Auswertung nutzt dieselbe kleine Revision nur, um sichtbare Statistiken nach einem Hardwareereignis verzögert als dirty zu markieren. Ohne Änderung antwortet der ESP32 mit HTTP 204. Gerät/Einstellungen und versteckte Browser-Tabs erzeugen keinen Live-Traffic. Ein Klick auf den Webbutton wartet nicht auf dieses Polling, sondern übernimmt die direkte Serverantwort.

## Display

Die Projektschicht nutzt den vorhandenen SH1106-Treiber über `DisplayViews`.

Standardansicht:

- große Unterbrechungszahl heute
- „HEUTE“
- Alter der letzten Unterbrechung
- Zeit-OK-Icon
- WLAN-Icon

Das kurze Invertieren bei einer Unterbrechung ist separat ein-/ausschaltbar. `DisplayViews` bietet drei Vorlagen: **Standard** (Heute + letzte Unterbrechung + Zeit/WLAN), **Nur Zahl** (heutiger Zähler maximal groß) und **Nur letzte Unterbrechung** (Zeitabstand maximal groß). Helligkeit, Dimmer-Timeout und gedimmte Helligkeit werden ebenfalls im Gerät gespeichert. Der OLED-Treiber kennt weiterhin keine Projektbedeutung; `DisplayViews` bildet die Projektvorlagen und arbeitet nichtblockierend.

## Pinbelegung des Projekts

### Unterbrechungstaster

```text
ESP32 GPIO13 / DI1 ---- Taster ---- GND
```

DI1 ist `INPUT_PULLUP`, active-low und wird im generischen GPIO-Modul entprellt. Zusätzlich besitzt genau dieser wichtige Eingang einen minimalen FALLING-Edge-Interrupt-Latch: Die ISR setzt ausschließlich ein Flag; Entprellung, Callback und Projektlogik laufen weiter im normalen Loop. Dadurch bleibt ein normaler kurzer Tastendruck auch während einer längeren synchronen HTTP-Ausgabe erhalten. Nur die Drückflanke zählt; Loslassen erzeugt kein Unterbrechungsereignis.

Alle übrigen Basis-Pins stehen in [`HARDWARE_WIRING.md`](HARDWARE_WIRING.md).

## Wichtige Dateien

```text
Unterbrechungszaehler.ino
project_config.h
project_time.*
interruption_types.h
interruption_service.*
interruption_store.*
interruption_aggregates.*
interruption_api.*
project_preferences.*
display_views.*
partitions.csv
ui-src/app.js
ui-src/app.css
```

Architektur: [`PROJECT_ARCHITECTURE.md`](PROJECT_ARCHITECTURE.md)
Zeitbasis: [`TIME_ARCHITECTURE.md`](TIME_ARCHITECTURE.md)
Tests: [`TEST_REPORT.md`](TEST_REPORT.md)

## Arduino-Inbetriebnahme

1. Arduino IDE mit aktuellem **esp32 by Espressif Systems** verwenden.
2. Board: **ESP32 Dev Module**.
3. Projektordner als Sketchordner verwenden; `partitions.csv` liegt direkt im Sketchordner.
4. In `config.h` WLAN-SSID und WLAN-Passwort setzen.
5. Hardware gemäß `HARDWARE_WIRING.md` anschließen.
6. Kompilieren und prüfen, dass das Binary in den 1.375-MiB-App-Slot passt.
7. Hochladen und Serial Monitor mit 115200 Baud öffnen.
8. Test: Hardwaretaster einmal drücken → Zähler/Display/Ton sofort, anschließend Persistenzmeldung im SerialLog.

Änderungen an HTML/CSS/JS anschließend mit

```bash
python3 tools/build_web.py
```

in `web_assets.h` bündeln.

## Was absichtlich noch nicht enthalten ist

- Löschfunktion für Projektdaten
- Cloud-Sync
- Datenbank
- Login/Benutzerverwaltung
- externe Chartbibliothek
- automatische NTP-Polling-Schleife
- komplette Raw-History im Browser

Das Projekt bleibt lokal, offline-fähig und auf den eigentlichen Unterbrechungszähler fokussiert.
