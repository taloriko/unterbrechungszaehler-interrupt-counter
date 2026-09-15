# Unterbrechungszähler 3.6.0

Version 3.0.0 bleibt der harte Ausgangspunkt des Projekts. 3.6.0 erweitert die physische Erfassung um einen Arbeitszyklus, ohne das bestehende 9-Byte-Raw-Format oder die bisherigen Unterbrechungs-Auswertungen zu verändern.

## Neu in 3.6.0: Arbeitszyklus

Der bestehende Taster auf **DI1/GPIO13** bekommt eine zweite Aufgabe, ohne dass zusätzliche Hardware nötig ist:

- **erster kurzer Druck:** Arbeitsbeginn / START – zählt nicht als Unterbrechung
- **weitere kurze Drücke:** der jeweils letzte Druck bleibt zunächst als Kandidat offen
- **nächster gültiger kurzer Druck:** bestätigt den vorherigen Kandidaten als echte Unterbrechung
- **langer Druck ab 2 Sekunden:** beendet den Arbeitszyklus ausdrücklich
- **kein langer Druck:** beim lokalen Tageswechsel wird der letzte Kandidat automatisch als ENDE gewertet und nicht gezählt

Das bedeutet praktisch: **erster Druck = Start, letzter Druck = Ende, alles dazwischen = Unterbrechung.**

Beim manuellen langen Enddruck zeigt das OLED anschließend für **10 Sekunden „FEIERABEND“ plus die heutige Unterbrechungszahl**. Beim automatischen Tagesabschluss bleibt das Display ruhig.

Der Webbutton bleibt unabhängig und erzeugt weiterhin sofort eine Unterbrechung.

## Anti-Spam

Die feste 10-Sekunden-Sperre für kurze physische Tastendrücke bleibt erhalten. Auch der Startdruck eröffnet diese Sperre. Weitere kurze Drücke innerhalb der Sperrzeit werden verworfen, lösen das bekannte Track-2-/OLED-Anti-Spam-Feedback aus und verändern keine Statistik. Ein verworfener Druck verlängert die Sperre nicht.

Der lange Feierabend-Druck ist von der Sperre ausgenommen, damit der Zyklus jederzeit ausdrücklich beendet werden kann.

## Funktionen

- Unterbrechungserfassung per DI1/GPIO13 und unabhängigem Webbutton
- lokaler Arbeitszyklus mit START/ENDE ohne zusätzliche Pins
- lokale Weboberfläche ohne Cloud
- Tageszähler, letzte Unterbrechung und Heatmaps
- Anzahl oder Ø abgeschlossener Abstand
- Fokus & Ruhe sowie Arbeitsmuster
- CSV-Export
- 100.000 bestätigte Roh-Unterbrechungen im 9-Byte-Ringspeicher
- 2.300 Tagesaggregate
- DS3231 RTC
- SH1106 OLED mit mehreren Ansichten, Helligkeit, Dimmer und 180°-Drehung
- DY-SV17F Soundmodul; Track 1 Boot/Test, Track 2 Anti-Spam, Track 3+ Unterbrechungen
- OTA-Update
- UI in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch

## Datenkompatibilität

START und ENDE werden bewusst **nicht** in den Unterbrechungs-Ring geschrieben. Der aktuell offene letzte Kurzdruck wird kompakt in NVS gehalten; START/ENDE landen zusätzlich in einem kleinen separaten Zyklusjournal. Nur bestätigte Unterbrechungen erreichen Raw-Ring und Tagesaggregate.

Damit bleiben bestehende 3.x-Daten, Heatmaps, CSV, Fokus-Auswertungen und Herkunftsfilter kompatibel. Details: [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md).

## Hardware

Die Pinbelegung bleibt unverändert. Für 3.6.0 ist kein zusätzlicher Taster nötig.

Siehe [HARDWARE.md](HARDWARE.md).

## Software und Flashen

Siehe [SOFTWARE.md](SOFTWARE.md).

## Technische Details

- [Sketch-Dokumentation](../../Unterbrechungszaehler/README.md)
- [Architektur](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Zeitarchitektur](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Testbericht](../../Unterbrechungszaehler/TEST_REPORT.md)
- [Release Notes](../../Unterbrechungszaehler/RELEASE_NOTES.md)

[English](../en/README.md) · [Schwäbisch](../swg/README.md) · [Projektstartseite](../../README.md)
