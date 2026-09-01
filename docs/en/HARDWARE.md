# Hardware

Required hardware:

- **ESP32 Dev Module / ESP32-WROOM-32D**
- **dry-contact push button**, e.g. Eaton M22
- **optional dry-contact slide switch** for standalone mode
- **optional DS3231 RTC module with AT24C32**
  - I2C address `0x68`
  - operate at **3.3 V**
  - automatically detected during boot
  - check the module charging circuit; a normal CR2032 must not be charged
- **optional 1.3 inch SH1106 OLED, 128 × 64, I2C**
  - address `0x3C` or `0x3D`
  - operate at **3.3 V**
  - automatically detected during boot
- **USB data cable** for flashing
- **5 V power supply**
  - minimum **1 A**
  - **2 A recommended**
- **optional battery or USB power bank**
- **optional MagSafe-style adhesive magnetic ring** for enclosure mounting

## I2C

RTC and OLED share:

- `GPIO21` = SDA
- `GPIO22` = SCL
- `3.3 V` and `GND`

Missing optional modules do not prevent basic event recording.

## Enclosure

The enclosure can use a MagSafe-style adhesive magnetic ring for removable magnetic mounting.

See [3D printing / enclosure](../../hardware/3d/README.md).

No print-ready STL/3MF file is currently included in the repository.
