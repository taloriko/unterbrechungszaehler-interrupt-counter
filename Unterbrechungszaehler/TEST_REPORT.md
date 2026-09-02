# Testbericht – Unterbrechungszähler 3.0.0

Auditdatum: 02.09.2026  
Ziel: ESP32 Dev Module / ESP32-WROOM-32, Arduino-ESP32

## Automatisierte Prüfungen

Der Releasecheck `tools/release_check.py` prüft:

- Projektname und Version 3.0.0
- deklarierte UI-Sprachen Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch
- Parität der drei Basis-Sprachpakete Deutsch/Englisch/Schwäbisch sowie Vorhandensein der zusätzlichen Sprach-/Dialektpakete
- 100.000er Raw-Ring und 2.300 Tagesaggregate
- DI1 auf GPIO13 mit aktivem Edge-Latch
- DY-SV17F auf GPIO18/19 plus BUSY GPIO39
- Projekt-API-Routen
- Storage-/Recovery-Simulationen
- JavaScript-Syntax
- deterministisches Webbundle, ETag und gzip-Roundtrip

GitHub Actions führt diese Checks zusätzlich aus, regeneriert `web_assets.h` zur Diff-Prüfung und kompiliert den echten Sketch mit Arduino-ESP32. Ein Release wird blockiert, wenn weniger als 64 KiB Reserve im App-Slot verbleiben.

## Bereits aus dem Entwicklungsstand vorhandene Hosttests

- 14/14 Storage-/Recovery-/Heatmap-Simulationen bestanden
- 100.000er Ring-Wrap bestanden
- identische i18n-Keysets der drei Basis-Sprachpakete Deutsch/Englisch/Schwäbisch
- zusätzliche UI-Pakete Italienisch, Französisch, Alb-Schwäbisch und Oberschwäbisch vorhanden
- Webbundle/ETag/gzip-Prüfung bestanden
- C++-Syntaxprüfungen gegen Host-Stubs bestanden

Diese Hostprüfungen ersetzen nicht den echten Arduino-Build; deshalb ist die GitHub-CI das verbindliche Software-Releasegate.

## Reale Hardwaretests

Nach dem CI-Build weiterhin auf einem Zielgerät prüfen:

- DI1/GPIO13: ein Tastendruck = ein Event
- Debounce und kurzer Tastendruck während längerer HTTP-/CSV-Ausgabe
- SH1106: Standard-/Zahl-/Letzte-Unterbrechung-Ansicht, Helligkeit und Flash
- DY-SV17F: Boot-Track 1, fester Track ab 2, Rotation 2…N, BUSY GPIO39
- DS3231-Fallback und NTP-Zeit
- Offlinebetrieb ohne WLAN
- LittleFS-Persistenz über Reboot
- OTA mit Erhalt von NVS/LittleFS
- CSV mit echten Ereignissen
- Heatmaps auf Desktop und schmalem Mobilgerät
- wiederholte Schreibvorgänge/Reboots im Langzeittest

## Freigabe

3.0.0 darf nach erfolgreicher GitHub-CI als Software-Release veröffentlicht werden. Die genannten Hardwaretests bleiben die praktische Gerätevalidierung.
