# Flashen

> **Status: In Arbeit**

Diese Anleitung gilt für das verwendete klassische **ESP32 Dev Module / ESP32-WROOM-32**.

## Arduino IDE

Arduino IDE 2.x installieren.

Unter `Datei -> Voreinstellungen -> Zusätzliche Boardverwalter-URLs` eintragen:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Im Boardverwalter anschließend **esp32 by Espressif Systems** installieren.

## Board-Einstellungen

```text
Board:            ESP32 Dev Module
Upload Speed:     115200
CPU Frequency:    240 MHz
Flash Frequency:  80 MHz
Flash Mode:       QIO
Flash Size:       4 MB
Partition Scheme: Default 4MB with spiffs
```

## WLAN-Daten

Im Sketch-Ordner `Secrets.example.h` nach `Secrets.h` kopieren und SSID/Passwort eintragen. `Secrets.h` wird durch `.gitignore` nicht ins Repository aufgenommen.

## Anschließen und hochladen

ESP32 mit einem USB-Datenkabel anschließen und den passenden COM-Port in Arduino auswählen.

`arduino/UnterbrechungszaehlerInterruptCounter/UnterbrechungszaehlerInterruptCounter.ino` öffnen und **Hochladen** wählen.

Falls der Upload bei `Connecting...` stehen bleibt:

1. `BOOT` gedrückt halten.
2. `EN` kurz drücken und loslassen.
3. Nach etwa einer Sekunde `BOOT` loslassen.

## Serieller Monitor

Baudrate: **115200**.

Nach dem Start sollten LittleFS, Ringspeicher, WLAN, NTP und Webserver als bereit gemeldet werden.

## Weboberfläche

Nach erfolgreichem Start:

```text
http://unterbrechungen.local
```

Falls mDNS nicht funktioniert, die im seriellen Monitor angezeigte IP-Adresse verwenden.

## Fehler: `bootloader.bin` kann syntaktisch nicht verarbeitet werden

Diese Meldung kann unter Windows durch problematische Sonderzeichen im vollständigen Sketch- oder Benutzerpfad entstehen.

Zum Gegencheck den Sketch in einen einfachen Pfad kopieren, zum Beispiel:

```text
C:\Arduino\InterruptCounter\UnterbrechungszaehlerInterruptCounter
```

Möglichst keine Klammern, `&`, Umlaute oder andere Sonderzeichen im vollständigen Pfad verwenden.

Falls auch ein leerer ESP32-Testsketch mit diesem Fehler abbricht, liegt das Problem wahrscheinlich an der lokalen Arduino-/ESP32-Core-Installation.
