# Standalone operation

Standalone mode is intended for mobile operation using a battery, USB power bank or another 5 V supply.

## Enable

Connect GPIO33 to GND through a dry-contact switch:

```text
GPIO33 open       = normal Wi-Fi/network mode
GPIO33 to GND     = standalone mode
```

A new standalone session starts when the mode is enabled.

## Power-saving behavior

Standalone mode:

- disables Wi-Fi
- disables mDNS
- disables the web server
- reduces CPU frequency to 80 MHz
- uses Light Sleep between interactions

Event recording on GPIO27 remains active.

## RTC

A DS3231 at `0x68` is detected during boot.

- valid RTC time can initialize system time
- standalone events can use absolute timestamps
- successful NTP synchronization updates the RTC
- invalid RTC time is not used as a time source

## OLED

A SH1106 128 × 64 OLED at `0x3C` or `0x3D` shows device and time status during startup.

In standalone mode the display switches off completely after 15 seconds.

## Storage without absolute time

Standalone sessions use a separate ring buffer for up to 10,000 records.

Without valid absolute time, events are stored as elapsed time since session start. Relative gaps inside the session therefore remain available.

Returning to normal mode enables Wi-Fi and normal time management again.

## Restart without RTC

After a complete power loss, a session recorded only with relative time cannot safely be assigned to an absolute time without an RTC. With a valid DS3231, an absolute time base is available again after restart.
