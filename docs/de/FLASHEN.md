# Flashen

Diese Anleitung gilt für **ESP32 Dev Module / ESP32-WROOM-32**.

## Arduino IDE

Arduino IDE 2.x installieren.

Unter `Datei → Voreinstellungen → Zusätzliche Boardverwalter-URLs` eintragen:

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
Partition Scheme: Default 4 MB
```

## WLAN-Daten

Im Ordner `arduino/Unterbrechungszaehler/`:

1. `Secrets.example.h` nach `Secrets.h` kopieren.
2. SSID und Passwort eintragen.

`Secrets.h` ist über `.gitignore` vom Repository ausgeschlossen.

## Hochladen

Folgende Datei in der Arduino IDE öffnen:

```text
arduino/Unterbrechungszaehler/Unterbrechungszaehler.ino
```

ESP32 über ein USB-Datenkabel anschließen, den passenden Port auswählen und **Hochladen** starten.

Falls der Upload bei `Connecting...` stehen bleibt:

1. `BOOT` gedrückt halten.
2. `EN` kurz drücken und loslassen.
3. Nach etwa einer Sekunde `BOOT` loslassen.

## Serieller Monitor

Baudrate:

```text
115200
```

Nach dem Start werden unter anderem Speicher, Netzwerk, Zeitquelle und erkannte optionale Hardware ausgegeben.

## Weboberfläche

Normaler Zugriff:

```text
http://unterbrechungen.local
```

Falls mDNS nicht funktioniert, die im seriellen Monitor angezeigte IP-Adresse verwenden.

Fallback ohne normales WLAN:

```text
WLAN: Unterbrechungszaehler
URL:  http://192.168.4.1
```

## Windows-Fehler mit `bootloader.bin`

Wenn Arduino unter Windows meldet, dass `bootloader.bin` syntaktisch nicht verarbeitet werden kann, einen kurzen Pfad ohne Sonderzeichen verwenden, zum Beispiel:

```text
C:\Arduino\Unterbrechungszaehler
```

Tritt der Fehler auch mit einem leeren ESP32-Testsketch auf, sollte die lokale Arduino-/ESP32-Core-Installation geprüft werden.
