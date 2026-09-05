# Unterbrechungszähler 3.2.0

Version 3.0.0 ist der neue Ausgangspunkt des Projekts. Frühere 1.x/2.x-Stände waren Entwicklungs- und Testversionen und werden nicht als Migrationsziel behandelt.

## Funktionen

**Neu in 3.2.0:**

- OLED folgt der gewählten UI-Sprache, inklusive kompakter Umlaut-/Akzent-Transliteration
- fünf OLED-Modi, 180°-Drehung und mindestens 4 Sekunden Bootscreen
- neue Display-Defaults 65 % normal / 5 % gedimmt
- DY-SV17F-Lautstärke 0–100 %, Standard 100 %, Tonmodus standardmäßig wechselnd


- Unterbrechung per Taster auf GPIO13 / DI1 oder Webbutton
- lokale Weboberfläche
- Tageszähler, letzte Unterbrechung und Heatmaps: Anzahl oder Ø abgeschlossener Abstand
- CSV-Export
- 100.000 Rohereignisse im binären Ringspeicher
- 2.300 Tagesaggregate
- DS3231 RTC
- SH1106 OLED mit persistentem Ein/Aus-Schalter und mindestens 2 s Bootbild
- DY-SV17F Soundmodul; Startpaket und USB-Kopieranleitung siehe [HARDWARE.md](HARDWARE.md)
- OTA-Update
- persistente Sound-/Displayeinstellungen
- UI in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch

Die ausführliche README-Dokumentation wird bewusst nur in Deutsch, Englisch und Schwäbisch gepflegt.

## Hardware

Siehe [HARDWARE.md](HARDWARE.md).

## Software und Flashen

Siehe [SOFTWARE.md](SOFTWARE.md).

## Technische Details

- [Sketch-Dokumentation](../../Unterbrechungszaehler/README.md)
- [Architektur](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Zeitarchitektur](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Testbericht](../../Unterbrechungszaehler/TEST_REPORT.md)

[English](../en/README.md) · [Schwäbisch](../swg/README.md) · [Projektstartseite](../../README.md)

## Neu in 3.3.0

Heatmaps können nach **Beides**, **Knopf / GPIO** oder **Web** gefiltert werden. Einzelne Herkunftsfilter arbeiten aus dem noch vorhandenen Roh-Ringspeicher und zeigen eine unvollständige Abdeckung offen an. Die komplette Datenbank kann nach Eingabe des Projektnamens `Unterbrechungszähler` gelöscht werden; danach startet das Gerät neu. Gerät → Speicher zeigt bei Fehlern die konkrete Ursache für Rohdaten oder Tagesstatistik.
