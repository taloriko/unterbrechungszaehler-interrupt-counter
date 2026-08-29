# Standalone operation

> **Status: Work in progress**

Standalone mode **BETA** is intended for mobile or independent operation, for example with:

- battery
- USB power bank
- another mobile 5 V supply

## Enable standalone mode

Connect GPIO33 to GND using a dry-contact switch.

```text
GPIO33 open       = normal Wi-Fi/network mode
GPIO33 to GND     = standalone mode BETA
```

A new standalone session is started when switching into standalone mode.

## Behavior in standalone mode

To reduce power consumption, the software:

- disables Wi-Fi
- disables mDNS
- disables the web server
- reduces CPU frequency to 80 MHz
- uses Light Sleep between inputs

Events are still recorded via GPIO27.

## RTC support

If a **DS3231 RTC** is present at I2C address `0x68` during boot, it is detected automatically.

- valid RTC time is used as system time
- standalone operation can use absolute time without Wi-Fi/NTP
- after a successful NTP sync in normal mode, the RTC is updated automatically
- if the RTC OSF flag is set or the stored date/time is implausible, the module is shown as detected but its time is treated as invalid

## OLED boot status

If a **SH1106 128 × 64 OLED** is present at `0x3C` or `0x3D`, it briefly shows the startup state in standalone mode:

- standalone mode
- RTC detected / not detected
- RTC time OK / invalid
- current time
- system ready

The display switches off completely after **15 seconds** to reduce power consumption and OLED burn-in.

## Storage without RTC

Standalone mode keeps its separate ring buffer for up to 10,000 records.

If no valid RTC is available, events can still be stored using elapsed time within the current session.

When returning to normal operation, Wi-Fi and NTP are started again.

## Limitation

Without an RTC, a complete ESP32 power loss during a standalone session cannot be reconstructed in absolute time. With a valid DS3231, absolute time is available again after reboot.

More information about suitable batteries, power banks and expected runtime will follow.
