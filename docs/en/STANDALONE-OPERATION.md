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

## Storage without NTP

Standalone mode uses a separate ring buffer for up to 10,000 records.

Events can be stored without valid NTP time by using elapsed time within the current session.

When returning to normal operation, Wi-Fi and NTP are started again.

## Limitation

If the ESP32 loses power completely during a standalone session, the duration of that power loss cannot be reconstructed without an additional real-time clock (RTC).

More information about suitable batteries, power banks and expected runtime will follow.
