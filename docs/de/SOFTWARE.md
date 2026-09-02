# Software, Build und Flashen – 3.0.0

## Voraussetzungen

- Arduino IDE oder Arduino CLI
- Boardpaket `esp32 by Espressif Systems`
- Zielboard: **ESP32 Dev Module**

Der Sketch liegt direkt in:

`Unterbrechungszaehler/`

Die Datei `partitions.csv` liegt im selben Sketchordner und definiert zwei OTA-App-Slots plus LittleFS.

## WLAN

In `Unterbrechungszaehler/config.h` stehen absichtlich nur Platzhalter:

```cpp
WIFI_SSID
WIWI_PASSWORD
```

Diese Werte lokal ersetzen. Reale Zugangsdaten nicht committen.

## Weboberfläche bauen

Nach Änderungen an `ui-src/index.html`, `ui-src/app.css` oder `ui-src/app.js`:

```bash
python3 Unterbrechungszaehler/tools/build_web.py
```

Dadurch wird `Unterbrechungszaehler/web_assets.h` reproduzierbar neu erzeugt.

## Portable Releasechecks

```bash
python3 Unterbrechungszaehler/tools/release_check.py
```

Der Check prüft unter anderem Version, Übersetzungsparität, Projekt-API, Storage-Simulation, JavaScript-Syntax und das deterministische gzip-Webbundle.

## Kompilieren

Arduino IDE:

1. `Unterbrechungszaehler/Unterbrechungszaehler.ino` öffnen.
2. Board **ESP32 Dev Module** wählen.
3. Kompilieren.
4. Prüfen, dass das Firmwareimage in den OTA-App-Slot von 1.441.792 Byte passt.
5. Flashen.

GitHub Actions kompiliert den Release zusätzlich mit einem festgelegten ESP32-Core und veröffentlicht nur nach erfolgreichem Build die OTA-BIN.

## OTA

Version 3.0.0 verwendet eine eigene Partitionstabelle. Ein normaler OTA-Vorgang schreibt in die inaktive App-Partition und soll NVS/LittleFS nicht löschen.

Da 3.0.0 ein harter Schnitt gegenüber den alten Testständen ist, wird **kein direktes OTA-Upgrade von 2.x zugesichert**.

## Nach dem Flash

- Serial Monitor: 115200 Baud
- DI1/GPIO13 einmal drücken
- Home-Zähler prüfen
- OLED-Feedback prüfen
- Sound prüfen
- RTC/NTP prüfen
- CSV und Heatmaps mit echten Events testen

Die noch erforderlichen Zielhardwaretests stehen in `Unterbrechungszaehler/TEST_REPORT.md`.
