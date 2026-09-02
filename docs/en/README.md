# Interruption Counter 3.0.0

> [!WARNING]
> **AI notice:** This project was created with substantial help from AI, then tested, revised and developed further in real-world use.

[Deutsch](../de/README.md) · [Project home](../../README.md)

## What is this?

A local ESP32-based interruption counter: press a physical button or the web button, store the event, and analyse it later in the browser. No cloud service and no administrative ritual for every interruption.

Version **3.0.0** is a clean new baseline. Earlier 1.x/2.x releases were development/test builds and are not treated as migration targets.

## Main features

- physical interruption input on GPIO13 / DI1
- local web UI with live daily count
- three heatmap views and CSV export
- binary ring buffer for 100,000 raw events
- daily aggregate ring for 2,300 days
- DS3231 RTC and SH1106 OLED
- DY-SV17F audio module with boot and interruption tracks
- OTA firmware update
- NVS-backed sound/display preferences
- German, English and Swabian UI

## Current wiring

| Function | ESP32 |
|---|---:|
| Interruption button / DI1 | GPIO13 to GND |
| I2C SDA – RTC + OLED | GPIO21 |
| I2C SCL – RTC + OLED | GPIO22 |
| DY-SV17F TX → ESP32 RX | GPIO18 |
| ESP32 TX → DY-SV17F RX | GPIO19 |
| DY-SV17F CON3/BUSY | GPIO39 / VN |

CON3/BUSY needs an external ~10 kΩ pull-up to the DY-SV17F V33 pin. CON1 and CON2 are tied low for UART mode.

## Quick start

1. Read [Hardware](HARDWARE.md).
2. Read [Software / Build / Flashing](SOFTWARE.md).
3. Set your local Wi-Fi values in `Unterbrechungszaehler/config.h`.
4. Open `Unterbrechungszaehler/Unterbrechungszaehler.ino` in Arduino IDE.
5. Select **ESP32 Dev Module**, compile and flash.
6. Press the button. In most offices, finding test interruptions should not be difficult. ;-)

## Technical documentation

- [Sketch README](../../Unterbrechungszaehler/README.md)
- [Hardware wiring](../../Unterbrechungszaehler/HARDWARE_WIRING.md)
- [Architecture](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Storage format](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Time architecture](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Test report](../../Unterbrechungszaehler/TEST_REPORT.md)

## Screenshots

Final screenshots will be added under `docs/images/`. Until then, the documentation intentionally avoids broken image references.

## License

MIT License.
