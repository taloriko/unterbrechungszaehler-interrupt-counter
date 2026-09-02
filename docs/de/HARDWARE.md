# Hardware – Unterbrechungszähler 3.0.0

Zielplattform ist ein klassisches **ESP32 Dev Module / ESP32-WROOM-32(E)**.

Version 3.0.0 ist ein Hardware-Neustart gegenüber den alten Testständen. Alte 2.x-Verdrahtungen nicht übernehmen, sondern diese Pinbelegung verwenden.

## Pinbelegung

| Modul/Funktion | ESP32 GPIO | Hinweis |
|---|---:|---|
| Unterbrechungstaster / DI1 | 13 | `INPUT_PULLUP`, active-low, Taster nach GND |
| I2C SDA – DS3231 + SH1106 | 21 | gemeinsamer I2C-Bus |
| I2C SCL – DS3231 + SH1106 | 22 | gemeinsamer I2C-Bus |
| DY-SV17F TXD/IO0 | 18 | DY TX → ESP RX |
| DY-SV17F RXD/IO1 | 19 | ESP TX → DY RX |
| DY-SV17F CON3/BUSY | 39 / VN | input-only, externer Pull-up erforderlich |

DI2–DI4 und DO1–DO4 sind im aktuellen Projektprofil deaktiviert und werden nicht initialisiert.

## Unterbrechungstaster

```text
ESP32 GPIO13 / DI1 ---- Taster ---- GND
```

Nur die Drückflanke zählt. Der Eingang nutzt Pull-up, Entprellung und einen kleinen FALLING-Edge-Latch, damit auch während längerer synchroner HTTP-Ausgaben ein normaler Tastendruck erhalten bleibt.

## DS3231 + SH1106

```text
DS3231 SDA ---- GPIO21
DS3231 SCL ---- GPIO22
SH1106 SDA ---- GPIO21
SH1106 SCL ---- GPIO22
alle GND ----- ESP32 GND
```

RTC-Adresse: `0x68`. OLED: 128×64, standardmäßig `0x3C`.

Achtung bei Breakoutboards: I2C-Pull-ups dürfen SDA/SCL nicht auf 5 V ziehen.

## DY-SV17F

UART2 läuft mit **9600 Baud, 8N1**.

```text
DY-SV17F TXD/IO0 ---- GPIO18
DY-SV17F RXD/IO1 ---- GPIO19
DY-SV17F GND -------- ESP32 GND
DY-SV17F V5 --------- stabile 5-V-Versorgung
```

Für UART-Modus beim Einschalten:

```text
CON3 = HIGH
CON2 = LOW
CON1 = LOW
```

Verdrahtung für CON3/BUSY:

```text
DY-SV17F V33
     |
    10 kΩ
     |
     +--------- CON3 / BUSY -------- ESP32 GPIO39 / VN

CON2 -------------------------------- GND
CON1 -------------------------------- GND
```

GPIO39 hat keinen internen Pull-up. V33 ist der 3,3-V-Ausgang des DY-SV17F und darf nicht zusätzlich direkt an ESP32-3V3 gelegt werden.

BUSY: LOW = Wiedergabe aktiv, HIGH = idle.

## Sounddateien

- Track 1: Boot-Ton
- Track 2 und höher: Unterbrechungstöne
- Im Rotationsmodus werden erkannte Tracks 2…N verwendet.

Die konkrete Dateibenennung auf dem Datenträger richtet sich nach dem vom DY-SV17F unterstützten Dateisystem/Sortierverhalten. Vor dem öffentlichen Verteilen eigener Audiodateien deren Lizenz/Herkunft prüfen.

## Versorgung

Alle Module brauchen gemeinsame Masse. ESP32-GPIOs sind 3,3-V-Logik; keine 5-V-Signale direkt auf ESP32-Eingänge geben.

Die vollständige technische Verdrahtungsdokumentation liegt zusätzlich in [`../../Unterbrechungszaehler/HARDWARE_WIRING.md`](../../Unterbrechungszaehler/HARDWARE_WIRING.md).
