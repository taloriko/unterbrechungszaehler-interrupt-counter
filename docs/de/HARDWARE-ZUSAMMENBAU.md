# Hardware zusammenbauen

## Ereigniseingang

Der Ereigniseingang liegt auf GPIO27:

```text
GPIO27 ---- potentialfreier Kontakt ---- GND
```

GPIO27 verwendet `INPUT_PULLUP`.

**Keine externe Spannung an GPIO27 anlegen.**

## Autark-Schalter

Optional kann ein potentialfreier Schalter an GPIO33 angeschlossen werden:

```text
GPIO33 ---- potentialfreier Schalter ---- GND
```

- GPIO33 offen: normaler WLAN-/Netzbetrieb
- GPIO33 gegen GND: Autark-Modus

GPIO33 verwendet ebenfalls `INPUT_PULLUP`.

**Keine externe Spannung an GPIO33 anlegen.**

## RTC und OLED über I2C

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

Unterstützte Adressen:

- DS3231 RTC: `0x68`
- SH1106 OLED 128 × 64: `0x3C` oder `0x3D`

Beide Module werden beim Boot automatisch erkannt. Fehlende optionale Hardware verhindert den Betrieb der übrigen Funktionen nicht.

## Versorgung

Der ESP32 wird über 5 V versorgt. Für den normalen WLAN-Betrieb werden mindestens 1 A, empfohlen 2 A vorgesehen.
