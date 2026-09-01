# Normale Nutzung

Im normalen Betrieb arbeitet der ESP32 mit WLAN, Zeitverwaltung, mDNS und Weboberfläche.

## Ereignis erfassen

- kurzer Druck auf den Taster: Ereignis speichern
- langer Druck von ca. 3 Sekunden: letzten Eintrag löschen
- alternativ über den virtuellen Taster in der Weboberfläche

Normale Ereignisse werden nur gespeichert, wenn eine gültige absolute Uhrzeit vorhanden ist.

## LED-Rückmeldung

Die LED liegt auf GPIO2. Bei manchen ESP32-Boards ist dieser Anschluss als D2 oder als Onboard-LED ausgeführt.

- 1x kurz: Ereignis gespeichert
- 3x schnell: letzter Eintrag gelöscht
- 2x schnell + 2x langsam: Warnung, z. B. fehlende Zeit oder Speicherproblem

## Weboberfläche

Normaler Zugriff:

```text
http://unterbrechungen.local
```

Alternativ kann die im seriellen Monitor angezeigte IP-Adresse verwendet werden, zum Beispiel:

```text
http://192.168.1.123
```

## Fallback-WLAN

Solange das konfigurierte WLAN nicht erreichbar ist:

```text
WLAN:    Unterbrechungszaehler
Adresse: http://192.168.4.1
```

Sobald das normale WLAN verbunden ist, wird der Fallback-Access-Point abgeschaltet.

## Zeit ohne NTP

Wenn weder NTP noch eine gültige RTC eine Zeit liefern, kann die Weboberfläche einmalig die aktuelle Browserzeit übertragen.

Eine bereits gültige Gerätezeit wird dadurch nicht überschrieben.

## Bereiche der Weboberfläche

- Heute
- Verlauf
- Heatmap
- Details
- Export
- Gerät
- Einstellungen
- Autark

Unter **Export** stehen die gespeicherten Daten als CSV zur Verfügung. Unter **Gerät** werden Netzwerk, Speicher und Hardwarestatus angezeigt. Unter **Einstellungen** befinden sich Sprache, Darstellung, RTC-, NTP- und Displayfunktionen.
