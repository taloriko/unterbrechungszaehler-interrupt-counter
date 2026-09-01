# Software

Die Firmware befindet sich unter:

```text
arduino/Unterbrechungszaehler/
```

Hauptdatei:

```text
Unterbrechungszaehler.ino
```

## Konfiguration

Allgemeine Hardware- und Speicherparameter liegen in `Config.h`.

Für die WLAN-Zugangsdaten:

1. `Secrets.example.h` nach `Secrets.h` kopieren.
2. SSID und Passwort in `Secrets.h` eintragen.
3. `Secrets.h` nicht veröffentlichen; die Datei ist über `.gitignore` ausgeschlossen.

## Zeitquellen

Die Firmware verwendet folgende Priorität:

1. NTP
2. optionale DS3231-RTC
3. Browserzeit als einmaliger Fallback
4. relative Session-Zeit im Autarkbetrieb

Normale Ereignisse werden nur mit gültiger absoluter Zeit gespeichert.

## Speicherung

- normaler Ringspeicher: 10.000 Ereignisse
- Langzeit-Ringspeicher: 100.000 Ereignisse
- Autark-Ringspeicher: 10.000 Datensätze
- Statistik-Cache für Heatmaps

Die Daten liegen im LittleFS des ESP32. Große Datenmengen werden blockweise verarbeitet.

## Netzwerk

Normaler Zugriff:

```text
http://unterbrechungen.local
```

Wenn das konfigurierte WLAN nicht erreichbar ist, stellt der ESP32 einen lokalen Access Point bereit:

```text
SSID: Unterbrechungszaehler
IP:   192.168.4.1
URL:  http://192.168.4.1
```

Nach erfolgreicher Verbindung mit dem normalen WLAN wird der Fallback-Access-Point abgeschaltet.

## Optionale Hardware

- DS3231 RTC: `0x68`
- SH1106 OLED 128 × 64: `0x3C` oder `0x3D`

Beide Module werden beim Start erkannt. Fehlende optionale Hardware verhindert die grundlegende Ereigniserfassung nicht.

## Weboberfläche

Die Weboberfläche stellt Tagesansicht, Verlauf, Detailansicht, Heatmaps, Exporte, Gerätestatus und Einstellungen bereit.

Sprachpakete werden zentral in `WebUiBehavior.h` registriert. Alle registrierten Sprachen verwenden denselben Schlüsselsatz und werden durch `tools/check_translations.py` geprüft.

## Architektur

Die technische Modulstruktur ist unter [Software-Architektur](SOFTWARE-ARCHITEKTUR.md) beschrieben.
