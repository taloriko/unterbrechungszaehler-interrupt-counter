# Software, Build and Flashing – 3.0.0

## Requirements

- Arduino IDE or Arduino CLI
- `esp32 by Espressif Systems`
- board: **ESP32 Dev Module**

The sketch lives in `Unterbrechungszaehler/`. Its custom `partitions.csv` is stored in the same folder.

## Wi-Fi

`Unterbrechungszaehler/config.h` intentionally contains placeholder credentials. Replace them locally and never commit real credentials.

## Build the web UI

After editing `ui-src/index.html`, `ui-src/app.css` or `ui-src/app.js`:

```bash
python3 Unterbrechungszaehler/tools/build_web.py
```

This regenerates `Unterbrechungszaehler/web_assets.h` deterministically.

## Portable release checks

```bash
python3 Unterbrechungszaehler/tools/release_check.py
```

The checker validates version metadata, translation parity, project API routes, storage simulations, JavaScript syntax and the generated gzip bundle.

## Compile / flash

1. Open `Unterbrechungszaehler/Unterbrechungszaehler.ino`.
2. Select **ESP32 Dev Module**.
3. Compile.
4. Verify that the firmware fits the 1,441,792-byte OTA app slot.
5. Flash and open Serial Monitor at 115200 baud.

GitHub Actions performs an additional release build with a pinned ESP32 core and publishes the OTA binary only after a successful build on `main`.

## OTA

3.0.0 uses a custom partition table with two OTA application slots and LittleFS. A normal OTA update writes the inactive application slot and should preserve NVS/LittleFS.

Because 3.0.0 is a hard reset from the old test builds, a direct OTA upgrade from 2.x is **not guaranteed**.

## Hardware verification after flashing

Test DI1/GPIO13, OLED, audio, RTC/NTP, CSV export and all heatmaps with real events. Remaining target-hardware tests are listed in `Unterbrechungszaehler/TEST_REPORT.md`.
