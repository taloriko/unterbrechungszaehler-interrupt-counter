# Hardware beschaffen

Für den Aufbau werden benötigt:

- **ESP32 Dev Module / ESP32-WROOM-32D**  
  Beispiel: [AliExpress – ESP32 WROOM-32D Development Board](https://a.aliexpress.com/_EvOzo6A)
- **potentialfreier Taster**, z. B. Eaton M22  
  Beispiel: [Amazon – M22 Taster](https://amzn.eu/d/0dexlafu)
- **optional: potentialfreier Schiebeschalter** für den Autark-Modus  
  Beispiel: [Reichelt – Miniatur-Schiebeschalter, 1x UM](https://www.reichelt.de/de/de/shop/produkt/schiebeschalter-miniatur_loetanschluss_1x_um-19975)
- **optional: DS3231 RTC-Modul mit AT24C32**
  - I2C-Adresse `0x68`
  - Betrieb mit **3,3 V**
  - lokale Zeitquelle ohne WLAN/NTP
  - automatische Erkennung beim Boot
  - bei Modulen mit Ladeschaltung auf den Batterietyp achten; eine normale CR2032 darf nicht geladen werden
- **optional: 1,3 Zoll OLED, 128 × 64, SH1106, I2C, weiß**
  - getestete Bauform: AZDelivery 1,3 Zoll SH1106
  - I2C-Adresse `0x3C` oder `0x3D`
  - Betrieb mit **3,3 V**
  - automatische Erkennung beim Boot
- **USB-Datenkabel** zum Flashen
- **5-V-Stromversorgung**
  - mindestens **1 A**
  - **2 A empfohlen** für sichere WLAN-Stromspitzen
- **optional: Akku oder USB-Powerbank** für mobilen Betrieb
- **optional: MagSafe-/Magnet-Klebering**  
  Beispiel: [Amazon – MagSafe Klebering](https://amzn.eu/d/0dy0oqH6)

## I2C

RTC und OLED teilen sich denselben Bus:

- `GPIO21` = SDA
- `GPIO22` = SCL
- `3,3 V` und `GND`

Fehlende optionale Module werden beim Boot erkannt und beeinträchtigen die grundlegende Ereigniserfassung nicht.

## Gehäuse

Für eine magnetische Befestigung kann ein MagSafe-/Magnet-Klebering auf der Unterseite des Gehäuses verwendet werden.

→ [3D-Druck / Gehäuse](../../hardware/3d/README.md)

Im Repository ist derzeit keine druckfertige STL-/3MF-Datei enthalten.
