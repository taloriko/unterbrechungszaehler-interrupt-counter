# Assembly

## Event input

```text
GPIO27 ---- dry contact ---- GND
```

GPIO27 uses `INPUT_PULLUP`.

**Do not apply external voltage to GPIO27.**

## Standalone switch

```text
GPIO33 ---- dry-contact switch ---- GND
```

- GPIO33 open: normal Wi-Fi/network mode
- GPIO33 connected to GND: standalone mode

GPIO33 also uses `INPUT_PULLUP`.

**Do not apply external voltage to GPIO33.**

## RTC and OLED via I2C

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

Supported addresses:

- DS3231 RTC: `0x68`
- SH1106 OLED 128 × 64: `0x3C` or `0x3D`

Both devices are detected during boot. Missing optional hardware does not prevent the remaining functions from operating.

## Power supply

The ESP32 is powered from 5 V. Use at least 1 A; 2 A is recommended for reliable Wi-Fi current peaks.
