# Changelog

## 3.2.0

- OLED-Inhalte folgen der persistent synchronisierten UI-Sprache; kompakte OLED-Transliteration für Umlaute und Akzente
- SH1106-Bootscreen auf mindestens 4 Sekunden verlängert, weiterhin nicht blockierend
- persistente 180°-Displaydrehung ergänzt
- frische Helligkeitsdefaults auf 65 % normal und 5 % gedimmt gesetzt
- Geräteeinstellungen nach Display, Display-Feedback und DY-SV17F-Sound gruppiert
- DY-SV17F-Lautstärke von 0–100 %, Standard 100 %, zentral auf den Modulbereich 0…30 abgebildet
- Wechsel-/Rotationsmodus ist bei einer frischen Konfiguration der neue Tonstandard; Track 1 bleibt Boot vorbehalten
- OLED-Modi **Tagesfortschritt** und **Fokus** ergänzt
- Arduino-IDE-Hinweis zum Erzeugen einer Sketch-BIN vollständig aus dem OTA-UI entfernt; Releases liefern die fertige OTA-BIN
- Webbundle, Sprachchecks und Releasechecks auf 3.2.0 erweitert

## 3.1.0

- Heatmaps zwischen **Anzahl** und **Ø Abstand bis zur nächsten Unterbrechung** umschaltbar
- Ø-Abstand ausschließlich aus gültigen, unmittelbar aufeinanderfolgenden retained Rohereignissen desselben lokalen Tages
- letzter Druck eines Tages und Übergänge über Mitternacht bewusst aus der Durchschnittsberechnung ausgeschlossen
- Samplezahl und Raw-Ring-Coverage für die Ø-Abstandsansicht; kurze Abstände werden in der Heatmap stärker gewichtet
- persistenter Display-Master-Schalter analog zur Soundeinstellung
- SH1106-Bootbild mindestens 2 Sekunden sichtbar, nicht blockierend; Display-Aus wird danach respektiert
- manueller Displaytest kehrt zuverlässig zur Benutzeranzeige bzw. zu Display-Aus zurück
- DY-SV17F-Micro-USB-/Root-Verzeichnis-/Dateibenennungsdokumentation ergänzt
- Sound-Startpaket unter `docs/sounds/` dokumentiert
- Hosttests auf 16 Szenarien erweitert; Webbundle und Releasechecks für 3.1.0 aktualisiert

## 3.0.1

Patch-Release für die OTA-Oberfläche und den Fallback-Access-Point. Die 3.0.0-Baseline bleibt unverändert; 3.0.1 korrigiert und verbessert ausschließlich den aktuellen 3.x-Stand.

### Änderungen

- Fallback-AP ist mit dem festen Passwort `Unterbrechungszähler` geschützt
- irreführender Hinweis auf einen offenen Fallback-AP vollständig aus der OTA-Oberfläche und allen UI-Sprachpaketen entfernt
- statischer Hinweis auf eine fehlende zweite OTA-App-Partition aus der normalen OTA-Karte entfernt; echte OTA-Fehlerdiagnose bei einem tatsächlichen Partitionsfehler bleibt erhalten
- OTA-Speicherwerte bleiben als Byte-Angaben sichtbar und werden zusätzlich als segmentierter Auslastungsbalken dargestellt
- Geräte-API liefert dafür `ota.usedPercent`
- Releasechecks prüfen AP-Schutz, entfernte Alt-Hinweise und den OTA-Auslastungsbalken
- Dokumentation für Deutsch, Englisch und Schwäbisch auf den passwortgeschützten Fallback-AP und den aktuellen Stand 3.0.1 aktualisiert

## 3.0.0

Version 3.0.0 ist der neue stabile Ausgangspunkt des Unterbrechungszählers. Frühere 1.x/2.x-Releases waren Entwicklungs- und Teststände; für sie wird keine Migration oder Hardwarekompatibilität zugesichert.

### Highlights

- vollständig neue modulare Firmwarestruktur für den ESP32
- lokaler Unterbrechungszähler per GPIO13/DI1 und Webbutton
- binärer Ringspeicher für 100.000 Rohereignisse
- separater Tagesaggregatring für 2.300 Tage
- drei Heatmap-Auswertungen sowie gestreamter CSV-Export
- persistente Sound- und Displayeinstellungen
- lokale Weboberfläche ohne Cloud-Abhängigkeit

### Hardware

- Unterbrechungstaster: GPIO13 gegen GND, `INPUT_PULLUP`, active-low
- DS3231 und SH1106 gemeinsam auf I2C GPIO21/22
- DY-SV17F auf UART2: RX GPIO18, TX GPIO19
- DY-SV17F CON3/BUSY auf GPIO39/VN mit externem ca. 10-kΩ-Pull-up an V33
- CON1 und CON2 des DY-SV17F für UART-Modus auf GND
- eigene 4-MiB-Partitionstabelle mit zwei OTA-App-Slots und LittleFS

### Weboberfläche

- Home mit Tageszähler, letzter Unterbrechung und großem Erfassungsbutton
- Feedback-/Displaykarte mit persistenten Geräteeinstellungen
- Auswertung mit Wochentag/Stunde, Monat/Kalenderwoche und 5-Jahres-Monatsansicht
- responsive Heatmaps ohne externe Chartbibliothek
- Live-Aktualisierung nur bei sichtbarem Home/Auswertung
- UI in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch
- README-Dokumentation in Deutsch, Englisch und Schwäbisch

### Sound

- DY-SV17F-Hardwareerkennung und BUSY-Rückmeldung
- Track 1 ausschließlich als Boot-Ton
- Unterbrechungston als fester Track ab 2 oder rotierend über erkannte Tracks 2…N
- Sound kann unabhängig von der Ereignisspeicherung deaktiviert werden

### Speicherung / Daten

- 100.000 Raw-Slots à 9 Byte mit CRC
- 2.300 Tagesrecords à 64 Byte
- transaktionale Metadaten mit Recoverypfaden
- feste 64er Pending-Queue ohne Heap-Allokation pro Ereignis
- CSV wird erst beim Download erzeugt und in kleinen Chunks gestreamt
- lokale Kalenderauswertung für Europe/Berlin; absolute Zeit bleibt UTC

### Zeit / Betrieb

- NTP als bevorzugte Zeitquelle
- DS3231 als Offline-/Startfallback
- Browserzeit als zusätzlicher Fallback
- Ereignisse ohne gültige absolute Zeit bleiben ausdrücklich relativ statt eine falsche Kalenderzeit zu erhalten

### Breaking Changes

- keine zugesicherte Hardwarekompatibilität zu 2.x
- keine zugesicherte Daten-/NVS-/LittleFS-Migration aus 2.x
- kein zugesichertes direktes OTA-Upgrade von 2.x
- 3.0.0 nach der aktuellen Hardwaredokumentation neu verdrahten und als neuen Ausgangspunkt behandeln

### Build / Release

- portable Releasechecks prüfen Storage, i18n, JavaScript und Webbundle
- GitHub Actions kompiliert mit festgelegtem Arduino-ESP32-Core
- Release wird nur nach erfolgreichem Build auf `main` erzeugt
- OTA-BIN wird automatisch an den GitHub-Release angehängt

### Noch auf realer Hardware zu bestätigen

- physischer Taster und Debounce
- reale LittleFS-Persistenz und Recovery
- OTA auf dem Zielboard
- DS3231 und SH1106
- DY-SV17F Boot-/Unterbrechungstöne und BUSY-Signal
- Offlinebetrieb und Langzeittest
