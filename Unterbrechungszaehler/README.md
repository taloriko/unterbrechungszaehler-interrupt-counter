# Unterbrechungszähler 3.6.1 – technische Übersicht

Dies ist der Sketchordner der Version **3.6.1** für ein klassisches ESP32 Dev Module / ESP32-WROOM-32.

## Ereigniserfassung

Der physische DI1/GPIO13 wird ab 3.6.x zuerst durch `WorkCycle` klassifiziert. In 3.6.1 ist der Einbau bewusst wieder einfach und direkt gehalten:

1. der erste kurze Tastendruck bei inaktivem Zyklus startet den Arbeitstag und zählt nicht als Unterbrechung,
2. während des aktiven Zyklus wird jeder gültige kurze Druck **sofort** als Unterbrechung über den bestehenden `InterruptionService` erfasst,
3. ein langer Druck ab 2 Sekunden beendet den Zyklus ausdrücklich und zählt nicht als Unterbrechung,
4. wenn der lange Enddruck vergessen wird, schließt der Zyklus beim lokalen Tageswechsel automatisch,
5. ein bereits als Unterbrechung erfasster kurzer Druck wird später nicht mehr rückwirkend umklassifiziert.

Damit bleiben Tastergefühl, normaler Unterbrechungston, Tageszähler und Speicherung synchron. Es gibt keinen verzögerten Kandidaten mehr.

Nach einem manuellen Ende zeigt das OLED für 10 Sekunden **FEIERABEND** und die heutige Unterbrechungszahl. Während dieser Anzeige werden weitere physische Eingaben ignoriert und die normale Unterbrechungsanzeige nicht dazwischen gezeichnet.

Der Webbutton bleibt bewusst unabhängig vom lokalen Arbeitszyklus und erzeugt weiterhin direkt eine Unterbrechung.

## Anti-Spam

Für echte kurze physische Unterbrechungsdrücke bleibt die feste 10-Sekunden-Sperre erhalten. **Der Startdruck verbraucht diese Sperre nicht.** Dadurch ist auch eine echte Unterbrechung kurz nach Arbeitsbeginn zulässig.

Erst ein akzeptierter Unterbrechungsdruck startet die 10-Sekunden-Sperre. Weitere kurze Drücke innerhalb dieser Zeit werden verworfen und erreichen weder Rohdaten noch Statistiken. Ein verworfener Druck verlängert die Sperre nicht und erhält wie bisher Track 2 plus OLED-Anti-Spam-Feedback.

Der lange 2-Sekunden-Druck zum expliziten Arbeitsende ist von der Sperre ausgenommen, damit Feierabend jederzeit zuverlässig ausgelöst werden kann.

## Speicherung

- 100.000 Raw-Unterbrechungen, weiterhin exakt 9 Byte je Record
- 2.300 Tagesaggregate, 64 Byte je Record
- feste 64er Pending-Queue des bestehenden Persistenzpfads
- CRC-geschützte Raw-Records und transaktionale Metadaten
- nur der kleine aktive Zykluszustand wird zusätzlich in NVS gehalten
- START/ENDE werden separat als kleine Marken in `/cycles.log` protokolliert
- CSV wird beim Download gestreamt und enthält weiterhin ausschließlich echte Unterbrechungen

Damit bleiben bestehende 3.x-Rohdaten und sämtliche bisherigen Unterbrechungs-Auswertungen kompatibel. START und ENDE verfälschen keine Heatmap, keinen Tageszähler und keinen Durchschnitt.

Details: [`STORAGE_FORMAT.md`](STORAGE_FORMAT.md)

## Auswertung

- Wochentag × Stunde
- Monat × ISO-Kalenderwoche
- letzte fünf Kalenderjahre × Monat
- Fokus & Ruhe
- Arbeitsmuster

Alle Auswertungen arbeiten weiterhin mit den vorhandenen echten Unterbrechungen. Da 3.6.1 jeden gültigen kurzen Unterbrechungsdruck sofort übernimmt, gibt es während des Arbeitstags keinen verzögerten Zählerstand mehr.

Die Metrik **Ø Abstand** wird auf Anforderung aus den vorhandenen Rohereignissen berechnet. Ein gültiger Abstand wird der Start-Unterbrechung zugeordnet; über Mitternacht wird nie ein Intervall gebildet.

## Hardware

- DI1: GPIO13 gegen GND
- I2C: GPIO21/22 für DS3231 + SH1106
- DY-SV17F: RX GPIO18, TX GPIO19, BUSY GPIO39
- BUSY benötigt externen ca. 10-kΩ-Pull-up an DY-SV17F V33

Für die Zykluslogik ist **keine zusätzliche Hardware und kein weiterer GPIO** nötig. Kurzer und langer Druck werden am bestehenden DI1 unterschieden.

Details: [`HARDWARE_WIRING.md`](HARDWARE_WIRING.md)

## Sound / Display

Track 1 ist Boot/Test, Track 2 Anti-Spam, Track 3 und höher sind normale Unterbrechungstöne. Sound, Display-Master, Displayflash, Anzeigeart, Helligkeit und Dimmer werden persistent gespeichert.

Der Startdruck erzeugt keinen Unterbrechungs- oder Anti-Spam-Ton. Ein gültiger kurzer Unterbrechungsdruck verwendet sofort den normalen Soundpfad ab Track 3. Nur ein tatsächlich verworfener kurzer Druck darf Track 2 auslösen.

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
