# Changelog

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
