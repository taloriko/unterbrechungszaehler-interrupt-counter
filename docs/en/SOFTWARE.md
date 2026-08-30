# Software

Firmware location:

```text
arduino/Unterbrechungszaehler/
```

Main sketch:

```text
Unterbrechungszaehler.ino
```

## Configuration

General hardware and storage parameters are defined in `Config.h`.

For Wi-Fi credentials:

1. Copy `Secrets.example.h` to `Secrets.h`.
2. Enter SSID and password in `Secrets.h`.
3. Do not publish `Secrets.h`; it is excluded through `.gitignore`.

## Time sources

Priority:

1. NTP
2. optional DS3231 RTC
3. browser time as a one-time fallback
4. relative session time in standalone mode

Normal events are stored only when a valid absolute time is available.

## Storage

- normal ring buffer: 10,000 events
- long-term ring buffer: 100,000 events
- standalone ring buffer: 10,000 records
- statistics cache for heatmaps

Data is stored in LittleFS. Large data sets are processed in blocks.

## Network

Normal access:

```text
http://unterbrechungen.local
```

When the configured Wi-Fi is unavailable, the ESP32 provides a local access point:

```text
SSID: Unterbrechungszaehler
IP:   192.168.4.1
URL:  http://192.168.4.1
```

The fallback access point is disabled after the normal Wi-Fi connection is established.

## Optional hardware

- DS3231 RTC: `0x68`
- SH1106 OLED 128 × 64: `0x3C` or `0x3D`

Both modules are detected during startup. Missing optional hardware does not prevent basic event recording.

## Web interface

The web interface provides daily view, history, details, heatmaps, exports, device status and settings.

Language packs are registered centrally in `WebUiBehavior.h`. Every registered language uses the same key set and is validated by `tools/check_translations.py`.

## Architecture

See the [software architecture](../de/SOFTWARE-ARCHITEKTUR.md) for the module structure.
