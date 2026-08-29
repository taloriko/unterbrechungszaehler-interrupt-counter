# Normale Nutzung

> **Status: In Arbeit**

Im normalen Betrieb arbeitet der ESP32 mit WLAN, NTP, mDNS und Weboberfläche.

## Ereignis erfassen

- kurzer Druck auf den Taster: Ereignis speichern
- langer Druck von ca. 3 Sekunden: letzten Eintrag löschen
- alternativ kann der virtuelle Taster in der Weboberfläche verwendet werden

## LED-Rückmeldung

- 1x kurz: Ereignis gespeichert
- 3x schnell: letzter Eintrag gelöscht
- 2x schnell + 2x langsam: Warnung, z. B. fehlende Zeit, Verbindung oder Speicherproblem

## Weboberfläche

Nach erfolgreicher WLAN-Verbindung ist die Oberfläche normalerweise erreichbar unter:

```text
http://unterbrechungen.local
```

Falls mDNS im Netzwerk nicht funktioniert, kann die im seriellen Monitor ausgegebene IP-Adresse verwendet werden.

Die Weboberfläche enthält aktuell die Bereiche:

- Heute
- Verlauf
- Heatmap
- Details
- Export
- Gerät
- Autark **BETA**

Weitere Bedienhinweise folgen.
