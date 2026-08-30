# Changelog

## 1.0.1

Reine Fehlerbehebungs- und Optimierungsversion ohne neue Funktionen.

### Fehlerbehebungen

- flackernde bzw. zuckende Texte bei Ringspeicher- und Geräteanzeigen behoben
- problematischer nativer Sprachdialog auf Smartphones durch eine stabile Button-Auswahl ersetzt
- Absturz bei aktivem Schwäbisch durch Entfernen der rekursiven Live-Übersetzung behoben
- Schwäbisch wird als fester UTF-8-Sprachsatz behandelt; Sonderzeichen benötigen keine Sonderlogik
- springende Symbole und verschobene Buttons bei der Darstellungswahl behoben
- alte Darstellungs-Rückmeldungen werden beim nächsten Klick entfernt
- unnötige Mehrfachaktualisierung derselben Statuswerte entfernt
- Sprach- und UI-Nachbearbeitung wird nicht mehr bei jeder Textänderung der gesamten Seite ausgeführt

### Optimierungen

- dynamische Übersetzungen werden nur noch an definierten Aktualisierungspunkten ausgeführt
- DOM-Werte werden nur noch geschrieben, wenn sich ihr sichtbarer Inhalt wirklich geändert hat
- RAM-, Flash- und LittleFS-Zusatzanzeigen werden langsamer aktualisiert
- numerische Geräte- und Speicheranzeigen verwenden eine stabilere Zifferndarstellung
- Sprachumschaltung verwendet weiterhin die zentrale Sprachverwaltung; der interne Select wird nur noch programmintern genutzt

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
