# Unterbrechungszähler / Interrupt Counter

[🇬🇧 English documentation](docs/en/README.md)

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt und anschließend praktisch getestet und weiterentwickelt. Wem die Verwendung von KI-generiertem Code grundsätzlich nicht passt, kann hier aufhören.

> **Stand dieser README:** `2026-08-29-20`

## Was ist das?

Der Unterbrechungszähler ist ein ESP32-basiertes Modul zum einfachen Erfassen und Auswerten von Ereignissen.

**Taster drücken → Ereignis speichern → später auswerten.**

Entstanden ist das Projekt aus der Frage, wie häufig Unterbrechungen im Arbeitsalltag tatsächlich auftreten. Statt Gefühl gibt es damit Zahlen, Tagesverläufe und Heatmaps.

Der Eingang arbeitet mit einem potentialfreien Kontakt und kann deshalb auch für andere Anwendungen genutzt werden, zum Beispiel Relais-, Tür-, Störmelde- oder Maschinenkontakte.

## Funktionen

- Ereigniserfassung per physischem oder virtuellem Taster
- Datum und Uhrzeit automatisch per NTP
- optionale **DS3231 RTC** als lokale Zeitquelle
- optionales **1,3 Zoll SH1106 OLED 128 × 64**
- automatische Hardware-Erkennung beim Boot
- lokale Weboberfläche auf dem ESP32
- Tagesübersicht, Verlauf und Detailansicht
- Heatmap Wochentag / Uhrzeit mit Auswahl von **Jahr, Kalenderwoche und Zeitbereich**
- Standard für Wochentag/Uhrzeit: **aktuelle Kalenderwoche, 05:00 bis 18:00 Uhr**
- Heatmap Monat / Kalenderwoche mit Jahresauswahl
- Heatmap Jahr / Monat für aktuelles Jahr plus vier Vorjahre
- CSV-Export
- Hell-/Dunkelmodus und mehrere Sprachen
- Fallback-WLAN bei nicht erreichbarem normalen WLAN
- serielle Diagnose mit `115200 Baud`
- Autark-Modus **BETA** für Akku-/Powerbank-Betrieb

### Zeitstrategie

Die Zeitquellen haben eine feste Hierarchie:

1. **NTP** – verbindliche Referenz und höchste Priorität
2. **RTC** – lokale Startzeit und Zeitquelle im Autarkbetrieb
3. **Browserzeit** – Fallback, wenn NTP und RTC nicht verfügbar sind
4. **keine absolute Zeit** – im Autarkbetrieb werden Ereignisse relativ zur Session gespeichert

NTP-Server, RTC-Zustand, aktuelle Zeitquelle und erkannte Differenzen werden gemeinsam unter **Einstellungen → Zeitverwaltung** dargestellt.

Bei einer später erkannten Zeitdifferenz werden historische Daten nicht pauschal verändert. Nur eindeutig vorläufige Daten des aktuellen Start-/Synchronisationszeitraums dürfen korrigiert werden. Bei Autark-Sessions wird bevorzugt der Zeitanker der Session korrigiert, während die relativen Ereignisabstände unverändert bleiben.

### Optionale Hardware-Erweiterungen

RTC und OLED teilen sich den I2C-Bus:

- `GPIO21` = SDA
- `GPIO22` = SCL
- Versorgung mit `3,3 V`

Unterstützt werden aktuell:

- DS3231 RTC an `0x68`
- SH1106 OLED 128 × 64 an `0x3C` oder `0x3D`

Beim Boot werden beide Module einmal automatisch gesucht. In der Weboberfläche werden RTC- und Display-Symbole immer angezeigt: erkannt = aktiv, nicht erkannt = ausgegraut.

Im Autark-Modus übernimmt eine gültige DS3231 die Zeit auch ohne WLAN/NTP. Ein erkanntes OLED zeigt beim Start für **15 Sekunden** den Autark-/RTC-Status und schaltet danach vollständig ab.

### Speicherung

- normaler Ringspeicher: **10.000 Ereignisse**
- Langzeit-Ringspeicher: **100.000 Ereignisse**
- Heatmaps werden über aggregierte Daten geladen, damit die Oberfläche auch bei vielen gespeicherten Ereignissen schnell bleibt

Bei durchschnittlich 30 Ereignissen pro Werktag reicht der Langzeitspeicher rechnerisch für ungefähr **12,8 Jahre**.

## Zugriff

Im normalen WLAN:

`http://unterbrechungen.local`

Falls mDNS nicht funktioniert, kann direkt die IP-Adresse des ESP32 verwendet werden, zum Beispiel:

`http://192.168.178.50`

Wenn das normale WLAN nicht erreichbar ist, startet der ESP32 ein lokales Fallback-WLAN:

- SSID: `Unterbrechungszaehler`
- Adresse: `http://192.168.4.1`

## Nachbauen

1. [Hardware beschaffen](docs/de/HARDWARE-BESCHAFFEN.md)
2. [Hardware zusammenbauen](docs/de/HARDWARE-ZUSAMMENBAU.md)
3. [Software konfigurieren](docs/de/SOFTWARE.md)
4. [Firmware flashen](docs/de/FLASHEN.md)
5. [Normale Nutzung](docs/de/NUTZUNG-NORMAL.md)
6. [Autarke Nutzung](docs/de/NUTZUNG-AUTARK.md)

## Weboberfläche

Reiter:

`Heute · Verlauf · Heatmap · Details · Export · Gerät · Einstellungen · Autark`

Unter **Gerät** bleiben die technischen Geräteinformationen wie WLAN, Uptime, Firmware, RAM, Flash, LittleFS sowie normaler und Langzeit-Ringspeicher.

Unter **Einstellungen** befinden sich zuerst Sprache und Darstellung und darunter die gemeinsame **Zeitverwaltung** mit NTP, RTC, Zeitquelle und Korrekturstrategie. Die separate RTC-Zeitkarte wird dadurch vermieden.

Die Filter für Jahr, Kalenderwoche und Uhrzeitbereich befinden sich direkt im Reiter **Heatmap**, weil sie unmittelbar die dargestellte Auswertung verändern.

## Screenshots

### Heute

![Reiter Heute](docs/images/Reiter%20-%20Heute.png)

### Heatmap

![Reiter Heatmap](docs/images/Reiter%20-%20Heatmap.png)

### Gerät

![Reiter Gerät](docs/images/Reiter%20-%20Geraet.png)

### Autark – Beta

![Reiter Autark Beta](docs/images/Reiter%20-%20Autark%20-%20Beta.png)

> Die Screenshots können vom aktuellen Entwicklungsstand leicht abweichen.

## Lizenz

MIT

---

GitHub: [taloriko](https://github.com/taloriko)
