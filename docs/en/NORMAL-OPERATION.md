# Normal operation

> **Status: Work in progress**

In normal operation the ESP32 runs with Wi-Fi, NTP, mDNS and the local web interface enabled.

## Recording events

- short button press: store an event
- long press of about 3 seconds: delete the latest event
- the virtual button in the web interface can also be used

## LED feedback

- 1x short: event stored
- 3x fast: latest event deleted
- 2x fast + 2x slow: warning, for example missing time, connection or storage problem

## Web interface

After connecting to Wi-Fi, the interface is normally available at:

```text
http://unterbrechungen.local
```

If mDNS is not available, use the IP address shown in the serial monitor.

Current sections:

- Today
- History
- Heatmap
- Details
- Export
- Device
- Standalone **BETA**

More usage details will follow.
