# Hardware – Interruption Counter 3.0.0

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

## Audio tracks

- Track 1: boot sound
- Track 2 and above: interruption sounds
- Rotate mode uses detected tracks 2…N

Check licensing before publishing audio files.

All modules need a common ground. ESP32 GPIO is 3.3-V logic; never feed 5-V signals directly into ESP32 inputs.

Full technical wiring: [`../../Unterbrechungszaehler/HARDWARE_WIRING.md`](../../Unterbrechungszaehler/HARDWARE_WIRING.md).
