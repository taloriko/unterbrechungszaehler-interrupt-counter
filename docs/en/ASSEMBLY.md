# Assembly

> **Status: Work in progress**

## Event input

```text
GPIO27 ---- dry contact ---- GND
```

GPIO27 uses `INPUT_PULLUP`.

**Do not apply external voltage to GPIO27.**

## Standalone mode switch

```text
GPIO33 ---- dry-contact switch ---- GND
```

- GPIO33 open: normal Wi-Fi/network mode
- GPIO33 connected to GND: standalone mode **BETA**

GPIO33 also uses `INPUT_PULLUP`.

**Do not apply external voltage to GPIO33.**

## RTC and OLED via I2C

The optional RTC and OLED share the same I2C bus:

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

Currently supported:

- **DS3231 RTC** at `0x68`
- **SH1106 OLED 128 × 64** at `0x3C` or `0x3D`

Both devices are detected automatically once during boot. Missing modules do not prevent the firmware from running.

More details about enclosure, terminals and mechanical assembly will follow.
