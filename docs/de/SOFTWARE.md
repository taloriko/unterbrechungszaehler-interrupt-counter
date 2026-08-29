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
- lokaler Fallback-Hotspot bei fehlender WLAN-Verbindung
- fester lokaler Zugriff über `http://192.168.4.1`
- automatische Übernahme der Handy-/Browserzeit, solange noch keine gültige Gerätezeit vorhanden ist
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

## Lokaler Fallback-Zugriff

Solange das konfigurierte WLAN noch nicht verbunden ist, startet der ESP32 zusätzlich einen eigenen offenen Access Point:

```text
SSID: Unterbrechungszaehler
IP:   192.168.4.1
URL:  http://192.168.4.1
```

Damit kann die Weboberfläche direkt mit einem Smartphone geöffnet werden, auch wenn das normale WLAN noch nicht verfügbar ist.

Sobald die Verbindung zum konfigurierten WLAN hergestellt wurde, wird dieser Fallback-Hotspot automatisch abgeschaltet.

## Zeit ohne NTP im Normalbetrieb

Im normalen Betrieb werden Ereignisse weiterhin **nur mit gültiger absoluter Uhrzeit** gespeichert.

Wenn noch keine gültige Zeit per NTP vorhanden ist und die Weboberfläche mit einem Smartphone oder Browser geöffnet wird, überträgt die Webseite einmalig die aktuelle Unix-Zeit des verbundenen Geräts an den ESP32.

Diese Zeit wird nur übernommen, wenn der ESP32 zu diesem Zeitpunkt noch keine gültige Uhrzeit besitzt. Eine bereits gültige Zeit wird dadurch nicht überschrieben.

Sobald NTP verfügbar ist, übernimmt die normale NTP-Synchronisation wieder die Zeitpflege.

Der Autark-Modus bleibt davon getrennt und arbeitet weiterhin nach seiner eigenen relativen Zeitlogik.

Weitere Konfigurationsmöglichkeiten und Details zum Softwareaufbau folgen.
