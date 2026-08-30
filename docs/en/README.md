# Interrupt Counter

[🇩🇪 Deutsche Dokumentation](../../README.md)

## Function

The Interrupt Counter is an ESP32-based module for recording, storing and analyzing events.

**Trigger a push button or dry contact → store the event → analyze it locally.**

The input can also be connected to relay, door, fault, operating feedback or machine contacts.

## Features

- event recording by physical or virtual button
- automatic date and time via NTP
- optional **DS3231 RTC** as a local time source
- optional **1.3 inch SH1106 OLED 128 × 64**
- automatic hardware detection at startup
- local web interface hosted on the ESP32
- daily overview, history and detailed event view
- weekday/hour heatmap with year, ISO week and time-range selection
- month/week heatmap with year selection
- year/month heatmap
- CSV export
- light/dark mode and German/English UI
- fallback Wi-Fi access point when the configured network is unavailable
- serial diagnostics at `115200 baud`
- standalone mode for battery or power-bank operation

## Time sources

The device uses time sources in this order:

1. **NTP** – reference time when a network connection is available
2. **RTC** – local startup time and time source without NTP
3. **browser time** – fallback while the device has no valid time
4. **relative time** – standalone sessions without an absolute time source

A valid RTC can initialize system time during startup. After a successful NTP synchronization, the RTC is updated from system time.

## Optional hardware

RTC and OLED share the I2C bus:

- `GPIO21` = SDA
- `GPIO22` = SCL
- `3.3 V` supply

Supported devices:

- DS3231 RTC at `0x68`
- SH1106 OLED 128 × 64 at `0x3C` or `0x3D`

Both modules are detected at startup. Missing optional hardware does not disable the basic event-recording function.

In standalone mode the DS3231 can provide time without Wi-Fi. A detected OLED shows startup status for 15 seconds and is then switched off.

## Storage

- normal ring buffer: **10,000 events**
- long-term ring buffer: **100,000 events**
- standalone ring buffer: **10,000 records**
- persistent statistics cache for heatmaps
- block-based reads for larger data sets to reduce filesystem operations and RAM peaks

The normal ring buffer remains the primary event store. A long-term storage failure does not prevent writing to the normal ring buffer.

## Access

On the configured Wi-Fi network:

`http://unterbrechungen.local`

The ESP32 IP address can also be used directly, for example:

`http://192.168.178.50`

If the configured Wi-Fi network is unavailable, the ESP32 starts a local fallback access point:

- SSID: `Unterbrechungszaehler`
- address: `http://192.168.4.1`

## Build guide

1. [Hardware](HARDWARE.md)
2. [Assembly](ASSEMBLY.md)
3. [Software](SOFTWARE.md)
4. [Flashing](FLASHING.md)
5. [Normal operation](NORMAL-OPERATION.md)
6. [Standalone operation](STANDALONE-OPERATION.md)

## Web interface

Tabs:

`Today · History · Heatmap · Details · Export · Device · Settings · Standalone`

The **Device** tab shows technical state and resource information including Wi-Fi, uptime, firmware, RAM, flash, LittleFS and ring-buffer usage.

The **Settings** tab contains general display and time settings.

Heatmap filters are located directly next to the corresponding analysis.

## License

MIT
