# Changelog

## 1.0.0

Erste öffentliche Release-Version.

### Funktionen

- Ereigniserfassung über Taster oder potentialfreien Kontakt
- normaler Ringspeicher mit 10.000 Ereignissen
- Langzeit-Ringspeicher mit 100.000 Ereignissen
- separater Autark-Ringspeicher
- lokale Weboberfläche ohne Cloud-Abhängigkeit
- Tagesansicht, Verlauf, Details und Heatmaps
- CSV-Export
- NTP-Zeitverwaltung mit Browser-Fallback
- optionale DS3231-RTC
- optionales SH1106-OLED 128 × 64
- Fallback-Access-Point bei fehlendem WLAN
- Autarkbetrieb mit reduziertem CPU-Takt und Light-Sleep
- serielle Diagnose mit 115200 Baud
- Weboberfläche in Deutsch, Englisch und Schwäbisch

### Architektur

- modular getrennte Services für Eingänge, Speicherung, Zeit, Netzwerk, Display, RTC, Analyse und Weboberfläche
- blockweiser Zugriff auf große Ringspeicher
- persistenter Statistik-Cache für Heatmaps
- optionale Hardware wird beim Start erkannt
- zentrale Sprachverwaltung mit gemeinsamem Schlüsselsatz
