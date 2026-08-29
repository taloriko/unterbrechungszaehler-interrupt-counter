# Hardware beschaffen

> **Status: In Arbeit**

Für den aktuell getesteten Aufbau werden benötigt:

- **ESP32 Dev Module / ESP32-WROOM-32D**  
  Beispiel: [AliExpress – ESP32 WROOM-32D Development Board](https://a.aliexpress.com/_EvOzo6A)
- **potentialfreier Taster**, z. B. Eaton M22  
  Beispiel: [Amazon – M22 Taster](https://amzn.eu/d/0dexlafu)
- **optional: potentialfreier Schiebeschalter** für den Autark-Modus  
  Beispiel: [Reichelt – Miniatur-Schiebeschalter, 1x UM](https://www.reichelt.de/de/de/shop/produkt/schiebeschalter-miniatur_loetanschluss_1x_um-19975)
- **optional: DS3231 RTC-Modul mit AT24C32**
  - I2C-Adresse RTC: `0x68`
  - Betrieb am ESP32 mit **3,3 V**
  - dient im Autark-Modus als Zeitquelle ohne WLAN/NTP
  - die Firmware erkennt das Modul beim Boot automatisch
  - bei Modulen mit Batterieladeschaltung auf den verwendeten Batterietyp achten; eine normale CR2032 darf nicht geladen werden
- **optional: 1,3 Zoll OLED, 128 × 64, SH1106, I2C, weiß**
  - getestete/empfohlene Bauform: AZDelivery 1,3 Zoll SH1106
  - typische I2C-Adresse: `0x3C`, alternativ `0x3D`
  - Betrieb am ESP32 mit **3,3 V**
  - die Firmware erkennt das Display beim Boot automatisch
  - im Autark-Modus zeigt es beim Start für 15 Sekunden den Hardware-/Zeitstatus und schaltet danach ab
- **USB-Datenkabel** zum Flashen
- **5-V-Stromversorgung** für den normalen Betrieb
  - mindestens **1 A**
  - empfohlen **2 A**, damit auch WLAN-Stromspitzen sicher abgedeckt sind
- **optional für mobilen Betrieb:** Akku oder USB-Powerbank
- **optional: MagSafe-/Magnet-Klebering** für die magnetische Gehäusebefestigung  
  Beispiel: [Amazon – MagSafe Klebering](https://amzn.eu/d/0dy0oqH6)

## Optionale I2C-Erweiterungen

RTC und OLED teilen sich denselben I2C-Bus:

- `GPIO21` = SDA
- `GPIO22` = SCL
- Versorgung jeweils mit `3,3 V` und `GND`

Die Firmware bleibt auch ohne diese Erweiterungen vollständig lauffähig. Nicht vorhandene Module werden in der Weboberfläche ausgegraut dargestellt.

## 3D-gedrucktes Gehäuse

Für das Projekt ist ein 3D-gedrucktes Gehäuse vorgesehen.

Die Gehäusekonstruktion besitzt die Möglichkeit, auf der Unterseite einen **MagSafe-/Magnet-Klebering** einzusetzen. Damit kann das Gerät einfach magnetisch befestigt und wieder abgenommen werden.

→ [3D-Druck / Gehäuse](../../hardware/3d/README.md)

> Die eigentliche STL-/3MF-Datei wird noch ergänzt.

Weitere Empfehlungen zu Anschlussklemmen und mechanischen Details folgen.
