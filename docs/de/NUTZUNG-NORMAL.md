# Normale Nutzung

> **Status: In Arbeit**

Im normalen Betrieb arbeitet der ESP32 mit WLAN, NTP, mDNS und Weboberfläche.

## Ereignis erfassen

- kurzer Druck auf den Taster: Ereignis speichern
- langer Druck von ca. 3 Sekunden: letzten Eintrag löschen
- alternativ kann der virtuelle Taster in der Weboberfläche verwendet werden

Wichtig: Im normalen Betrieb wird ein Ereignis **nur gespeichert, wenn eine gültige absolute Uhrzeit vorhanden ist**.

## LED-Rückmeldung

Die Rückmeldung erfolgt über die LED an **GPIO2**. Bei manchen ESP32-Boards ist dieser Anschluss mit **D2** gekennzeichnet bzw. mit der Onboard-LED verbunden.

- 1x kurz: Ereignis gespeichert
- 3x schnell: letzter Eintrag gelöscht
- 2x schnell + 2x langsam: Warnung, z. B. fehlende Zeit, Verbindung oder Speicherproblem

## Weboberfläche im normalen WLAN

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

## Lokaler Zugriff ohne verbundenes WLAN

Solange der ESP32 das konfigurierte WLAN noch nicht erreicht hat, stellt er zusätzlich einen eigenen offenen Hotspot bereit:

```text
WLAN: Unterbrechungszaehler
Adresse: http://192.168.4.1
```

Vorgehen:

1. Mit dem Smartphone mit dem WLAN `Unterbrechungszaehler` verbinden.
2. Im Browser `http://192.168.4.1` öffnen.
3. Die Weboberfläche kann sofort lokal verwendet werden.

Sobald der ESP32 sein normales WLAN erreicht, wird der Fallback-Hotspot automatisch abgeschaltet.

## Zeit vom Smartphone übernehmen

Wenn NTP noch keine gültige Uhrzeit geliefert hat, versucht die Weboberfläche beim Öffnen automatisch einmalig, die aktuelle Zeit des Smartphones bzw. Browsers an den ESP32 zu übertragen.

Die Zeit wird nur übernommen, wenn auf dem ESP32 noch **keine gültige Zeit** vorhanden ist.

Damit gilt im normalen Betrieb weiterhin:

**Keine gültige Zeit = keine Speicherung eines Ereignisses.**

Nach erfolgreicher Übernahme der Handyzeit können Ereignisse lokal gespeichert werden. Sobald NTP verfügbar ist, läuft die normale NTP-Synchronisation weiter.

## Bereiche der Weboberfläche

- Heute
- Verlauf
- Heatmap
- Details
- Export
- Gerät
- Autark **BETA**

Weitere Bedienhinweise folgen.
