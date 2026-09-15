# Interruption Counter 3.6.0

> [!WARNING]
> **AI notice:** This project was created with substantial AI support, then tested, revised and developed further. If you fundamentally dislike AI-generated code, you are of course still allowed to press the button. ;-)

Version 3.0.0 remains the hard baseline of the project. Version 3.6.0 adds a physical work-cycle model without changing the proven 9-byte interruption record or the existing analytics.

[Deutsch](../de/README.md) · [Schwäbisch](../swg/README.md) · [Project home](../../README.md)

## New in 3.6.0: work cycle

The existing **DI1/GPIO13** button now distinguishes the workday boundaries without another pin or another button:

- **first short press:** START / begin work – not counted as an interruption
- **later short presses:** the latest press remains a pending candidate
- **next valid short press:** confirms the previous candidate as a real interruption
- **long press for at least 2 seconds:** explicitly ends the work cycle
- **no long press:** at the local day change, the pending final short press becomes END and is not counted

In practical terms: **first press = start, last press = end, everything in between = interruption.**

After an explicit long-press end, the OLED shows **DONE FOR TODAY plus today's interruption count for 10 seconds**. Automatic day-change closure stays silent.

The web button remains independent and still creates an interruption immediately.

## Anti-spam

The fixed 10-second protection window remains in place for short physical presses. The START press opens that window as well. Further short presses inside it are discarded, trigger the existing track-2/OLED anti-spam feedback, and never affect stored data or analytics. A rejected press does not extend the window.

The long-press END action bypasses this short-press cooldown so the work cycle can always be closed deliberately.

## Main features

- DI1/GPIO13 physical capture plus an independent web button
- local START/END work cycle without additional hardware
- local web UI without a cloud dependency
- daily counter, last interruption and heatmaps
- count or average completed interval analytics
- Focus & quiet-time insights and work patterns
- streamed CSV export
- 100,000 confirmed interruptions in the unchanged 9-byte raw ring
- 2,300 daily aggregate slots
- DS3231 RTC
- SH1106 OLED with multiple views, brightness/dimming and 180° rotation
- DY-SV17F audio: track 1 boot/test, track 2 anti-spam, track 3+ normal interruption sounds
- OTA update
- UI in German, English, Italian, French, Swabian, Alb-Swabian and Upper Swabian

## Data compatibility

START and END are deliberately **not** written to the interruption raw ring. The currently open final short press is stored compactly in NVS; START/END are also written to a tiny separate cycle journal. Only confirmed interruptions reach the existing raw ring and daily aggregates.

This keeps existing 3.x data, heatmaps, CSV exports, focus analytics and source filters compatible. See the [storage format](../../Unterbrechungszaehler/STORAGE_FORMAT.md) for details.

## Hardware

The pin assignment is unchanged. Version 3.6.0 requires no additional button.

- DI1: GPIO13 to GND
- I2C SDA/SCL: GPIO21/22 for DS3231 + SH1106
- DY-SV17F UART: RX GPIO18, TX GPIO19
- DY-SV17F BUSY: GPIO39/VN with external pull-up

See [HARDWARE.md](HARDWARE.md).

## Software and flashing

See [SOFTWARE.md](SOFTWARE.md).

## Technical documentation

- [Sketch documentation](../../Unterbrechungszaehler/README.md)
- [Architecture](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Storage format](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Time architecture](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Test report](../../Unterbrechungszaehler/TEST_REPORT.md)
- [Release notes](../../Unterbrechungszaehler/RELEASE_NOTES.md)

## License

MIT. Use it, modify it, extend it and build something of your own. If it somehow becomes a multi-million-dollar product one day, I will still be happy to receive a postcard.
