# Unterbrechungszähler 3.6.0 – Schwäbisch

> [!WARNING]
> **KI-Hinweis:** Des Projekt isch mit ordentlich KI-Unterstützung entstanden, danach aber in echt getestet, verbessert ond weitergebaut worda. Wenn du KI-generierten Code grundsätzlich net leiden kasch, darfst trotzdem dr Knopf drucka. ;-)

[Deutsch](../de/README.md) · [English](../en/README.md) · [Projektstartseite](../../README.md)

## Neu in 3.6.0: dr Arbeitszyklus

Dr gleiche Knopf auf **DI1/GPIO13** weiß jetzt au, wann dr Arbeitstag anfängt ond aufhört – ganz ohne an zweite Knopf oder no an GPIO.

- **erschter kurzer Druck:** START – zählt net als Unterbrechung
- **weitere kurze Drück:** dr jeweils letschte bleibt erscht mol als Kandidat offa
- **dr nächschte gültige kurze Druck:** macht aus em vorherige Kandidat a echte Unterbrechung
- **lang drucka, mindestens 2 Sekunda:** Feierabend ausdrücklich beenda
- **lang drucka vergessa:** beim lokale Tageswechsel wird dr letschte Kandidat automatisch s ENDE ond zählt net als Unterbrechung

Also ganz einfach: **erschter Druck = Start, letschter Druck = Ende, alles dazwischa = Unterbrechung.**

Wenn dr Feierabend mit em lange Druck gmacht wird, zeigt s OLED danach **10 Sekunda „FEIERABEND“ plus d heutige Anzahl Unterbrechunga**. Beim automatische Tagesabschluss bleibt s Display ruhig.

Dr Webknopf bleibt unabhängig ond macht weiterhin direkt a Unterbrechung.

## Anti-Spam

D 10-Sekunda-Sperre für kurze echte Knopfdrück bleibt. Au dr Startdruck macht die Sperre auf. Was innerhalb dere Zeit nomol kurz druckt wird, wird net gspeichert ond net gezählt; Track 2 ond s OLED gebet bloß s bekannte Anti-Spam-Feedback. A verworfener Druck verlängert d Sperre net.

Dr lange Feierabend-Druck wird davon net blockiert. Sonst wär des ja ausgerechnet beim Heimgoa lästig.

## Was kann s Gerät?

- physische Erfassung über DI1/GPIO13 ond unabhängiger Webknopf
- lokaler START/ENDE-Arbeitszyklus ohne zusätzliche Hardware
- lokale Weboberfläche ohne Cloud
- Tageszähler, letzte Unterbrechung ond Heatmaps
- Anzahl oder Ø abgeschlossener Abstand
- Fokus & Ruh sowie Arbeitsmuster
- CSV-Export
- 100.000 bestätigte Unterbrechunga im unveränderte 9-Byte-Ringspeicher
- 2.300 Tagesaggregate
- DS3231 RTC
- SH1106 OLED mit mehrere Ansichten, Helligkeit, Dimmer ond 180°-Drehung
- DY-SV17F: Track 1 Boot/Test, Track 2 Anti-Spam, Track 3+ normale Unterbrechungstön
- OTA-Update
- Oberfläche auf Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch ond Oberschwäbisch

## Daten bleiben kompatibel

START ond ENDE kommet absichtlich **net** in dr Unterbrechungs-Ringspeicher. Dr offene letschte Kurzdruck wird klein im NVS gmerkt; START/ENDE kommet zusätzlich in a separates kleines Zyklusjournal. Bloß bestätigte Unterbrechunga landet im bisherigen Raw-Ring ond in de Tagesaggregate.

Drum bleibet d vorhandene 3.x-Daten, Heatmaps, CSV, Fokus-Auswertunga ond Herkunftsfilter kompatibel. Details: [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md).

## Hardware

D Pinbelegung bleibt gleich. Für 3.6.0 braucht s koi zusätzliche Hardware.

| Funktion | ESP32 |
|---|---:|
| Taster / DI1 | GPIO13 gegen GND |
| I2C SDA – RTC + OLED | GPIO21 |
| I2C SCL – RTC + OLED | GPIO22 |
| DY-SV17F TX → ESP32 RX | GPIO18 |
| ESP32 TX → DY-SV17F RX | GPIO19 |
| DY-SV17F CON3/BUSY | GPIO39 / VN |

## Technische Dokumentation

- [Sketch-Dokumentation](../../Unterbrechungszaehler/README.md)
- [Hardware-Wiring](../../Unterbrechungszaehler/HARDWARE_WIRING.md)
- [Architektur](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Zeitarchitektur](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Testbericht](../../Unterbrechungszaehler/TEST_REPORT.md)
- [Release Notes](../../Unterbrechungszaehler/RELEASE_NOTES.md)

## Lizenz

MIT. Benutza, ändra, erweitera ond ebbes Eigenes draus baua isch ausdrücklich erlaubt. Wenn daraus irgendwann a millionenschweres Produkt wird, freu i mi immer no über a Postkarte.
