# Normal operation

In normal operation the ESP32 uses Wi-Fi, time management, mDNS and the local web interface.

## Recording events

- short button press: store an event
- long press of about 3 seconds: delete the latest event
- alternatively use the virtual button in the web interface

Normal events are stored only when a valid absolute time is available.

## LED feedback

The feedback LED is connected to GPIO2. Some ESP32 boards label this pin D2 or use it for the onboard LED.

- 1x short: event stored
- 3x fast: latest event deleted
- 2x fast + 2x slow: warning, for example missing time or storage problem

## Web interface

Normal access:

```text
http://unterbrechungen.local
```

Alternatively use the IP address shown in the serial monitor.

## Fallback Wi-Fi

While the configured Wi-Fi is unavailable:

```text
Wi-Fi:   Unterbrechungszaehler
Address: http://192.168.4.1
```

The fallback access point is disabled after the normal Wi-Fi connection is established.

## Time without NTP

If neither NTP nor a valid RTC provides time, the web interface may transfer the browser time once.

An already valid device time is not overwritten by this fallback.

## Web interface sections

- Today
- History
- Heatmap
- Details
- Export
- Device
- Settings
- Standalone

Exports provide the stored data as CSV. Device shows network, storage and hardware state. Settings contains language, appearance, RTC, NTP and display functions.
