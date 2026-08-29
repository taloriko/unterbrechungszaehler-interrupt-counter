# Software

> **Status: In Arbeit**

Die Firmware befindet sich unter:

```text
arduino/UnterbrechungszaehlerInterruptCounter/
```

Hauptdatei:

```text
UnterbrechungszaehlerInterruptCounter.ino
```

## Aktuelle Funktionen

- lokale Weboberfläche
- Ereigniserfassung über GPIO27
- virtueller Taster in der Weboberfläche
- Löschen des letzten Eintrags per langem Tastendruck
- automatische NTP-Zeitsynchronisation
- automatische CET-/CEST-Umschaltung
- konfigurierbarer primärer NTP-Server
- Fallback-NTP-Server
- Tagesauswertung
- Verlauf
- Heatmap nach Wochentag und Uhrzeit
- Detailansicht mit Zeitabständen
- CSV-Export
- Geräteinformationen zu ESP32, WLAN, RAM, Flash und LittleFS
- grafische Speicheranzeigen
- binärer Ringspeicher für bis zu 10.000 normale Ereignisse
- separater Ringspeicher für den Autark-Modus
- automatische Dark-Mode-Darstellung
- mDNS über `unterbrechungen.local`
- Autark-Modus **BETA**

## WLAN-Zugangsdaten

Die Datei `Secrets.example.h` dient als Vorlage.

Sie wird nach `Secrets.h` kopiert und dort mit den eigenen WLAN-Zugangsdaten ergänzt.

`Secrets.h` ist über `.gitignore` vom Repository ausgeschlossen.

Weitere Konfigurationsmöglichkeiten und Details zum Softwareaufbau folgen.
