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
- local fallback access point while the configured Wi-Fi is unavailable
- fixed local access via `http://192.168.4.1`
- automatic browser/device time handover while no valid device time exists
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

## Local fallback access

While the configured Wi-Fi network is not connected, the ESP32 also starts its own open access point:

```text
SSID: Unterbrechungszaehler
IP:   192.168.4.1
URL:  http://192.168.4.1
```

This allows the web interface to be opened directly from a smartphone even before the normal Wi-Fi connection is available.

The fallback access point is automatically disabled as soon as the configured Wi-Fi connection has been established.

## Time without NTP in normal operation

Normal operation still records events **only when a valid absolute time is available**.

If NTP has not provided a valid time yet and the web interface is opened from a smartphone or browser, the page sends the current Unix time of that device to the ESP32 once.

The ESP32 accepts this value only while it does not already have a valid time. An already valid device time is never overwritten by the browser fallback.

When NTP becomes available, normal NTP synchronization continues to maintain the device time.

Standalone mode remains separate from this behavior and keeps using its own relative-time logic.

More configuration and software architecture details will follow.
