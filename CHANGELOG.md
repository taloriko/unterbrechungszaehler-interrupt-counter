# Changelog

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
- statische Beschriftungen und viele dynamische Rueckmeldungen ueber gemeinsame Sprachschluessel angebunden
- Darkmode mit `System`, Sonne und Mond
- `System` folgt automatisch der Hell-/Dunkel-Einstellung des verwendeten Endgeraets
- manuelle Theme- und Sprachwahl werden lokal im Browser gespeichert
- separate bearbeitbare Uebersetzungsliste unter `translations/ui-translations.csv` bleibt Grundlage fuer spaetere manuelle Korrekturen

## 2026-08-29-8 – zurueckgezogen

- Sprach- und Theme-Umbau wurde nach Vergleich mit `2026-08-29-7` wieder entfernt
- Grund: beim Umbau gingen Teile der bestehenden Oberflaeche verloren, unter anderem RAM-/Flash-Anzeige und Teile der Bedienungsansicht
- Firmware wurde deshalb vollstaendig auf den stabilen Stand `2026-08-29-7` zurueckgesetzt
- neue Sprachtexte werden ab jetzt getrennt in `translations/ui-translations.csv` gepflegt und erst nach manueller Pruefung wieder eingebaut
- die Uebersetzungsliste enthaelt auch dynamische Rueckmeldungen und Fehlermeldungen, nicht nur statische Beschriftungen

## 2026-08-29-7

- Versionsnummer auf den fortlaufenden Stand `2026-08-29-7` korrigiert
- Datum aus dem Kopfbereich der Weboberflaeche entfernt
- Ueberschrift mittig und zurueckhaltender gestaltet
- kleines Icon direkt an der mittigen Ueberschrift
- Geraetezeit links im Kopfbereich angeordnet
- WLAN-/Lokaler-Verbindungsstatus rechts im Kopfbereich angeordnet
- Autark-Ansicht: `Neu laden` zeigt jetzt sichtbar `Aktualisiere...`, Erfolg mit Anzahl der geladenen Eintraege und Uhrzeit oder eine Fehlermeldung

## 2026-08-29-1

- Start von WLAN und NTP nicht mehr blockierend
- lokaler offener Fallback-Hotspot `Unterbrechungszaehler` hinzugefuegt
- feste lokale Adresse `http://192.168.4.1` waehrend das normale WLAN noch nicht verbunden ist
- Fallback-Hotspot wird nach erfolgreicher normaler WLAN-Verbindung automatisch abgeschaltet
- Weboberflaeche kann bei fehlender NTP-Zeit einmalig die Zeit des verbundenen Handys/Browsers an den ESP32 uebergeben
- Browserzeit wird nur akzeptiert, solange noch keine gueltige Geraetezeit vorhanden ist
- Normalbetrieb bleibt strikt: ohne gueltige absolute Zeit werden keine Ereignisse gespeichert
- Geraeteansicht um Zeitquelle und Status des lokalen Fallback-Zugangs erweitert

## 2026-08-28-6

- finales GitHub-Repository verlinkt
- Projekt-Link unten rechts in der Weboberflaeche aktiviert
- README auf das bestehende Repository angepasst

## 2026-08-28-5

- primären NTP-Server auf der Geräte-Seite frei konfigurierbar gemacht
- NTP-Server wird vor dem Speichern per DNS und echter UDP/NTP-Antwort geprüft
- NTP-Einstellung wird dauerhaft im ESP32-NVS gespeichert
- Cloudflare und Google bleiben als Fallback-NTP aktiv
- Flash-Dokumentation um Windows-Fehler `bootloader.bin ... syntaktisch` ergänzt

## 2026-08-28-4

- Versionsschema angepasst: fortlaufende Nummer ohne fuehrende Nullen
- GitHub-Profil `taloriko` unten links in der Weboberflaeche verlinkt
- Platzhalter fuer spaeteren Projekt-Link in der Weboberflaeche vorbereitet
- README vollstaendig Deutsch/Englisch ueberarbeitet
- deutlicher KI-Hinweis am Anfang der README
- Entstehungsgeschichte mit bewusst leicht sarkastischer Einleitung ergaenzt
- technische Funktionen und Ablauf anschliessend sachlich beschrieben
- Screenshot-Platzhalter fuer Hauptansicht, Auswertung und Geraet/Autark hinzugefuegt
- Anleitung zum Anlegen und Pushen des GitHub-Repositories ergaenzt
- deutsche und englische GitHub-Beschreibung ergaenzt

## 2026-08-28-3

- virtueller Druck quittiert die Karten ebenfalls blau
- Loeschen quittiert die Karten rot
- Flash und freier RAM im Geraete-Reiter als Balken
- neuer Reiter Autark BETA
- GPIO33 als Schiebeschalter fuer Autarken Modus
- separater Ringspeicher fuer 10.000 Autark-Datensaetze
- Autark-Betrieb ohne gueltige NTP-Zeit ueber relative Laufzeit
- WLAN, mDNS und Webserver im Autarken Modus abgeschaltet
- CPU im Autarken Modus auf 80 MHz reduziert
- Light-Sleep zwischen Eingaben im Autarken Modus

## 2026-08-28-2

- stabilerer Reiterwechsel ohne horizontales Springen
- Aktionsknopfe direkt an der Tageszahl
- Bedienungsbereich auf reine Anleitung reduziert

## 2026-08-28-1

- Export-Dateiname mit Datum und Uhrzeit
- neuer Footer mit Version
- Icons und erweiterte Geraeteinformationen
- Hardware-Tastendruck laesst Karten kurz blau aufleuchten