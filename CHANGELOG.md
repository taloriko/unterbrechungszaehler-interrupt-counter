# Changelog

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
- UI in Deutsch, Englisch und Schwäbisch

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
