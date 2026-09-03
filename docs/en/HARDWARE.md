# Hardware – Interruption Counter 3.1.0

Target: classic **ESP32 Dev Module / ESP32-WROOM-32(E)**.

Version 3.0.0 is a hardware reset compared with the old test builds. Do not reuse a 2.x wiring diagram.

## Pin map

| Function | ESP32 GPIO | Note |
|---|---:|---|
| Interruption button / DI1 | 13 | `INPUT_PULLUP`, active-low, switch to GND |
| I2C SDA – DS3231 + SH1106 | 21 | shared I2C bus |
| I2C SCL – DS3231 + SH1106 | 22 | shared I2C bus |
| DY-SV17F TXD/IO0 | 18 | DY TX → ESP RX |
| DY-SV17F RXD/IO1 | 19 | ESP TX → DY RX |
| DY-SV17F CON3/BUSY | 39 / VN | input-only, external pull-up required |

DI2–DI4 and DO1–DO4 are disabled in the current project profile.

## Button

```text
ESP32 GPIO13 / DI1 ---- button ---- GND
```

Only the press edge counts. The input uses pull-up, debounce and a minimal falling-edge latch.

## DS3231 + SH1106

Both modules share GPIO21/22. RTC address is `0x68`; the 128×64 OLED defaults to `0x3C`.

Make sure breakout-board I2C pull-ups do not pull SDA/SCL to 5 V.

## DY-SV17F

UART2: **9600 baud, 8N1**.

```text
DY-SV17F TXD/IO0 ---- GPIO18
DY-SV17F RXD/IO1 ---- GPIO19
DY-SV17F GND -------- ESP32 GND
DY-SV17F V5 --------- stable 5-V supply
```

UART mode at power-up requires CON3 high and CON1/CON2 low. Connect CON3/BUSY to GPIO39 and add an external ~10 kΩ pull-up to the module's V33 output. GPIO39 has no internal pull-up.

BUSY low means playback active; high means idle.

## Audio files and internal flash

The DY-SV17F decodes **MP3 and WAV** and contains **32 Mbit / 4 MB internal flash**. Its integrated **5 W Class-D amplifier** can directly drive a **4 Ω, roughly 3–5 W speaker**. Documented sample rates are **8 / 11.025 / 12 / 16 / 22.05 / 24 / 32 / 44.1 / 48 kHz**, with a 24-bit DAC, about 90 dB dynamic range and 85 dB SNR.

Project track mapping:

- `00001.mp3` / `00001.wav`: track 1, boot sound only
- `00002` and above: interruption sounds
- rotate mode: detected tracks 2…N

Starter pack: [`../sounds/`](../sounds/)
File mapping: [`../sounds/DATEIZUORDNUNG.txt`](../sounds/DATEIZUORDNUNG.txt)

### Copying files over Micro-USB

1. Connect the DY-SV17F using a real **Micro-USB data cable**; a charge-only cable is not sufficient.
2. Open the module's internal flash drive on the computer.
3. Copy files **directly into the root directory**; do not use subfolders.
4. Use five digits with leading zeroes: `00001.mp3`, `00002.mp3`, …; WAV works the same way. Do not keep two formats with the same track number at once.
5. Safely eject the drive and disconnect Micro-USB.

**Important:** normal audio playback is unavailable while the DY-SV17F is connected to the computer / its flash is being used through USB. Disconnect USB before testing boot or interruption playback.

Check licensing before publishing your own audio files.

All modules need a common ground. ESP32 GPIO is 3.3-V logic; never feed 5-V signals directly into ESP32 inputs.

Full technical wiring: [`../../Unterbrechungszaehler/HARDWARE_WIRING.md`](../../Unterbrechungszaehler/HARDWARE_WIRING.md).

## OLED settings from 3.2.0

The SH1106 can be persistently rotated by 180°. Fresh defaults are 65% normal brightness and 5% dim brightness. The boot screen remains visible for at least four seconds without blocking the remaining device services.
