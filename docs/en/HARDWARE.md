# Hardware

> **Status: Work in progress**

Currently tested hardware:

- **ESP32 Dev Module / ESP32-WROOM-32D**  
  Example: [AliExpress – ESP32 WROOM-32D Development Board](https://a.aliexpress.com/_EvOzo6A)
- **dry-contact push button**, e.g. Eaton M22  
  Example: [Amazon – M22 push button](https://amzn.eu/d/0dexlafu)
- **optional dry-contact slide switch** for standalone mode  
  Example: [Reichelt – miniature slide switch, 1x changeover](https://www.reichelt.de/de/de/shop/produkt/schiebeschalter-miniatur_loetanschluss_1x_um-19975)
- **optional DS3231 RTC module with AT24C32**
  - RTC I2C address: `0x68`
  - use **3.3 V** with the ESP32
  - provides local time in standalone mode without Wi-Fi/NTP
  - automatically detected once during boot
  - check the battery/charging circuit of generic modules; a normal CR2032 must not be charged
- **optional 1.3 inch OLED, 128 × 64, SH1106, I2C, white**
  - recommended module: AZDelivery 1.3 inch SH1106
  - typical I2C address `0x3C`, alternatively `0x3D`
  - use **3.3 V** with the ESP32
  - automatically detected once during boot
  - in standalone mode it shows boot/RTC status for 15 seconds and then switches off
- **USB data cable** for flashing
- **5 V power supply** for normal operation
  - minimum **1 A**
  - **2 A recommended** to safely cover Wi-Fi current peaks
- **optional battery or USB power bank** for mobile operation
- **optional MagSafe-style adhesive magnetic ring** for magnetic enclosure mounting  
  Example: [Amazon – MagSafe adhesive ring](https://amzn.eu/d/0dy0oqH6)

## Optional I2C extensions

RTC and OLED share the same bus:

- `GPIO21` = SDA
- `GPIO22` = SCL
- both powered from `3.3 V` and `GND`

The firmware remains fully operational when either or both modules are missing. Missing modules are shown greyed out in the web interface.

## 3D-printed enclosure

A 3D-printed enclosure is planned for this project.

The enclosure design includes the option to install a **MagSafe-style adhesive magnetic ring** on the underside, allowing the device to be attached magnetically and removed easily.

→ [3D printing / enclosure](../../hardware/3d/README.md)

> The actual STL/3MF file will be added later.

More details about terminals and mechanical assembly will follow.
