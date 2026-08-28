# Flash-Anleitung ESP32

Diese Anleitung gilt für das verwendete klassische **ESP32 Dev Module / ESP32-WROOM-32**.

## 1. Arduino IDE

Arduino IDE 2.x installieren.

Unter `Datei -> Voreinstellungen -> Zusätzliche Boardverwalter-URLs` eintragen:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Im Boardverwalter anschließend **esp32 by Espressif Systems** installieren.

## 2. Board-Einstellungen

```text
Board:            ESP32 Dev Module
Upload Speed:     115200
CPU Frequency:    240 MHz
Flash Frequency:  80 MHz
Flash Mode:       QIO
Flash Size:       4 MB
Partition Scheme: Default 4MB with spiffs
```

LittleFS kann die Dateisystem-Partition dieses Partitionsschemas verwenden.

## 3. WLAN-Daten

Im Sketch-Ordner `Secrets.example.h` nach `Secrets.h` kopieren und SSID/Passwort eintragen. `Secrets.h` wird durch `.gitignore` nicht ins Repository aufgenommen.

## 4. Anschließen

ESP32 mit einem USB-Datenkabel anschließen und den passenden COM-Port in Arduino auswählen.

## 5. Hochladen

`arduino/UnterbrechungszaehlerInterruptCounter/UnterbrechungszaehlerInterruptCounter.ino` öffnen und **Hochladen** wählen.

Falls der Upload bei `Connecting...` stehen bleibt:

1. `BOOT` gedrückt halten.
2. `EN` kurz drücken und loslassen.
3. Nach etwa einer Sekunde `BOOT` loslassen.

## 6. Serieller Monitor

Baudrate: **115200**.

Nach dem Start sollten LittleFS, Ringspeicher, WLAN, NTP und Webserver als bereit gemeldet werden.

## 7. Taster

```text
GPIO27 ---- potentialfreier Taster ---- GND
```

Keine Fremdspannung an GPIO27 anlegen. Zum Testen GPIO27 kurz mit GND brücken.

## 8. Weboberfläche

```text
http://unterbrechungen.local
```

Falls mDNS nicht funktioniert, die im seriellen Monitor angezeigte IP-Adresse verwenden.

## LED-Codes

- 1x kurz: Ereignis gespeichert
- 3x schnell: letzter Eintrag erfolgreich gelöscht
- 2x schnell + 2x langsam: keine gültige Zeit, kein WLAN oder Speicherfehler

Bei ausgefallenem WLAN wird ein Ereignis weiterhin lokal gespeichert, sofern die ESP32-Uhrzeit bereits gültig ist.

## Optional: Autarker Modus (Beta)

Fuer den Autarken Modus einen potentialfreien Schiebeschalter anschliessen:

```text
GPIO33 ---- Schiebeschalter ---- GND
```

- offen = normaler Netz-/WLAN-Betrieb
- geschlossen = Autarker Modus

Keine externe Spannung auf GPIO33 geben. Der Eingang verwendet `INPUT_PULLUP`.

## Primären NTP-Server einstellen

Nach dem Flashen im Reiter **Gerät** unter **WLAN & NTP** den gewünschten Server eintragen und **Prüfen & speichern** wählen. Der ESP32 prüft kurz DNS und eine echte NTP-Antwort. Bei Erfolg wird die Einstellung dauerhaft gespeichert.

## Fehler: `bootloader.bin` kann syntaktisch nicht verarbeitet werden

Diese Meldung entsteht unter Windows in der ESP32-Arduino-Buildkette und nicht durch eine im Projekt enthaltene `bootloader.bin`. Besonders problematisch sind Sonderzeichen wie `&` oder Klammern in einem Ordner des vollständigen Sketch-/Benutzerpfads.

Zum Gegencheck den Sketch in einen sehr einfachen Pfad kopieren, zum Beispiel:

```text
C:\Arduino\InterruptCounter\UnterbrechungszaehlerInterruptCounter
```

Dort dürfen im gesamten Pfad möglichst keine Klammern, `&`, Umlaute oder sonstige Sonderzeichen vorkommen. Anschließend erneut mit **ESP32 Dev Module** kompilieren.

Falls der Fehler auch dort auftritt, einen komplett leeren ESP32-Testsketch kompilieren. Schlägt auch dieser mit `bootloader.bin` fehl, liegt es an der lokalen Arduino-/ESP32-Core-Installation und nicht an diesem Projekt. Dann das Paket **esp32 by Espressif Systems** im Boardverwalter entfernen und erneut installieren.
