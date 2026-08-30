# Unterbrechungszähler / Interrupt Counter

[Deutsch](docs/de/README.md) · [English](docs/en/README.md) · [Schwäbisch](docs/swg/README.md)

ESP32-basierter Zähler für Unterbrechungen oder andere potentialfreie Kontakte.

**Kontakt auslösen → Zeitpunkt speichern → lokal im Browser auswerten.**

![Weboberfläche](docs/images/Reiter%20-%20Heute.png)

## Was kann das Gerät?

- Ereignisse per Taster oder potentialfreiem Kontakt erfassen
- lokale Weboberfläche ohne Cloud
- Tagesansicht, Verlauf, Details und Heatmaps
- CSV-Export und Langzeit-Ringspeicher
- NTP sowie optionale DS3231-RTC
- optionales SH1106-OLED 128 × 64
- Fallback-WLAN für lokalen Zugriff
- Autarkbetrieb für Akku oder Powerbank
- Deutsch, Englisch und Schwäbisch in der Oberfläche

## Schnellstart

1. [Hardware](docs/de/HARDWARE-BESCHAFFEN.md)
2. [Zusammenbau](docs/de/HARDWARE-ZUSAMMENBAU.md)
3. [Software konfigurieren](docs/de/SOFTWARE.md)
4. [Firmware flashen](docs/de/FLASHEN.md)
5. [Benutzung](docs/de/NUTZUNG-NORMAL.md)

Firmware: `arduino/Unterbrechungszaehler/`

Standardzugriff im WLAN: `http://unterbrechungen.local`

Fallback-WLAN: `Unterbrechungszaehler` → `http://192.168.4.1`

## Weitere Dokumentation

- [Autarkbetrieb](docs/de/NUTZUNG-AUTARK.md)
- [Software-Architektur](docs/de/SOFTWARE-ARCHITEKTUR.md)
- [Änderungen](CHANGELOG.md)

## Lizenz

MIT

---

GitHub: [taloriko](https://github.com/taloriko)
