# Unterbrechungszähler / Interrupt Counter

[🇬🇧 English documentation](docs/en/README.md)

## Funktion

Der Unterbrechungszähler ist ein ESP32-basiertes Modul zum Erfassen, Speichern und Auswerten von Ereignissen.

**Taster oder potentialfreien Kontakt betätigen → Ereignis speichern → lokal auswerten.**

Der Eingang kann neben einem Taster auch mit Relais-, Tür-, Störmelde-, Betriebs- oder Maschinenkontakten verwendet werden.

## Funktionen

- Ereigniserfassung per physischem oder virtuellem Taster
- Datum und Uhrzeit automatisch per NTP
- optionale **DS3231 RTC** als lokale Zeitquelle
- optionales **1,3 Zoll SH1106 OLED 128 × 64**
- automatische Hardware-Erkennung beim Boot
- lokale Weboberfläche auf dem ESP32
- Tagesübersicht, Verlauf und Detailansicht
- Heatmap Wochentag / Uhrzeit mit Auswahl von Jahr, Kalenderwoche und Zeitbereich
- Heatmap Monat / Kalenderwoche mit Jahresauswahl
- Heatmap Jahr / Monat
- CSV-Export
- Hell-/Dunkelmodus und Deutsch/Englisch
- Fallback-WLAN bei nicht erreichbarem normalen WLAN
- serielle Diagnose mit `115200 Baud`
- Autark-Modus für Akku-/Powerbank-Betrieb

## Zeitstrategie

Die Zeitquellen werden in folgender Reihenfolge verwendet:

1. **NTP** – Referenzzeit bei verfügbarer Netzwerkverbindung
2. **RTC** – lokale Startzeit und Zeitquelle ohne NTP
3. **Browserzeit** – Fallback, solange das Gerät noch keine gültige Zeit besitzt
4. **relative Zeit** – Autark-Sessions ohne absolute Zeitbasis

Eine gültige RTC kann beim Start die Systemzeit setzen. Nach einer erfolgreichen NTP-Synchronisation wird die RTC mit der Systemzeit aktualisiert.

## Optionale Hardware

RTC und OLED verwenden gemeinsam den I2C-Bus:

- `GPIO21` = SDA
- `GPIO22` = SCL
- Versorgung mit `3,3 V`

Unterstützt werden:

- DS3231 RTC an `0x68`
- SH1106 OLED 128 × 64 an `0x3C` oder `0x3D`

Beide Module werden beim Start erkannt. Fehlende optionale Hardware verhindert die grundlegende Ereigniserfassung nicht.

Im Autark-Modus kann die DS3231 die Zeitversorgung ohne WLAN übernehmen. Ein erkanntes OLED zeigt den Startstatus für 15 Sekunden und wird anschließend abgeschaltet.

## Speicherung

- normaler Ringspeicher: **10.000 Ereignisse**
- Langzeit-Ringspeicher: **100.000 Ereignisse**
- separater Autark-Ringspeicher: **10.000 Datensätze**
- persistenter Statistik-Cache für Heatmaps
- blockweises Lesen größerer Datenmengen zur Begrenzung von Dateisystemzugriffen und RAM-Spitzen

Der normale Ringspeicher bleibt die primäre Datenquelle. Ein Fehler im Langzeit-Ringspeicher verhindert nicht das Speichern im normalen Ringspeicher.

## Zugriff

Im normalen WLAN:

`http://unterbrechungen.local`

Alternativ kann die IP-Adresse des ESP32 verwendet werden, zum Beispiel:

`http://192.168.178.50`

Wenn das konfigurierte WLAN nicht erreichbar ist, startet der ESP32 einen lokalen Fallback-Access-Point:

- SSID: `Unterbrechungszaehler`
- Adresse: `http://192.168.4.1`

## Nachbauen

1. [Hardware beschaffen](docs/de/HARDWARE-BESCHAFFEN.md)
2. [Hardware zusammenbauen](docs/de/HARDWARE-ZUSAMMENBAU.md)
3. [Software konfigurieren](docs/de/SOFTWARE.md)
4. [Firmware flashen](docs/de/FLASHEN.md)
5. [Normale Nutzung](docs/de/NUTZUNG-NORMAL.md)
6. [Autarke Nutzung](docs/de/NUTZUNG-AUTARK.md)
7. [Software-Architektur](docs/de/SOFTWARE-ARCHITEKTUR-REBOOT.md)

## Weboberfläche

Reiter:

`Heute · Verlauf · Heatmap · Details · Export · Gerät · Einstellungen · Autark`

Unter **Gerät** werden technische Zustände und Ressourcen wie WLAN, Uptime, Firmware, RAM, Flash, LittleFS sowie die Ringspeicher angezeigt.

Unter **Einstellungen** befinden sich allgemeine Darstellungs- und Zeitfunktionen.

Die Heatmap-Filter liegen direkt bei der jeweiligen Auswertung.

## Lizenz

MIT

---

GitHub: [taloriko](https://github.com/taloriko)
