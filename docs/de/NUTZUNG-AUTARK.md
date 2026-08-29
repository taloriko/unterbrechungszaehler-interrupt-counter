# Autarke Nutzung

> **Status: In Arbeit**

Der Autark-Modus **BETA** ist für einen mobilen bzw. unabhängigen Betrieb gedacht, zum Beispiel mit:

- Akku
- USB-Powerbank
- anderer mobiler 5-V-Stromversorgung

## Aktivieren

GPIO33 wird über einen potentialfreien Schalter mit GND verbunden.

```text
GPIO33 offen       = normaler WLAN-/Netzbetrieb
GPIO33 gegen GND   = Autark-Modus BETA
```

Beim Wechsel in den Autark-Modus wird eine neue Session gestartet.

## Verhalten im Autark-Modus

Zur Reduzierung des Energieverbrauchs werden softwareseitig:

- WLAN deaktiviert
- mDNS deaktiviert
- Webserver deaktiviert
- CPU-Takt auf 80 MHz reduziert
- Light-Sleep zwischen Eingaben verwendet

Die Ereignisse werden weiterhin über GPIO27 erfasst.

## Speicherung ohne NTP

Für den Autark-Betrieb existiert ein eigener Ringspeicher mit bis zu 10.000 Datensätzen.

Ereignisse können auch ohne gültige NTP-Zeit über die relative Laufzeit innerhalb einer Session gespeichert werden.

Beim Zurückschalten in den normalen Betrieb werden WLAN und NTP wieder gestartet.

## Einschränkung

Wird der ESP32 während einer Autark-Session vollständig stromlos, kann die Dauer dieser stromlosen Zeit ohne zusätzliche Echtzeituhr (RTC) nicht rekonstruiert werden.

Weitere Angaben zu geeigneten Akkus, Powerbanks und erreichbaren Laufzeiten folgen.
