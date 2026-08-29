# Software

> **Status: Work in progress**

The firmware is located in:

```text
arduino/UnterbrechungszaehlerInterruptCounter/
```

Main file:

```text
UnterbrechungszaehlerInterruptCounter.ino
```

## Current features

- local web interface
- event recording via GPIO27
- virtual button in the web interface
- delete latest event by long button press
- automatic NTP time synchronization
- automatic CET/CEST handling
- configurable primary NTP server
- fallback NTP servers
- daily analysis
- history
- weekday/hour heatmap
- detailed view with time gaps
- CSV export
- device information for ESP32, Wi-Fi, RAM, Flash and LittleFS
- graphical storage indicators
- binary ring buffer for up to 10,000 normal events
- separate ring buffer for standalone mode
- automatic dark mode
- mDNS via `unterbrechungen.local`
- standalone mode **BETA**

## Wi-Fi credentials

Copy `Secrets.example.h` to `Secrets.h` and enter your Wi-Fi credentials there.

`Secrets.h` is excluded from the repository through `.gitignore`.

More configuration and software architecture details will follow.
