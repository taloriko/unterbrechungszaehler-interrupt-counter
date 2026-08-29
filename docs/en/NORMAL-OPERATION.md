# Normal operation

> **Status: Work in progress**

In normal operation the ESP32 runs with Wi-Fi, NTP, mDNS and the local web interface enabled.

## Recording events

- short button press: store an event
- long press of about 3 seconds: delete the latest event
- the virtual button in the web interface can also be used

Important: In normal operation an event is stored **only when a valid absolute time is available**.

## LED feedback

Feedback is provided by the LED on **GPIO2**. On some ESP32 boards this pin may be labeled **D2** or connected to the onboard LED.

- 1x short: event stored
- 3x fast: latest event deleted
- 2x fast + 2x slow: warning, for example missing time, connection or storage problem

## Web interface on the normal Wi-Fi network

After connecting to Wi-Fi, the interface is normally available at:

```text
http://unterbrechungen.local
```

If mDNS is not available, the IP address shown in the serial monitor can be entered directly in the browser.

Example:

```text
http://192.168.1.123
```

The actual IP address depends on the network in use.

## Local access without normal Wi-Fi

While the configured Wi-Fi network is still unavailable, the ESP32 creates an additional open hotspot:

```text
Wi-Fi: Unterbrechungszaehler
Address: http://192.168.4.1
```

Procedure:

1. Connect the smartphone to the Wi-Fi network `Unterbrechungszaehler`.
2. Open `http://192.168.4.1` in the browser.
3. The web interface can be used locally immediately.

The fallback hotspot is automatically disabled as soon as the ESP32 connects to its configured Wi-Fi network.

## Using the smartphone time

If NTP has not supplied a valid time yet, the web interface automatically tries once to send the current time of the smartphone/browser to the ESP32.

The value is accepted only if the ESP32 does **not already have a valid time**.

Therefore the normal operating rule remains:

**No valid time = no event is stored.**

After the smartphone time has been accepted, events can be recorded locally. When NTP becomes available, normal NTP synchronization continues.

## Current sections

- Today
- History
- Heatmap
- Details
- Export
- Device
- Standalone **BETA**

More usage details will follow.
