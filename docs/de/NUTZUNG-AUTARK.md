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

## RTC-Unterstützung

Ist beim Boot eine **DS3231 RTC** an I2C-Adresse `0x68` vorhanden, wird sie automatisch erkannt.

- gültige RTC-Zeit wird als Systemzeit übernommen
- der Autark-Betrieb kann damit auch ohne WLAN/NTP absolute Zeit verwenden
- nach erfolgreicher NTP-Synchronisation im Normalbetrieb wird die RTC aktualisiert
- ist das OSF-Flag der RTC gesetzt oder die Zeit unplausibel, wird die RTC als erkannt, aber zeitlich ungültig angezeigt

## OLED-Startanzeige

Ist ein **SH1106 OLED 128 × 64** an `0x3C` oder `0x3D` vorhanden, zeigt es beim Start im Autark-Modus kurz den Zustand an:

- Autark-Modus
- RTC erkannt / nicht erkannt
- RTC-Zeit OK / ungültig
- aktuelle Zeit
- System bereit

Nach **15 Sekunden** wird das Display vollständig abgeschaltet. Dadurch bleibt der Energieverbrauch im Autark-Betrieb gering und OLED-Einbrennen wird vermieden.

## Speicherung ohne RTC

Für den Autark-Betrieb existiert weiterhin ein eigener Ringspeicher mit bis zu 10.000 Datensätzen.

Ist keine gültige RTC vorhanden, können Ereignisse weiterhin über die relative Laufzeit innerhalb einer Session gespeichert werden.

Beim Zurückschalten in den normalen Betrieb werden WLAN und NTP wieder gestartet.

## Einschränkung

Ohne RTC kann eine vollständige Stromunterbrechung des ESP32 während einer Autark-Session zeitlich nicht rekonstruiert werden. Mit gültiger DS3231-Zeit steht nach dem Neustart wieder eine absolute Uhrzeit zur Verfügung.

Weitere Angaben zu geeigneten Akkus, Powerbanks und erreichbaren Laufzeiten folgen.
