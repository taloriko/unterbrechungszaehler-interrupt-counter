# Release Notes – Unterbrechungszähler 0.2.0

Erster projektspezifischer Stand auf **ESP32 UI Base FINAL 1.6.0**.

## Enthalten

- physischer Unterbrechungstaster auf DI1 / GPIO13
- großer Web-Unterbrechungsbutton
- unmittelbarer RAM-Home-Zähler und letzte Unterbrechung
- Zeitstempel mit TimeSource aus dem zentralen TimeService
- Europe/Berlin-Tages-/Stunden-/ISO-KW-Zuordnung
- Abstand zum vorherigen Ereignis desselben lokalen Tages
- LittleFS-Raw-Ringspeicher für 100.000 Events
- transaktionale Metadaten und Recoverypfade
- 2.300 Tagesaggregate für langfristige Statistiken
- CSV-Streamingdownload
- Heatmap Wochentag × Stunde
- Heatmap Monat × Kalenderwoche
- Heatmap letzte fünf Jahre × Monate
- sichtbare aktuelle Zeile/Spalte als Orientierung
- DY-SV17F-Unterbrechungston, separat abschaltbar
- SH1106-Projektansicht mit Heute-Zähler, letzter Unterbrechung, Zeit-/WLANstatus und kurzem Ereignisflash
- 64er feste Pending-Persistenzqueue
- DI1 Interrupt-Latch für menschliche Tastendrücke während längerer synchroner HTTP-Ausgaben
- kombinierter Analytics-Endpunkt: normale Auswertungsansicht mit einem HTTP-Request und einem Tagesring-Durchlauf
- eigene 4-MiB-Partitionstabelle mit zwei OTA-Slots und 1,1875 MiB LittleFS

## Validierung

- 14/14 Storage-/Recovery-/Heatmap-Hosttests: PASS
- 100.000er Ring-Wrap: PASS
- 305 identische i18n-Schlüssel je Sprache: PASS
- JavaScript-Syntax: PASS
- Webbundle/ETag/gzip: PASS
- 24 C++-/INO-Einheiten mit strengen Warnungen als Fehler: PASS gegen lokale API-Teststubs

## Noch auf realem Zielboard zu bestätigen

- Build/Link gegen den tatsächlich installierten Arduino-ESP32-Core
- Firmwaregröße < 1.441.792 Byte pro OTA-App-Slot
- Upload und Custom-Partition
- reale LittleFS-Persistenz über Neustart/OTA
- physischer Taster, OLED und DY-SV17F im Dauerbetrieb

Details: `TEST_REPORT.md`, `PROJECT_ARCHITECTURE.md`, `STORAGE_FORMAT.md`, `HARDWARE_WIRING.md`.


## 0.2.0 Änderungen

- Kalenderwochen-Heatmap: Spaltenköpfe nur noch `1…53`, ohne wiederholtes `KW`.
- Filterrefresh repariert: Jahr, KW und Von/Bis aktualisieren die betroffene Heatmap unmittelbar.
- Neue Home-Kachel **Feedback & Display**.
- Display-Flash bei Unterbrechung separat ein-/ausschaltbar.
- OLED-Modi: Standard, nur heutige Zahl, nur letzte Unterbrechung.
- OLED-Helligkeit, Dimmer-Timeout und gedimmte Helligkeit persistent konfigurierbar.
- Unterbrechungston: fester Track ab 2 oder **Wechselnd** über Track 2…N. Track 1 bleibt Boot-Ton.
- Alle neuen Geräteeinstellungen werden sofort angewendet und in NVS gespeichert.
