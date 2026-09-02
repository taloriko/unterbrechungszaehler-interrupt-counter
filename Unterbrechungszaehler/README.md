# Unterbrechungszähler 3.0.0 – technische Übersicht

Dies ist der Sketchordner der Version **3.0.0** für ein klassisches ESP32 Dev Module / ESP32-WROOM-32.

## Ereigniserfassung

Unterbrechungen kommen über DI1/GPIO13 oder den Webbutton. Beide Wege laufen durch `InterruptionService`. Der Live-Zähler wird zuerst aktualisiert; Display-/Audiofeedback und persistente Speicherung folgen kooperativ.

## Speicherung

- 100.000 Raw-Events, 9 Byte je Record
- 2.300 Tagesaggregate, 64 Byte je Record
- feste 64er Pending-Queue
- CRC-geschützte Records und transaktionale Metadaten
- CSV wird beim Download gestreamt

Details: [`STORAGE_FORMAT.md`](STORAGE_FORMAT.md)

## Auswertung

- Wochentag × Stunde
- Monat × ISO-Kalenderwoche
- letzte fünf Kalenderjahre × Monat

Statistiken lesen Tagesaggregate und nicht bei jeder Ansicht den vollständigen Raw-Ring.

## Hardware

- DI1: GPIO13 gegen GND
- I2C: GPIO21/22 für DS3231 + SH1106
- DY-SV17F: RX GPIO18, TX GPIO19, BUSY GPIO39
- BUSY benötigt externen ca. 10-kΩ-Pull-up an DY-SV17F V33

Details: [`HARDWARE_WIRING.md`](HARDWARE_WIRING.md)

## Sound / Display

Track 1 ist Boot-Ton. Unterbrechungen verwenden einen festen Track ab 2 oder rotieren über erkannte Tracks 2…N. Sound, Displayflash, Anzeigeart, Helligkeit und Dimmer werden persistent gespeichert.

## Zeit

Absolute Zeit bleibt UTC; lokale Auswertung nutzt `Europe/Berlin`. NTP hat Priorität, RTC und Browser dienen als Fallback. Ohne gültige absolute Zeit bleibt ein Event relativ.

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
- [`TEST_REPORT.md`](TEST_REPORT.md)
- [`../README.md`](../README.md)
