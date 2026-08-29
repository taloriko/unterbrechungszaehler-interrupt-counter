# Hardware zusammenbauen

> **Status: In Arbeit**

## Taster

Der Ereigniseingang liegt auf GPIO27.

```text
GPIO27 ---- potentialfreier Kontakt ---- GND
```

Der Eingang verwendet `INPUT_PULLUP`.

**Keine externe Spannung an GPIO27 anlegen.**

## Schalter für den Autark-Modus

Optional kann ein potentialfreier Schalter an GPIO33 angeschlossen werden.

```text
GPIO33 ---- potentialfreier Schalter ---- GND
```

- GPIO33 offen: normaler WLAN-/Netzbetrieb
- GPIO33 gegen GND: Autark-Modus **BETA**

Auch GPIO33 verwendet `INPUT_PULLUP`.

**Keine externe Spannung an GPIO33 anlegen.**

## RTC und OLED über I2C

Die optionalen Erweiterungen werden gemeinsam am I2C-Bus betrieben:

```text
ESP32 GPIO21 SDA ----+---- DS3231 SDA
                     +---- SH1106 SDA

ESP32 GPIO22 SCL ----+---- DS3231 SCL
                     +---- SH1106 SCK/SCL

ESP32 3V3 -----------+---- DS3231 VCC
                     +---- SH1106 VDD

ESP32 GND -----------+---- DS3231 GND
                     +---- SH1106 GND
```

Unterstützt werden aktuell:

- **DS3231 RTC**, Adresse `0x68`
- **SH1106 OLED 128 × 64**, Adresse `0x3C` oder `0x3D`

Die Erkennung erfolgt automatisch einmal beim Boot. Fehlt ein Modul, läuft die Firmware ohne Fehler weiter.

Weitere Angaben zu Gehäuse, Klemmen und mechanischem Aufbau folgen.
