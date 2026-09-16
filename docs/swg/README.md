# Unterbrechungszähler 3.6.1 – Schwäbisch

> [!WARNING]
> **KI-Hinweis:** Des Projekt isch mit ordentlich KI-Unterstützung entstanden, danach aber in echt getestet, verbessert ond weitergebaut worda. Wenn du KI-generierten Code grundsätzlich net leiden kasch, darfst trotzdem dr Knopf drucka. ;-)

[Deutsch](../de/README.md) · [English](../en/README.md) · [Projektstartseite](../../README.md)

## Neu in 3.6.1: dr Arbeitszyklus neu eingebaut

Dr gleiche Knopf auf **DI1/GPIO13** weiß weiterhin, wann dr Arbeitstag anfängt ond aufhört – aber d Logik isch jetzt wieder bewusst direkt ond nachvollziehbar.

- **erschter kurzer Druck:** START – zählt net als Unterbrechung ond macht au no koi Anti-Spam-Sperre auf
- **weitere gültige kurze Drück:** werdet sofort als echte Unterbrechung gspeichert ond krieget sofort s normale Feedback
- **lang drucka, mindestens 2 Sekunda:** Feierabend ausdrücklich beenda – zählt net als Unterbrechung
- **lang drucka vergessa:** beim lokale Tageswechsel wird bloß dr Zyklus automatisch zugmacht; echte Unterbrechunga werdet nachher net umgdeutet

Dr Pending-Kandidaten-Ansatz aus 3.6.0 isch wieder raus. Koi Knopfdruck wartet mehr drauf, dass erscht no dr nächschte Druck komma muss, bevor er zählt.

Wenn dr Feierabend mit em lange Druck gmacht wird, zeigt s OLED danach **10 Sekunda „FEIERABEND“ plus d heutige Anzahl Unterbrechunga**. In dere Zeit werdet weitere echte Knopfdrück ignoriert ond d normale Home-/Sekunda-Anzeige darf net dazwischenfunka. Beim automatische Tagesabschluss bleibt s Display ruhig.

Dr Webknopf bleibt unabhängig ond macht weiterhin direkt a Unterbrechung.

## Anti-Spam

D 10-Sekunda-Sperre bleibt für echte kurze **Unterbrechungsdrück**. Dr START-Druck macht die Sperre ausdrücklich net auf. Erscht a angenommene Unterbrechung startet die 10 Sekunda.

Was innerhalb dere Zeit nomol kurz druckt wird, wird net gspeichert ond net gezählt; Track 2 ond s OLED gebet bloß s bekannte Anti-Spam-Feedback. A verworfener Druck verlängert d Sperre net.

Dr lange Feierabend-Druck wird davon net blockiert. Sonst wär des ja ausgerechnet beim Heimgoa lästig.

## Was kann s Gerät?

- physische Erfassung über DI1/GPIO13 ond unabhängiger Webknopf
- lokaler START/ENDE-Arbeitszyklus ohne zusätzliche Hardware
- lokale Weboberfläche ohne Cloud
- Tageszähler, letzte Unterbrechung ond Heatmaps
- Anzahl oder Ø abgeschlossener Abstand
- Fokus & Ruh sowie Arbeitsmuster
- CSV-Export
- 100.000 Unterbrechunga im unveränderte 9-Byte-Ringspeicher
- 2.300 Tagesaggregate
- DS3231 RTC
- SH1106 OLED mit mehrere Ansichten, Helligkeit, Dimmer ond 180°-Drehung
- DY-SV17F: Track 1 Boot/Test, Track 2 Anti-Spam, Track 3+ normale Unterbrechungstön
- OTA-Update
- Oberfläche auf Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch ond Oberschwäbisch

## Daten bleibet kompatibel

START ond ENDE kommet absichtlich **net** in dr Unterbrechungs-Ringspeicher. Dr aktive Zyklus wird klein im NVS gmerkt; START/ENDE kommet zusätzlich in a separates kleines Zyklusjournal. Jede gültige kurze Unterbrechung landet dagegen sofort im bisherige Raw-Ring-/Aggregate-Weg.

Drum bleibet d vorhandene 3.x-Daten, Heatmaps, CSV, Fokus-Auswertunga ond Herkunftsfilter kompatibel. Details: [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md).

## Hardware

D Pinbelegung bleibt gleich. Für 3.6.1 braucht s koi zusätzliche Hardware.

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
