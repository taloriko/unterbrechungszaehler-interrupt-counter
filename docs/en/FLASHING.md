# Flashing

> **Status: Work in progress**

This guide applies to the tested **ESP32 Dev Module / ESP32-WROOM-32**.

## Arduino IDE

Install Arduino IDE 2.x and add the Espressif ESP32 board package.

Recommended settings:

```text
Board:            ESP32 Dev Module
Upload Speed:     115200
CPU Frequency:    240 MHz
Flash Frequency:  80 MHz
Flash Mode:       QIO
Flash Size:       4 MB
Partition Scheme: Default 4MB with spiffs
```

Copy `Secrets.example.h` to `Secrets.h` and enter your Wi-Fi credentials.

Open:

```text
arduino/UnterbrechungszaehlerInterruptCounter/UnterbrechungszaehlerInterruptCounter.ino
```

Connect the ESP32 using a USB data cable, select the correct COM port and upload the sketch.

Serial monitor baud rate: **115200**.

After a successful start, the web interface should normally be available at:

```text
http://unterbrechungen.local
```

More troubleshooting information will follow.
