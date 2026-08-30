# Autarke Nutzung

Der Autark-Modus ist für mobilen Betrieb mit Akku, USB-Powerbank oder einer anderen 5-V-Stromversorgung vorgesehen.

## Aktivieren

GPIO33 wird über einen potentialfreien Schalter mit GND verbunden:

```text
GPIO33 offen       = normaler WLAN-/Netzbetrieb
GPIO33 gegen GND   = Autark-Modus
```

Beim Wechsel in den Autark-Modus wird eine neue Session gestartet.

## Stromsparverhalten

Im Autark-Modus werden:

- WLAN abgeschaltet
- mDNS abgeschaltet
- Webserver abgeschaltet
- CPU-Takt auf 80 MHz reduziert
- Light-Sleep zwischen Bedienungen verwendet

Die Ereigniserfassung über GPIO27 bleibt aktiv.

## RTC

Eine DS3231 an `0x68` wird beim Boot automatisch erkannt.

- gültige RTC-Zeit kann die Systemzeit beim Start setzen
- Autark-Ereignisse können damit absolute Zeit erhalten
- nach erfolgreicher NTP-Synchronisation wird die RTC aktualisiert
- ungültige RTC-Zeit wird nicht als Zeitquelle übernommen

## OLED

Ein SH1106 OLED 128 × 64 an `0x3C` oder `0x3D` zeigt beim Start den Geräte- und Zeitstatus.

Im Autarkbetrieb wird das Display nach 15 Sekunden vollständig abgeschaltet.

## Speicherung ohne absolute Zeit

Für Autark-Sessions existiert ein eigener Ringspeicher mit bis zu 10.000 Datensätzen.

Ist keine gültige absolute Zeit vorhanden, werden Ereignisse als relative Laufzeit seit dem Session-Start gespeichert. Dadurch bleiben die Abstände innerhalb der Session erhalten.

Beim Zurückschalten in den Normalbetrieb werden WLAN und normale Zeitverwaltung wieder aktiviert.

## Neustart ohne RTC

Nach einer vollständigen Stromunterbrechung kann eine zuvor nur relativ erfasste Session ohne RTC nicht sicher einer absoluten Uhrzeit zugeordnet werden. Mit gültiger DS3231 steht nach dem Neustart wieder eine absolute Zeitbasis zur Verfügung.
