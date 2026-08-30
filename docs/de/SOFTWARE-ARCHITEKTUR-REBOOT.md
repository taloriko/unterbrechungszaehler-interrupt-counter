# Software-Architektur

Die Firmware befindet sich unter:

```text
arduino/UnterbrechungszaehlerModular/
```

## Zweck

Die Firmware erfasst Unterbrechungen über einen Taster beziehungsweise einen potentialfreien Kontakt, speichert die Ereignisse dauerhaft und stellt sie über eine lokale Weboberfläche zur Auswertung bereit.

Der Programmaufbau trennt Ereigniserfassung, Speicherung, Zeitverwaltung, Netzwerk, Anzeige und Auswertung voneinander. Optionale Hardware oder Komfortfunktionen dürfen die grundlegende Ereigniserfassung nicht unnötig beeinflussen.

## Aufbau der Module

Jedes Funktionsmodul besteht möglichst aus einer Header- und einer Implementierungsdatei:

```text
Modul.h      öffentliche Schnittstelle, Zustände und Datentypen
Modul.cpp    interne Logik und Hardwarezugriffe
```

Für Services gelten einheitliche Grundprinzipien:

- `begin()` initialisiert das Modul und seine Abhängigkeiten.
- `tick()` bearbeitet wiederkehrende Aufgaben ohne lange blockierende Wartezeiten.
- Direkte Aktionen verwenden eindeutige Verben wie `add`, `delete`, `read`, `set`, `start` oder `stop`.
- Abhängigkeiten werden beim Initialisieren übergeben und als Zeiger gespeichert.
- Hardwaremodule stellen ihren Zustand über Methoden wie `present()`, `available()` oder `connected()` bereit.
- Hardwarezugriffe und Dateisystemzugriffe bleiben im zuständigen Service gekapselt.
- Die Hauptdatei koordiniert die Services und enthält keine Fachlogik der einzelnen Funktionen.

## Module

| Modul | Aufgabe |
| --- | --- |
| `Config` | Pins, Grenzwerte, Zeitkonstanten, Dateinamen und Speicherkapazitäten |
| `StorageService` | LittleFS, normaler Ringspeicher, Langzeit-Ringspeicher und Autark-Datensätze |
| `TimeService` | Systemzeit, NTP, Browserzeit, Zeitzone und aktive Zeitquelle |
| `RtcService` | Erkennung, Lesen und Schreiben der optionalen DS3231-Echtzeituhr |
| `DisplayService` | Erkennung, Ansteuerung und Framebuffer des optionalen SH1106-OLED |
| `NetworkService` | WLAN, Fallback-Access-Point und mDNS |
| `CounterService` | Erfassen und Löschen normaler Unterbrechungen |
| `AutarkService` | Autark-Sessions, relative Laufzeit und Autark-Ereignisse |
| `InputService` | Taster und Autark-Schalter inklusive Entprellung und Langdruck-Erkennung |
| `LedService` | nicht blockierende LED-Rückmeldungen |
| `AnalyticsService` | aggregierte Langzeitauswertungen und Heatmaps |
| `WebService` | HTTP-API, Weboberfläche und CSV-Ausgabe |
| `WebUi*` | HTML, CSS und JavaScript der Weboberfläche |

## Programmablauf

Die Hauptschleife bearbeitet die Services in kurzen Arbeitsschritten:

1. Eingänge lesen und entprellen.
2. Wechsel zwischen Normal- und Autarkbetrieb anwenden.
3. Zeitstatus aktualisieren.
4. Autark-Zustand aktualisieren.
5. Display aktualisieren.
6. LED-Muster aktualisieren.
7. Netzwerk und Webserver im Normalbetrieb bearbeiten.
8. Diagnosewerte in festen Intervallen ausgeben.
9. Im inaktiven Autarkbetrieb Light-Sleep verwenden.

Lange blockierende Abläufe werden vermieden. Kurze Wartezeiten werden nur dort verwendet, wo Hardwareprotokolle oder externe Antworten sie erfordern.

## Speicherstrategie

### Normaler Ringspeicher

Der normale Ringspeicher enthält bis zu 10.000 Unix-Zeitstempel. Er ist die primäre Datenquelle für aktuelle Ereignisse und die Webansicht.

### Langzeit-Ringspeicher

Der Langzeit-Ringspeicher enthält bis zu 100.000 Unix-Zeitstempel. Er dient als Datenbasis für Langzeitauswertungen, Heatmaps und den vollständigen Archivexport.

### Autark-Ringspeicher

Autark-Datensätze enthalten:

- Session-ID
- Datensatztyp `Start`, `Event` oder `End`
- vergangene Sekunden seit Session-Start
- optionalen absoluten Zeitanker

Dadurch bleiben die relativen Abstände auch dann erhalten, wenn beim Erfassen keine absolute Uhrzeit verfügbar ist.

### Blockzugriffe

Größere Datenmengen werden blockweise aus den Ringspeichern gelesen. Eine Datei wird dabei einmal pro Block geöffnet und zusammenhängende Datensätze werden gemeinsam gelesen. Auch ein Überlauf vom Ende zum Anfang eines Ringspeichers wird innerhalb eines Blockzugriffs berücksichtigt.

Einzelzugriffe bleiben für Funktionen erhalten, die nur einen bestimmten Datensatz benötigen.

## Datensicherheit

Beim Speichern eines Ereignisses werden Datensatz und Ringkopf unmittelbar geschrieben. Der Ringkopf enthält Schreibposition und Anzahl der gültigen Einträge.

Beschädigte oder unpassende Speicherdateien werden nicht automatisch überschrieben. Sie werden mit der Endung `.invalid` gesichert und anschließend neu angelegt.

Der normale Ringspeicher hat Vorrang vor dem Langzeit-Ringspeicher. Fällt nur das Langzeitarchiv aus, bleibt die normale Erfassung funktionsfähig und der Synchronisationsstatus wird als fehlerhaft markiert.

## Ressourcenverwendung

Die Firmware verwendet überwiegend statisch dimensionierte Datenstrukturen, damit der Heap nicht durch große dauerhaft wechselnde Allokationen belastet wird.

Wesentliche feste Speicherbereiche sind:

- OLED-Framebuffer: 1.024 Byte
- Analysewerte für Wochentag, Stunde, Kalenderwoche und Monat: ungefähr 21 KiB RAM
- temporäre Analyseblöcke: 256 Zeitstempel beziehungsweise ungefähr 1 KiB Stack

Die Weboberfläche liegt als `PROGMEM` im Flash und wird nicht dauerhaft in den RAM kopiert.

Große Datenbestände werden blockweise verarbeitet. Dadurch bleibt der zusätzliche Arbeitsspeicher unabhängig von der Anzahl gespeicherter Ereignisse begrenzt.

## Display

Das SH1106-OLED verwendet einen 1.024 Byte großen 128×64-Framebuffer.

Im Livebetrieb wird das Display nicht in jeder Hauptschleife neu aufgebaut. Ein neuer Frame wird hauptsächlich bei folgenden Änderungen erzeugt:

- Minutenwechsel
- WLAN- oder Access-Point-Status
- RTC-Status
- neues oder gelöschtes Ereignis
- geänderte Displayeinstellungen

Im Autarkbetrieb wird das Display nach der vorgesehenen Anzeigezeit vollständig abgeschaltet.

## Zeitverwaltung

Die absolute Systemzeit kann aus folgenden Quellen stammen:

1. NTP
2. RTC
3. Browserzeit

Eine gültige RTC kann beim Start die Systemzeit setzen. Nach erfolgreicher NTP-Synchronisation wird die RTC mit der Systemzeit aktualisiert.

Ohne gültige absolute Zeit werden normale Ereignisse nicht gespeichert. Der Autarkbetrieb verwendet stattdessen relative Zeiten innerhalb einer Session.

## Netzwerk

Im Normalbetrieb versucht der ESP32, das konfigurierte WLAN zu verwenden. Solange keine Verbindung besteht, steht ein lokaler Fallback-Access-Point zur Verfügung.

Bei erfolgreicher WLAN-Verbindung wird der Fallback-Access-Point abgeschaltet und mDNS gestartet.

Im Autarkbetrieb werden Webserver, mDNS, Access Point und WLAN-Funk abgeschaltet. Die gespeicherte WLAN-Konfiguration wird dabei nicht gelöscht, damit beim Rückwechsel keine unnötigen Flash-Schreibzugriffe entstehen.

## Autarkbetrieb

Beim Aktivieren des Autarkbetriebs:

- wird eine neue Session angelegt,
- wird WLAN abgeschaltet,
- wird der CPU-Takt reduziert,
- bleibt die Ereigniserfassung aktiv,
- kann eine gültige RTC als Zeitquelle verwendet werden,
- zeigt ein vorhandenes OLED den Startstatus,
- wird anschließend im Leerlauf Light-Sleep verwendet.

Beim Verlassen der Session wird ein Ende-Datensatz gespeichert. Ist noch keine absolute Zeit verfügbar, kann der fehlende Zeitanker später nachgetragen werden.

## Fehlerverhalten optionaler Komponenten

RTC und OLED sind optional. Fehlt eines dieser Module, bleiben Ereigniserfassung, Speicherung und die jeweils nicht betroffenen Funktionen verfügbar.

Auch Netzwerk und Weboberfläche sind nicht Bestandteil des zeitkritischen Erfassungspfads. Ein fehlendes WLAN verhindert daher nicht die lokale Bedienung über den physischen Taster, sofern für den gewählten Betriebsmodus die benötigte Zeitbasis vorhanden ist.
