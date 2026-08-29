# Normale Nutzung

> **Status: In Arbeit**

Im normalen Betrieb arbeitet der ESP32 mit WLAN, NTP, mDNS und Weboberfläche.

## Ereignis erfassen

- kurzer Druck auf den Taster: Ereignis speichern
- langer Druck von ca. 3 Sekunden: letzten Eintrag löschen
- alternativ kann der virtuelle Taster in der Weboberfläche verwendet werden

## LED-Rückmeldung

Die Rückmeldung erfolgt über die LED an **GPIO2**. Bei manchen ESP32-Boards ist dieser Anschluss mit **D2** gekennzeichnet bzw. mit der Onboard-LED verbunden.

- 1x kurz: Ereignis gespeichert
- 3x schnell: letzter Eintrag gelöscht
- 2x schnell + 2x langsam: Warnung, z. B. fehlende Zeit, Verbindung oder Speicherproblem

## Weboberfläche

Nach erfolgreicher WLAN-Verbindung ist die Oberfläche normalerweise erreichbar unter:

```text
http://unterbrechungen.local
```

Falls mDNS im Netzwerk nicht funktioniert, kann die im seriellen Monitor ausgegebene IP-Adresse direkt im Browser verwendet werden.

Beispiel:

```text
http://192.168.1.123
```

Die tatsächliche IP-Adresse hängt vom verwendeten Netzwerk ab.

Die Weboberfläche enthält aktuell die Bereiche:

- Heute
- Verlauf
- Heatmap
- Details
- Export
- Gerät
- Autark **BETA**

Weitere Bedienhinweise folgen.
