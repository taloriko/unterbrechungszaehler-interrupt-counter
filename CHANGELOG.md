# Changelog

## 2.1.1

Flash-/OTA-Fix ohne Funktionsabbau. Die Weboberflaeche bleibt inhaltlich unveraendert, wird aber nicht mehr als rund 150 kB Klartext in die Firmware gelinkt.

### Flash / OTA

- Weboberflaeche wird deterministisch mit gzip komprimiert und als `WebUiGzip.h` in PROGMEM eingebettet
- Browser erhalten die Seite mit `Content-Encoding: gzip` und entpacken sie automatisch
- doppelte Auslieferung von `WEB_UI_V2` entfernt
- editierbare `WebUi*.h`-Quelldateien bleiben erhalten; `tools/generate_webui_gzip.py` erzeugt daraus die komprimierte Fassung
- ESP32-Arduino-Core im CI auf `3.3.11` festgesetzt, damit Builds reproduzierbar bleiben
- CI bricht einen Release ab, wenn weniger als 64 KiB Programmspeicher-Reserve verbleiben
- Versionsstand auf `2.1.1` angehoben

### Kompatibilitaet

- keine Aenderung an Pinbelegung, LittleFS-Daten, Einstellungen, API oder Bedienung
- bestehende OTA-Partitionierung bleibt unveraendert
- 2.1.1 ist als direktes OTA-Update fuer bestehende kompatible 2.x-Installationen vorgesehen

## 2.1.0

Stabilitäts- und Diagnose-Release auf Basis des vollständigen 2.0.0-Stands. Schwerpunkt sind echte Hardwarezustände, eine belastbare DY-SV17F-Ansteuerung und ein reproduzierbarer OTA-Releaseprozess.

### Fehlerbehebungen

- DY-SV17F verwendet für normale Titelwiedergabe jetzt den vorgesehenen UART-Befehl `0x07` statt des Interlude-Befehls `0x16`
- Soundhardware gilt nicht mehr nach einer einzelnen Antwort als erkannt, sondern erst nach drei passenden Antworten auf aktiv gesendete Statusabfragen
- offene bzw. störbehaftete UART-Eingänge können dadurch wesentlich schwerer fälschlich als vorhandenes Soundmodul erscheinen
- Wiedergaben besitzen einen eigenen Zustandsautomaten: Befehl gesendet → `PLAY` bestätigt → `STOP` bestätigt
- Erfolgszähler steigt ausschließlich nach einer vollständig bestätigten Wiedergabe
- Start- und Wiedergabe-Timeouts lösen festhängende Soundvorgänge wieder auf und werden getrennt gezählt
- Verlust des Soundmoduls während einer Wiedergabe wird als Fehler erfasst und blockiert keine weiteren Systemmodule

### Diagnose / Watchdog

- Watchdog trennt jetzt die reine Laufzeitüberwachung eines Codepfads vom tatsächlichen fachlichen Modulzustand
- Modulzustände: `disabled`, `initializing`, `ready`, `busy`, `not_detected`, `degraded`, `error`, `timeout`
- RTC, Display und Sound können dadurch korrekt als optional/nicht erkannt erscheinen, ohne den gesamten ESP32 als fehlerhaft darzustellen
- Netzwerk und Webserver werden im Autarkmodus als bewusst deaktiviert statt als ausgefallen markiert
- Fallback-AP, ungültige Zeit und teilweise verfügbare Speicherbereiche erscheinen als eingeschränkter Betrieb
- Moduldiagnose im Web zeigt zusätzlich Zustandsdetail, Laufzeit, Maximum, langsame Aufrufe und Fehlerzähler

### Weboberfläche

- Sounddiagnose zeigt gesendete, erfolgreich abgeschlossene, fehlgeschlagene und per Timeout beendete Wiedergaben getrennt
- Hardwareprüfung des DY-SV17F wird während der Mehrfacherkennung sichtbar dargestellt
- Watchdog-Tabelle unterscheidet bereit, beschäftigt, deaktiviert, nicht erkannt, eingeschränkt, Fehler und Timeout
- Trackliste, Soundeinstellungen, Diagnose und OTA bleiben vollständig in der normalen Weboberfläche auf Port 80 integriert

### Wartbarkeit

- nicht mehr verwendeter separater Add-on-Webserver auf Port 81 entfernt
- zentrale Modulzustände in `ModuleStatus.h` vereinheitlicht
- Soundprotokoll und Fehlerbehandlung vollständig in `SoundService` gekapselt
- bestehende Pinbelegung, Speicherformate, Bedienung und Web-API bleiben soweit möglich kompatibel

### Build / Release

- Build-Artefakt übernimmt die Versionsnummer automatisch aus `Config.h`
- keine hart codierte `2.0.0`-Dateibezeichnung mehr im GitHub-Actions-Build
- erfolgreicher Build auf `main` erzeugt automatisch den GitHub-Release `v2.1.0`
- die geprüfte OTA-BIN wird dem Release direkt als Datei angehängt

### Hardwaretest erforderlich

- tatsächliche UART-Rückmeldungen und Zeitverhalten des konkreten DY-SV17F
- Wiedergabe der aufgespielten Tracks und korrekte Reihenfolge des Dateisystems
- OTA-Update auf realer ESP32-WROOM-32D-Hardware
- RTC/Display-Erkennung mit tatsächlich angeschlossener bzw. abgezogener Hardware

## 2.0.0

Teststand auf Basis des aktuellen `main`-Zweigs 1.0.1. Die bestehenden UI- und Sprachkorrekturen bleiben damit vollständig erhalten.

### Neue Funktionen

- optionales DY-SV17F-Soundmodul über UART2
- Soundmodul gilt erst nach einer gültigen UART-Statusantwort als vorhanden
- getrennte Zähler für gesendete Wiedergaben und vollständig bestätigte Wiedergaben
- eine Wiedergabe zählt erst als bestätigt, wenn der Player zunächst `PLAY` und anschließend `STOP` zurückmeldet
- persistente Trackliste mit Tracknummer und frei wählbarer Beschreibung
- OTA-Firmwareupdate über die lokale Add-on-Webseite
- ESP32 Task-Watchdog mit 12 Sekunden Timeout
- zusätzliche Laufzeitüberwachung für Input, Zeit, Autark, Display, LED, Netzwerk, Web und Sound
- Diagnose zeigt je Modul letzten Durchlauf, maximal gemessene Laufzeit und Anzahl auffällig langsamer Durchläufe
- CI erzeugt eine direkt für OTA geeignete Firmware-BIN als Build-Artefakt

### Hardware

- DY-SV17F RX des ESP32: GPIO 16
- DY-SV17F TX des ESP32: GPIO 17
- UART: 9600 Baud, 8N1
- Soundmodul und Lautsprecher bleiben vollständig optional

### Bedienung

- die normale Weboberfläche auf Port 80 entspricht weiterhin dem aktuellen Stand aus `main`
- Sound, Trackliste, Watchdog-Diagnose und OTA befinden sich im Teststand zunächst getrennt auf Port 81

## 1.0.1

Reine Fehlerbehebungs- und Optimierungsversion ohne neue Funktionen.

### Fehlerbehebungen

- flackernde bzw. zuckende Texte bei Ringspeicher- und Geräteanzeigen behoben
- problematischer nativer Sprachdialog auf Smartphones durch eine stabile Button-Auswahl ersetzt
- Absturz bei aktivem Schwäbisch durch Entfernen der rekursiven Live-Übersetzung behoben
- Deutsch, Englisch und Schwäbisch verwenden denselben Sprach- und Renderpfad
- Schwäbisch liegt wie Deutsch und Englisch direkt im zentralen I18N-Sprachsatz
- UTF-8-Sonderzeichen werden ohne sprachspezifische Sonderlogik verarbeitet
- springende Symbole und verschobene Buttons bei der Darstellungswahl behoben
- alte Darstellungs-Rückmeldungen werden beim nächsten Klick entfernt
- unnötige Mehrfachaktualisierung derselben Statuswerte entfernt
- Sprach- und UI-Nachbearbeitung wird nicht mehr bei jeder Textänderung der gesamten Seite ausgeführt

### Optimierungen

- alle drei Sprachen besitzen denselben geprüften Schlüsselsatz
- Sprachwechsel verwendet für Deutsch, Englisch und Schwäbisch dieselbe `tr()`-Funktion und dieselben Renderfunktionen
- DOM-Werte werden nur noch geschrieben, wenn sich ihr sichtbarer Inhalt wirklich geändert hat
- RAM-, Flash- und LittleFS-Zusatzanzeigen werden langsamer aktualisiert
- numerische Geräte- und Speicheranzeigen verwenden eine stabilere Zifferndarstellung
- der interne Select wird nur noch programmintern genutzt; sichtbar sind stabile Sprachbuttons

## 1.0.0

Erste öffentliche Release-Version.

### Funktionen

- Ereigniserfassung über Taster oder potentialfreien Kontakt
- normaler Ringspeicher mit 10.000 Ereignissen
- Langzeit-Ringspeicher mit 100.000 Ereignissen
- separater Autark-Ringspeicher
- lokale Weboberfläche ohne Cloud-Abhängigkeit
- Tagesansicht, Verlauf, Details und Heatmaps
- CSV-Export
- NTP-Zeitverwaltung mit Browser-Fallback
- optionale DS3231-RTC
- optionales SH1106-OLED 128 × 64
- Fallback-Access-Point bei fehlendem WLAN
- Autarkbetrieb mit reduziertem CPU-Takt und Light-Sleep
- serielle Diagnose mit 115200 Baud
- Weboberfläche in Deutsch, Englisch und Schwäbisch

### Architektur

- modular getrennte Services für Eingänge, Speicherung, Zeit, Netzwerk, Display, RTC, Analyse und Weboberfläche
- blockweiser Zugriff auf große Ringspeicher
- persistenter Statistik-Cache für Heatmaps
- optionale Hardware wird beim Start erkannt
- zentrale Sprachverwaltung mit gemeinsamem Schlüsselsatz
