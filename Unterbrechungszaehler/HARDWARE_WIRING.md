# Projektanschluss: Unterbrechungstaster

Für den Unterbrechungszähler ist **DI1 / GPIO13** fest als physischer Taster vorgesehen:

```text
ESP32 GPIO13 / DI1 ---- Taster ---- GND
```

DI1 verwendet den vorhandenen internen Pull-up und ist active-low. Nur die entprellte Drückflanke erzeugt eine Unterbrechung. Für DI1 ist zusätzlich ein FALLING-Edge-Interrupt-Latch aktiviert; die ISR setzt ausschließlich ein Flag und führt keine Projekt-, Logging-, Audio-, Display- oder Dateisystemlogik aus.

**Projektprofil 0.1.0:** Nur DI1 ist aktiviert. DI2–DI4 und DO1–DO4 bleiben als mögliche Basiskanäle dokumentiert, sind in `hardware_config.h` aber deaktiviert und werden zur Laufzeit nicht gescannt/konfiguriert. Die Pins sind damit für spätere Projektmodule frei.

---

# Übernommene Hardwareverdrahtung – Unterbrechungszähler 0.1.0 / Basis 1.6.0

Zielboard der Standardbelegung: **klassisches ESP32 Dev Module / ESP32-WROOM-32(E)**.

Alle Pins sind zusätzlich zentral in `hardware_config.h` definiert. Wenn die reale Hardware abweicht, wird nur dort angepasst.

## Pinübersicht

| Modul/Funktion | ESP32 GPIO | Richtung aus ESP32-Sicht | Hinweis |
|---|---:|---|---|
| I2C SDA – DS3231 + SH1106 | 21 | bidirektional | gemeinsamer Bus |
| I2C SCL – DS3231 + SH1106 | 22 | Ausgang/bidirektional | gemeinsamer Bus |
| DY-SV17F TXD/IO0 | 18 | Eingang/RX | DY TX → ESP RX |
| DY-SV17F RXD/IO1 | 19 | Ausgang/TX | ESP TX → DY RX |
| DY-SV17F CON3/BUSY | 39 / VN | Eingang | input-only, kein interner Pull-up |
| DI1 | 13 | Eingang | INPUT_PULLUP, active-low |
| DI2 | 14 | Eingang | **Projekt 0.1.0: deaktiviert/frei** |
| DI3 | 32 | Eingang | **Projekt 0.1.0: deaktiviert/frei** |
| DI4 | 33 | Eingang | **Projekt 0.1.0: deaktiviert/frei** |
| DO1 | 25 | Ausgang | **Projekt 0.1.0: deaktiviert/frei** |
| DO2 | 26 | Ausgang | **Projekt 0.1.0: deaktiviert/frei** |
| DO3 | 27 | Ausgang | **Projekt 0.1.0: deaktiviert/frei** |
| DO4 | 23 | Ausgang | **Projekt 0.1.0: deaktiviert/frei** |

Nicht für die Standardbasis benutzt werden Flash-GPIO6–11, UART0 GPIO1/3 und die Strapping-Pins 0/2/4/5/12/15. GPIO16/17 bleiben frei.

## Gemeinsame Versorgung

Alle verbundenen Module benötigen eine gemeinsame Masse:

```text
ESP32 GND -------- DS3231 GND
        +--------- SH1106 GND
        +--------- DY-SV17F GND
```

ESP32-GPIOs sind 3,3-V-Logik. Keine 5-V-Signale direkt auf ESP32-Eingänge geben.

## DS3231

```text
DS3231 SDA ---- GPIO21
DS3231 SCL ---- GPIO22
DS3231 GND ---- ESP32 GND
```

Die Basis erwartet Adresse `0x68`.

Achtung bei Breakoutboards: I2C-Pull-ups dürfen die ESP32-SDA/SCL-Leitungen nicht auf 5 V ziehen. Versorgung/Pull-ups des konkreten Boards prüfen.

## SH1106 OLED

Aktuelle Basisannahme: I2C, 128×64, Adresse `0x3C`.

```text
SH1106 SDA ---- GPIO21
SH1106 SCL ---- GPIO22
SH1106 GND ---- ESP32 GND
SH1106 VCC ---- passende Modulversorgung laut Breakout
```

DS3231 und SH1106 teilen denselben I2C-Bus. `I2cBus` initialisiert `Wire` nur einmal.

Wenn das konkrete OLED auf `0x3D` liegt, nur `DISPLAY_SH1106_ADDRESS` ändern. Eine SPI-Ausführung ist nicht dieselbe Verdrahtung und benötigt später einen anderen Display-Transportadapter.

## DY-SV17F

UART:

```text
DY-SV17F TXD / IO0 -------- GPIO18  (ESP32 RX)
DY-SV17F RXD / IO1 -------- GPIO19  (ESP32 TX)
DY-SV17F GND -------------- ESP32 GND
DY-SV17F V5 --------------- stabile 5-V-Versorgung
```

### CON1/CON2/CON3 für UART-Modus

Beim Power-On muss gelten:

```text
CON3 = HIGH
CON2 = LOW
CON1 = LOW
```

Verdrahtung:

```text
DY-SV17F V33
     |
    10 kΩ
     |
     +--------- CON3 / BUSY -------- ESP32 GPIO39 / VN

CON2 -------------------------------- GND
CON1 -------------------------------- GND
```

**V33 ist hier der 3,3-V-Ausgang des DY-SV17F.** V33 nicht zusätzlich direkt mit der 3,3-V-Versorgung des ESP32 verbinden.

Der 10-kΩ-Pull-up ist wichtig, weil GPIO39/VN input-only ist und keinen internen Pull-up besitzt. CON3 wird nach der Start-Modeauswahl zum BUSY-Signal; deshalb darf CON3 nicht dauerhaft direkt an 3,3 V gelegt werden.

BUSY-Logik der Basis:

- LOW → Wiedergabe aktiv
- HIGH → idle

Nach Änderungen an CON1/2/3 für einen Modustest beide Geräte komplett stromlos machen und neu einschalten; ein reiner ESP32-Reset setzt den Betriebsmodus des separat versorgten Audiomoduls nicht zwingend neu.

## Digitale Eingänge

Standard:

```text
DI-Kontakt offen      -> inaktiv
DI-Kontakt nach GND   -> aktiv
```

weil die Kanäle als `INPUT_PULLUP`, `activeHigh=false` definiert sind.

Für externe Spannungsquellen ist eine passende Pegelanpassung/Schutzbeschaltung nötig. Keine 5 V direkt auf die GPIOs legen.

## Digitale Ausgänge

DO1–DO4 starten logisch AUS. Der GPIO darf keine Last wie Relais, Lampe, Motor oder Magnet direkt treiben. Dafür passende Treiberstufe/Transistor/MOSFET/Relaismodul und Freilauf-/Schutzbeschaltung verwenden.

Ein gesetzter DO bestätigt ohne Feedbackpin nur den ESP32-Ausgangslatch. Für echte Rückmeldung kann pro Kanal in `GPIO_CHANNELS` ein separater `feedbackPin` konfiguriert werden.

## Pinmap-Sicherheitsprüfung

Beim Boot prüft `HardwareRegistry`:

- gültige klassische ESP32-Pinnummer
- reservierte Flash-/Serial-Pins
- Output auf input-only Pins
- interne Pull-Anforderung auf GPIO34–39
- doppelte Pins inklusive Feedbackpins
- Strapping-Pins als Warnung

Ein harter Fehler blockiert die optionale Hardwareinitialisierung und setzt die betroffenen Headerzustände auf Fehler. Damit werden konfliktbehaftete Pins nicht trotzdem angesteuert.
