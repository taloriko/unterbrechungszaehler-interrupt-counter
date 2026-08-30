# Software-Architektur

Firmwarepfad:

```text
arduino/Unterbrechungszaehler/
```

## Zweck

Die Firmware erfasst Unterbrechungen über einen Taster oder potentialfreien Kontakt, speichert Ereignisse dauerhaft und stellt sie über eine lokale Weboberfläche zur Auswertung bereit.

Ereigniserfassung, Speicherung, Zeitverwaltung, Netzwerk, Anzeige und Auswertung sind voneinander getrennt. Optionale Komponenten dürfen die grundlegende Ereigniserfassung nicht unnötig beeinflussen.

## Modulstruktur

Ein Funktionsmodul besteht möglichst aus:

```text
Modul.h      öffentliche Schnittstelle und Datentypen
Modul.cpp    Implementierung und Hardwarezugriffe
```

Für Services gelten einheitliche Regeln:

- `begin()` initialisiert Modul und Abhängigkeiten.
- `tick()` bearbeitet kurze zyklische Aufgaben.
- Direkte Aktionen verwenden eindeutige Verben wie `add`, `delete`, `read`, `set`, `start` oder `stop`.
- Hardware- und Dateisystemzugriffe bleiben im zuständigen Service gekapselt.
- Die Hauptdatei koordiniert Services und enthält keine Fachlogik der Module.

## Module

| Modul | Aufgabe |
| --- | --- |
| `Config` | Pins, Grenzwerte, Zeitkonstanten, Dateinamen und Kapazitäten |
| `StorageService` | LittleFS und Ringspeicher |
| `TimeService` | Systemzeit, NTP, Browserzeit und aktive Zeitquelle |
| `RtcService` | DS3231-Erkennung, Lesen und Schreiben |
| `DisplayService` | SH1106-Erkennung, Anzeige und Framebuffer |
| `NetworkService` | WLAN, Fallback-Access-Point und mDNS |
| `CounterService` | normale Unterbrechungen |
| `AutarkService` | Autark-Sessions und relative Zeiten |
| `InputService` | Taster und Autark-Schalter mit Entprellung |
| `LedService` | nicht blockierende LED-Rückmeldungen |
| `AnalyticsService` | Langzeitauswertungen und Heatmaps |
| `WebService` | HTTP-API, Weboberfläche und CSV-Ausgabe |
| `WebUi*` | HTML, CSS, JavaScript und Sprachverwaltung |

## Programmablauf

Die Hauptschleife arbeitet die Services in kurzen Schritten ab:

1. Eingänge lesen und entprellen.
2. Betriebsmodus aktualisieren.
3. Zeitstatus aktualisieren.
4. Autark-Zustand aktualisieren.
5. Display aktualisieren.
6. LED-Muster aktualisieren.
7. Netzwerk und Webserver bearbeiten.
8. Diagnosewerte ausgeben.
9. Im Autark-Leerlauf Light-Sleep verwenden.

Lange blockierende Abläufe werden vermieden.

## Speicherung

### Normaler Ringspeicher

Bis zu 10.000 Unix-Zeitstempel für aktuelle Ereignisse und Webansicht.

### Langzeit-Ringspeicher

Bis zu 100.000 Unix-Zeitstempel für Statistik, Heatmaps und Archivexport.

### Autark-Ringspeicher

Autark-Datensätze enthalten Session-ID, Datensatztyp, vergangene Sekunden seit Session-Start und optional einen absoluten Zeitanker.

### Blockzugriffe

Größere Datenmengen werden blockweise gelesen. Dadurch bleiben Dateisystemzugriffe und zusätzlicher Arbeitsspeicher begrenzt. Ringüberläufe werden innerhalb des Blockzugriffs berücksichtigt.

## Datensicherheit

Beim Speichern werden Datensatz und Ringkopf unmittelbar geschrieben. Der Ringkopf enthält Schreibposition und Anzahl gültiger Einträge.

Beschädigte oder unpassende Speicherdateien werden als `.invalid` gesichert und anschließend neu angelegt.

Der normale Ringspeicher hat Vorrang. Ein Fehler im Langzeit-Ringspeicher verhindert die normale Ereigniserfassung nicht.

## Ressourcen

Wesentliche feste Speicherbereiche:

- OLED-Framebuffer: 1.024 Byte
- Analysewerte: ungefähr 21 KiB RAM
- temporärer Analyseblock: 256 Zeitstempel, ungefähr 1 KiB

Die Weboberfläche liegt im Flash (`PROGMEM`). Große Datenbestände werden blockweise verarbeitet.

## Zeitverwaltung

Priorität der absoluten Zeitquellen:

1. NTP
2. RTC
3. Browserzeit

Eine gültige RTC kann beim Start die Systemzeit setzen. Nach erfolgreicher NTP-Synchronisation wird die RTC aktualisiert.

Ohne gültige absolute Zeit werden normale Ereignisse nicht gespeichert. Der Autarkbetrieb kann relative Zeiten innerhalb einer Session verwenden.

## Netzwerk

Im Normalbetrieb verbindet sich der ESP32 mit dem konfigurierten WLAN. Solange keine Verbindung besteht, steht ein lokaler Fallback-Access-Point zur Verfügung.

Im Autarkbetrieb werden Webserver, mDNS, Access Point und WLAN-Funk abgeschaltet. Gespeicherte WLAN-Daten bleiben erhalten.

## Display

Das SH1106-OLED verwendet einen 128×64-Framebuffer mit 1.024 Byte.

Ein neuer Frame wird hauptsächlich bei Zustandsänderungen erzeugt, beispielsweise bei Minutenwechsel, Netzwerkstatus, RTC-Status, Ereignissen oder geänderten Displayeinstellungen.

Im Autarkbetrieb wird das Display nach der vorgesehenen Anzeigezeit vollständig abgeschaltet.

## Sprachverwaltung

Sichtbare Oberflächentexte verwenden Sprachschlüssel. Sprachen werden zentral registriert und verwenden denselben Schlüsselsatz. Fehlt eine Übersetzung, wird auf Deutsch zurückgefallen.

Die Quelldaten liegen unter:

```text
translations/ui-translations.csv
```

Neue Sprachen werden als zusätzliche Sprachdefinition ergänzt und mit `tools/check_translations.py` auf fehlende Schlüssel geprüft.

## Fehlerverhalten optionaler Komponenten

RTC und OLED sind optional. Fehlt eine dieser Komponenten, bleiben Ereigniserfassung, Speicherung und nicht betroffene Funktionen verfügbar.

Netzwerk und Weboberfläche liegen außerhalb des zeitkritischen Erfassungspfads. Ein fehlendes WLAN verhindert die lokale Erfassung über den physischen Eingang nicht, sofern die benötigte Zeitbasis für den gewählten Betriebsmodus vorhanden ist.
