# Release Notes – Unterbrechungszähler 3.0.0

3.0.0 ist der neue Ausgangspunkt des Projekts. Frühere 1.x/2.x-Stände waren Entwicklungs- und Testversionen; es gibt keine zugesicherte Migration.

## Enthalten

- Unterbrechungstaster auf DI1/GPIO13 und Webbutton
- Live-Tageszähler und letzte Unterbrechung
- Europe/Berlin-Kalenderauswertung
- 100.000er Raw-Ringspeicher
- 2.300 Tagesaggregate
- CSV-Streamingexport
- drei Heatmap-Ansichten
- SH1106-Projektansichten und Displayflash
- DY-SV17F Boot-/Unterbrechungstöne
- fester oder rotierender Unterbrechungstrack
- NVS-gespeicherte Sound-/Displayeinstellungen
- DS3231/NTP/Browser-Zeitquellen
- OTA mit eigener 4-MiB-Partitionstabelle
- UI in Deutsch, Englisch und Schwäbisch

## Hardware 3.0.0

- DI1 GPIO13
- I2C GPIO21/22
- DY-SV17F UART GPIO18/19
- DY-SV17F BUSY GPIO39 mit externem Pull-up

## Release-Gates

Portable Hostchecks und der echte Arduino-ESP32-Build laufen über GitHub Actions. Der Release wird erst nach erfolgreichem Build auf `main` erzeugt.

Reale Hardwaretests bleiben zusätzlich erforderlich; Details stehen in `TEST_REPORT.md`.
