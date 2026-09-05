# Release 3.3.0

- Passwortgeschütztes vollständiges Löschen der Ereignisdatenbank (Bestätigung = Projektname)
- Heatmap-Herkunftsfilter für Beides / Knopf-GPIO / Web, mit offenem API-Modell für spätere Quellen
- konkrete Speicherfehler in Gerät → Speicher
- bestehende 3.1/3.2-Regeln für Ø-Abstand, Raw-Ring-Coverage, Embedded-Effizienz und nicht blockierende Bedienung bleiben erhalten

# Unterbrechungszähler 3.2.0

- OLED folgt der gewählten UI-Sprache mit ressourcenschonender Transliteration für Sonderzeichen
- Bootscreen mindestens 4 Sekunden, weiterhin nicht blockierend
- persistente 180°-Drehung und fünf Displaymodi inklusive Tagesfortschritt/Fokus
- frische Displaydefaults 65 % / 5 %
- DY-SV17F-Lautstärke 0–100 %, Standard 100 %; Wechselmodus ist neuer Standard
- Einstellungen klar nach Display, Display-Feedback und Sound gegliedert
- veralteter Arduino-Sketch-BIN-Hinweis im OTA-UI entfernt

# Release Notes – Unterbrechungszähler 3.1.0

3.1.0 erweitert die Auswertung und Displaysteuerung, ohne das bestehende Raw-Record-Format oder die 100.000er Ringspeicherkapazität zu ändern.

## Neu

- Heatmap-Metrik **Anzahl** oder **Ø Abstand**
- abgeschlossene Intervalle werden der Start-Unterbrechung zugeordnet
- letzter Tagesdruck sowie Intervalle über Mitternacht werden nicht verwendet
- Ø-Abstand basiert auf retained Rohereignissen und weist unvollständige Coverage aus
- Display persistent ein-/ausschaltbar
- Bootscreen mindestens zwei Sekunden sichtbar, ohne `delay(2000)`
- DY-SV17F-Soundpaket und Micro-USB-Kopieranleitung dokumentiert

## Datenmodell

Das 9-Byte-Raw-Format und der 64-Byte-Tagesaggregate-Record bleiben unverändert. Anzahl-Heatmaps verwenden die Tagesaggregate; Ø-Abstand scannt nur bei Bedarf den retained Raw-Ring und bildet `sum(intervalSeconds) / sampleCount` je Zelle.

---

# Release Notes – Unterbrechungszähler 3.0.1

3.0.1 ist ein Patch-Release auf Basis von 3.0.0.

## Änderungen gegenüber 3.0.0

- passwortgeschützter Fallback-AP mit `Unterbrechungszähler`
- alte statische OTA-/AP-Warnungen aus der normalen Oberfläche entfernt
- echte OTA-Fehlerdiagnose bleibt für tatsächliche Updatefehler erhalten
- OTA-Speicher zusätzlich als segmentierter Auslastungsbalken
- `ota.usedPercent` in der Geräte-API
- aktualisierte Releasechecks und Dokumentation

---

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
- UI in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch

Die ausführliche README-Dokumentation wird bewusst nur in Deutsch, Englisch und Schwäbisch gepflegt.

## Hardware 3.0.0

- DI1 GPIO13
- I2C GPIO21/22
- DY-SV17F UART GPIO18/19
- DY-SV17F BUSY GPIO39 mit externem Pull-up

## Release-Gates

Portable Hostchecks und der echte Arduino-ESP32-Build laufen über GitHub Actions. Der Release wird erst nach erfolgreichem Build auf `main` erzeugt.

Reale Hardwaretests bleiben zusätzlich erforderlich; Details stehen in `TEST_REPORT.md`.
