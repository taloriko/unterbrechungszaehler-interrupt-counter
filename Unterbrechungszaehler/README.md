# Unterbrechungszähler 3.6.8 – technische Übersicht

Dies ist der Sketchordner der Version **3.6.8** für ein klassisches ESP32 Dev Module / ESP32-WROOM-32.

## Erweiterungen in 3.6.8

Der DI1-Taster wird durch `WorkCycle` klassifiziert: Erster kurzer Druck startet einen aktiven Zyklus ohne Rohereignis, weitere akzeptierte kurze Drücke laufen über `InterruptionService::capture(PhysicalButton)`, und mindestens 2 Sekunden Halten mit anschließendem Loslassen beendet den Zyklus. Alle Drücke darunter gelten als kurz und unterliegen gegebenenfalls der bisherigen 10-Sekunden-Sperre. Der Webbutton bleibt unabhängig.

Ab 250 ms Halten zeigt `WorkCycle` einen OLED-Fortschrittsbalken im bestehenden Framebuffer (nur bei gültiger lokaler Uhrzeit beim Druckbeginn). Der Balken erreicht nach 2 Sekunden 100 %. Manuelles Ende: 10 Sekunden Abschlussanzeige, danach Status `PAUSE`. Beginn und Ende werden unabhängig vom Unterbrechungs-Ring protokolliert.

Die automatische lokale Endzeit wird über `ProjectPreferences::workCycleEndMinute()` als kompakter NVS-Wert in Minuten nach Mitternacht gespeichert (Default `0` = **00:00**). Sie betrifft nur den offenen Zyklus; Tagesstatistik und Raw-Event-Zuordnung wechseln weiterhin um 00:00 Uhr. Ein über die Endzeit offener Zyklus wird beim nächsten verfügbaren Zeitwert geschlossen. Nach Stromausfall entspricht der gespeicherte Abschlusszeitpunkt der Wiederanlaufzeit.

Das OLED zeigt in der Standardansicht einen maximal skalierten Tageszähler und links den Zyklusstatus; die Weboberfläche bietet eine eigene Karte **Arbeitszyklus**. Neue OLED-Meldungen sind für alle sieben Sprachvarianten vorgesehen. Der Audiowechsel beginnt nach Boot zufällig bei einem erkannten normalen Track 3…N, läuft anschließend zyklisch und verwendet bei unbekannter Trackanzahl den eingestellten Basiston. Ein Track wird bei belegt gemeldetem Treiber nur nach akzeptiertem Abspielbefehl weitergeschaltet.

Vor dem direkten Kompilieren in der Arduino-IDE `python3 tools/build_web.py` ausführen, wenn `ui-src/*` geändert wurde. Das eingecheckte `web_assets.h` enthält für 3.6.8 bereits die aktuelle Oberfläche; die GitHub-Releaseprüfung kontrolliert seine Übereinstimmung. Alle bisherigen Persistenzformate und OTA-/LittleFS-Partitionen bleiben unverändert.

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

Die Anzahl-Heatmaps lesen weiterhin die kompakten Tagesaggregate. Die neue Metrik **Ø Abstand** wird nur auf Anforderung aus den noch vorhandenen Rohereignissen berechnet: Ein gültiger Abstand wird der Start-Unterbrechung zugeordnet; der letzte Druck eines Kalendertages bleibt ausgeschlossen und über Mitternacht wird nie ein Intervall gebildet.

## Hardware

- DI1: GPIO13 gegen GND
- I2C: GPIO21/22 für DS3231 + SH1106
- DY-SV17F: RX GPIO18, TX GPIO19, BUSY GPIO39
- BUSY benötigt externen ca. 10-kΩ-Pull-up an DY-SV17F V33

Details: [`HARDWARE_WIRING.md`](HARDWARE_WIRING.md)

## Sound / Display

Track 1 ist Boot-Ton. Unterbrechungen verwenden einen festen Track ab 2 oder rotieren über erkannte Tracks 2…N. Sound, Display-Master, Displayflash, Anzeigeart, Helligkeit und Dimmer werden persistent gespeichert. Das OLED-Bootbild bleibt mindestens 2 Sekunden sichtbar, ohne den Projektloop zu blockieren. Die Sounddateien und USB-Kopieranleitung stehen in [`../docs/de/HARDWARE.md`](../docs/de/HARDWARE.md).

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
