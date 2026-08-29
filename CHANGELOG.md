# Changelog

## 2026-08-29-18

- optionale I2C-Hardware-Erweiterungen hinzugefügt
- DS3231 RTC an `0x68` wird einmal beim Boot automatisch erkannt
- SH1106 OLED 128 × 64 an `0x3C` oder `0x3D` wird einmal beim Boot automatisch erkannt
- gemeinsamer I2C-Bus auf `GPIO21` (SDA) und `GPIO22` (SCL)
- Firmware bleibt auch ohne RTC und/oder Display vollständig lauffähig
- gültige DS3231-Zeit wird beim Start als Systemzeit übernommen
- RTC-OSF-Flag wird ausgewertet; ungültige RTC-Zeit wird nicht übernommen
- nach erfolgreicher NTP-Synchronisation wird eine erkannte RTC automatisch aktualisiert
- bei Browser-Zeitfallback kann die RTC ebenfalls mit der gültigen Systemzeit beschrieben werden
- Autark-Session erhält bei vorhandener RTC einen absoluten Zeitanker
- erkanntes SH1106 zeigt im Autark-Modus beim Start Status, RTC-Zustand und Uhrzeit
- OLED schaltet nach 15 Sekunden vollständig ab
- Weboberfläche zeigt RTC- und Display-Symbol dauerhaft; nicht erkannte Hardware wird ausgegraut
- zusätzliche RTC- und Display-Karten erscheinen nur bei erkannter Hardware
- RTC kann über die Weboberfläche manuell mit der aktuellen Systemzeit synchronisiert werden
- Display kann über die Weboberfläche für 15 Sekunden getestet werden
- neue Hardware-API `/api/hardware`
- Hardwarebeschaffung, Verdrahtung und Autark-Dokumentation ergänzt
- Firmware-Runtime-Version auf `2026-08-29-18` gesetzt

## 2026-08-29-16

- Langzeitaufzeichnung fuer mindestens 10 Jahre vorbereitet
- zusaetzlicher Ringspeicher `/events_10y.bin` mit 100.000 absoluten Zeitstempeln
- 100.000 Eintraege benoetigen nur rund 400 kB Rohdaten
- bei 30 Unterbrechungen pro Werktag reicht die Kapazitaet rechnerisch fuer ca. 12,8 Jahre
- bestehender 10.000er Ringspeicher bleibt unveraendert fuer schnelle Tages-, Verlauf- und Detailansichten
- vorhandene normale Ereignisse werden beim ersten Start automatisch in den Langzeitspeicher uebernommen
- neue normale Ereignisse und Loeschvorgaenge werden parallel im Langzeitspeicher nachgefuehrt
- Autark-Daten bleiben weiterhin getrennt im Autark-Ringspeicher
- neue API `/api/aggregate` fuer kompakte Langzeitauswertungen
- Aggregat-API liefert Wochentag/Uhrzeit, Monat/Kalenderwoche, Jahr/Monat und verfuegbare Jahre
- Heatmaps laden nicht mehr alle Langzeit-Zeitstempel, sondern nur noch wenige Kilobyte aggregierte Werte
- Monat/Kalenderwoche zeigt 12 Monate vertikal und KW 1 bis 52 horizontal mit Jahresauswahl
- aktuelles Jahr ist bei Monat/Kalenderwoche vorausgewaehlt; Jahre mit vorhandenen Daten stehen im Dropdown zur Verfuegung
- Jahr/Monat zeigt statisch immer das aktuelle Jahr und die vier vorherigen Jahre
- Aggregatdaten werden im RAM zwischengespeichert und nur bei geaendertem Langzeitring neu aufgebaut
- Heatmap-Aktualisierung auf 15 Sekunden reduziert, da die Daten jetzt langfristig aggregiert sind
- serielle Statuszeile zeigt zusaetzlich Belegung des Langzeitspeichers
- Firmware-Runtime-Version auf `2026-08-29-16` gesetzt

## 2026-08-29-14

- Ursache aus den Screenshots behoben: grosse Heatmap-Erweiterung wurde nicht mehr als komplette HTML-Seite im RAM des ESP32 aufgebaut
- Heatmap-Erweiterung wird jetzt als separates Skript unter `/heatmap-extension.js` ausgeliefert
- Hauptseite erhaelt nur noch einen kleinen Script-Verweis, dadurch deutlich weniger Heap-Bedarf beim Seitenaufruf
- Einstellungen-Reiter wird browserseitig erzeugt und enthaelt Sprache, Darstellung sowie Heatmap-Start- und Endstunde
- vorhandene Sprache-/Theme-Bedienung wird weiterverwendet, keine doppelte Einstellungslogik auf dem ESP32
- Wochentag/Uhrzeit-Heatmap wird browserseitig mit dem gespeicherten Bereich neu aufgebaut
- Monat/Woche und Jahr/Monat laden direkt aus `/api/events`
- sichtbare Fehlermeldung bleibt erhalten, falls die Events-API nicht gelesen werden kann
- Versionsverwaltung bereinigt: `SerialDiagnostics.ino` und `HeatmapExtension.ino` ueberschreiben die Firmware-Version nicht mehr
- zentrale Runtime-Version `2026-08-29-14` in `Version.ino`
- Serial-Diagnose zeigt den Fallback-AP weiterhin mit SSID, IP und URL

## 2026-08-29-13

- Einstellungen-Reiter erneut vereinfacht und selbststaendig aufgebaut
- kein Verschieben der bisherigen Geraete-Einstellungen mehr noetig
- Sprache, Darstellung und Heatmap-Zeitbereich werden direkt im Einstellungen-Reiter angezeigt
- Heatmap-Zeitbereich bleibt lokal gespeichert, Standard `05:00` bis `18:00`
- Heatmaps laden ihre Daten jetzt eigenstaendig direkt aus `/api/events`
- Wochentag/Uhrzeit, Monat/Woche und Jahr/Monat werden unabhaengig von der bisherigen Hauptseiten-Renderlogik aufgebaut
- bei Ladefehlern wird im jeweiligen Heatmap-Bereich eine sichtbare Fehlermeldung angezeigt
- Heatmaps werden beim Start, beim Oeffnen und alle 3 Sekunden waehrend sichtbarer Heatmap aktualisiert

## 2026-08-29-12

- Einstellungen aus der Geraeteansicht in einen eigenen Reiter verschoben
- Ursache der wirkungslosen Heatmap-Zeiteinstellung behoben
- Wochentag/Uhrzeit-Heatmap wird jetzt mit dem gespeicherten Bereich neu geladen
- Standardbereich bleibt `05:00` bis `18:00`, Endstunde inklusive
- zweite Heatmap `Monat / Woche` vor der Jahresauswertung hinzugefuegt
- Monat/Woche zeigt je Monat die Ereignisse in Woche 1 bis 5 des Monats
- dritte Heatmap `Jahr / Monat` korrigiert und direkt aus `/api/events` aufgebaut
- Jahr/Monat zeigt damit vorhandene Ereignisse wie z. B. August 2026 sofort an
- Heatmaps werden beim Oeffnen und waehrend sichtbarer Ansicht regelmaessig aktualisiert
- serielle Diagnose zeigt den Fallback-AP jetzt explizit mit Status, SSID, IP und URL
- periodische Statuszeile zeigt bei aktivem Fallback ebenfalls die AP-IP

## 2026-08-29-11

- Heatmap erweitert um konfigurierbaren Zeitbereich
- Einstellungen fuer Start- und Endstunde in der Geraeteansicht
- Standardbereich `05:00` bis `18:00`, Endstunde inklusive
- Eingabepruefung: nur 0 bis 23 Uhr und Start muss kleiner als Ende sein
- ungueltige Werte werden nicht gespeichert
- Heatmap-Zeitbereich wird lokal im Browser gespeichert und erzeugt keine zusaetzlichen ESP32-Flash-Schreibzyklen
- bestehende Wochentag/Uhrzeit-Heatmap verwendet den eingestellten Bereich
- zweite Heatmap `Jahr / Monat` hinzugefuegt
- Jahr/Monat-Heatmap zeigt die Ereignisanzahl je Monat und Jahr
- Erweiterung in separate Datei `HeatmapExtension.ino` ausgelagert, um die stabile Hauptoberflaeche moeglichst wenig anzufassen
- neue Heatmap-Texte fuer Deutsch, Schwaebisch, Englisch, Italienisch und Franzoesisch vorbereitet

## 2026-08-29-10

- serielle Diagnoseausgaben wiederhergestellt
- Diagnose bewusst in separate Datei `SerialDiagnostics.ino` ausgelagert, damit UI-Aenderungen sie nicht erneut entfernen
- Startausgabe mit Firmware-Version, Reset-Grund, Chip, CPU, Heap, Flash und NTP-Server
- Statusausgaben fuer LittleFS, normalen Ringspeicher und Autark-Ringspeicher
- Meldungen bei WLAN-Verbindung und WLAN-Verlust
- Meldungen beim Starten und Abschalten des lokalen Fallback-Hotspots
- Meldungen bei gueltiger bzw. fehlender Zeit und Anzeige der Zeitquelle
- Meldungen beim Ein- und Ausschalten des Autark-Modus
- Meldungen bei neuen Ereignissen und beim Loeschen des letzten Ereignisses
- Meldungen fuer physische Tastpulse und UI-Aktionen
- kompakte Systemstatuszeile alle 60 Sekunden mit Modus, WLAN/IP/RSSI, Zeitquelle, Speicherzaehlern und freiem Heap
- Baudrate bleibt `115200`

## 2026-08-29-9

- stabile Weboberflaeche wieder auf Basis von `2026-08-29-7` aufgebaut
- RAM-, Flash-, Ringspeicher-, Autark- und Bedienungsansichten aus `-7` beibehalten
- Datum wieder unter der Uhrzeit im Kopfbereich eingeblendet
- normaler Daten-Refresh zeigt wie Autark `Aktualisiere...`, Erfolg mit Eintragsanzahl und Uhrzeit oder eine Fehlermeldung
- Spracheinstellung als Dropdown in der Geraeteansicht
- Sprachen: Deutsch, Schwaebisch, Englisch, Italienisch und Franzoesisch
- vorhandene automatische Uebersetzungen direkt eingebaut; nicht-deutsche Fassungen werden als automatisch erstellt und ungeprueft gekennzeichnet
