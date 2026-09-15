# Unterbrechungszähler 3.6.0 – technische Übersicht

Dies ist der Sketchordner der Version **3.6.0** für ein klassisches ESP32 Dev Module / ESP32-WROOM-32.

## Ereigniserfassung

Der physische DI1/GPIO13 wird ab 3.6.0 zuerst durch `WorkCycle` klassifiziert. Dadurch sind Arbeitsbeginn und Arbeitsende keine Unterbrechungen:

1. der erste kurze Tastendruck eines lokalen Tages startet den Arbeitszyklus,
2. während des aktiven Zyklus bleibt der jeweils letzte kurze Druck zunächst nur als persistenter Kandidat erhalten,
3. erst ein weiterer gültiger kurzer Druck bestätigt den vorherigen Kandidaten als echte Unterbrechung,
4. ein langer Druck ab 2 Sekunden beendet den Zyklus ausdrücklich,
5. ohne langen Druck wird beim lokalen Tageswechsel der letzte Kandidat automatisch als Zyklusende verwendet und nicht als Unterbrechung gezählt.

Beim manuellen Ende wird ein eventuell vorheriger Kandidat noch als Unterbrechung bestätigt, weil der spätere lange Druck eindeutig das tatsächliche Ende markiert. Danach zeigt das OLED für 10 Sekunden **FEIERABEND** und die heutige Unterbrechungszahl.

Der Webbutton bleibt bewusst unabhängig vom lokalen Arbeitszyklus und erzeugt weiterhin direkt eine Unterbrechung.

## Anti-Spam

Für kurze physische Tastendrücke bleibt die feste 10-Sekunden-Sperre erhalten. Der Startdruck eröffnet die Sperre ebenfalls. Weitere kurze Drücke innerhalb dieser Zeit werden verworfen und erreichen weder Rohdaten noch Statistiken. Ein verworfener Druck verlängert die Sperre nicht.

Der lange 2-Sekunden-Druck zum expliziten Arbeitsende ist davon ausgenommen, damit Feierabend jederzeit zuverlässig ausgelöst werden kann.

## Speicherung

- 100.000 bestätigte Raw-Unterbrechungen, weiterhin exakt 9 Byte je Record
- 2.300 Tagesaggregate, 64 Byte je Record
- feste 64er Pending-Queue
- CRC-geschützte Raw-Records und transaktionale Metadaten
- aktueller Zykluszustand und letzter Kandidat kompakt in NVS
- separate kleine Zyklusmarken in `/cycles.log`; sie verändern das bestehende Raw-Format nicht
- CSV wird beim Download gestreamt und enthält weiterhin ausschließlich echte Unterbrechungen

Damit bleiben bestehende 3.x-Rohdaten und sämtliche bisherigen Unterbrechungs-Auswertungen kompatibel. START und ENDE verfälschen keine Heatmap, keinen Tageszähler und keinen Durchschnitt.

Details: [`STORAGE_FORMAT.md`](STORAGE_FORMAT.md)

## Auswertung

- Wochentag × Stunde
- Monat × ISO-Kalenderwoche
- letzte fünf Kalenderjahre × Monat
- Fokus & Ruhe
- Arbeitsmuster

Alle bisherigen Auswertungen arbeiten weiterhin mit bestätigten Unterbrechungen. Der jeweils letzte physische Kurzdruck eines nicht manuell beendeten Tages gelangt deshalb gar nicht erst in den Unterbrechungs-Ring.

Die Metrik **Ø Abstand** wird auf Anforderung aus den vorhandenen Rohereignissen berechnet. Ein gültiger Abstand wird der Start-Unterbrechung zugeordnet; über Mitternacht wird nie ein Intervall gebildet.

## Hardware

- DI1: GPIO13 gegen GND
- I2C: GPIO21/22 für DS3231 + SH1106
- DY-SV17F: RX GPIO18, TX GPIO19, BUSY GPIO39
- BUSY benötigt externen ca. 10-kΩ-Pull-up an DY-SV17F V33

Für die neue Zykluslogik ist **keine zusätzliche Hardware und kein weiterer GPIO** nötig. Kurzer und langer Druck werden am bestehenden DI1 unterschieden.

Details: [`HARDWARE_WIRING.md`](HARDWARE_WIRING.md)

## Sound / Display

Track 1 ist Boot/Test, Track 2 Anti-Spam, Track 3 und höher sind normale Unterbrechungstöne. Sound, Display-Master, Displayflash, Anzeigeart, Helligkeit und Dimmer werden persistent gespeichert.

Beim expliziten Arbeitsende übernimmt `WorkCycle` das SH1106 für 10 Sekunden mit der Abschlussanzeige **FEIERABEND** plus heutiger Unterbrechungszahl und gibt es danach an die normale Displayansicht zurück. Der automatische Tagesabschluss erzeugt bewusst keine zusätzliche Displayaktivität.

## Zeit

Absolute Zeit bleibt UTC; lokale Auswertung und Zyklusgrenze nutzen `Europe/Berlin`. NTP hat Priorität, RTC und Browser dienen als Fallback. Ohne gültige absolute Zeit kann keine sichere Tagesgrenze bestimmt werden; in diesem Ausnahmefall fällt die physische Erfassung auf die bewährte direkte Unterbrechungserfassung zurück, statt Start/Ende zu raten.

Details: [`TIME_ARCHITECTURE.md`](TIME_ARCHITECTURE.md)

## Build

Nach UI-Änderungen:

```bash
python3 tools/build_web.py
```

Releasecheck:

```bash
python3 tools/release_check.py
```

Arduino: `Unterbrechungszaehler.ino` öffnen, **ESP32 Dev Module** wählen und kompilieren. Die enthaltene `partitions.csv` definiert zwei OTA-App-Slots und LittleFS.

## Weitere Dokumente

- [`PROJECT_ARCHITECTURE.md`](PROJECT_ARCHITECTURE.md)
- [`STORAGE_FORMAT.md`](STORAGE_FORMAT.md)
- [`TEST_REPORT.md`](TEST_REPORT.md)
- [`../README.md`](../README.md)
