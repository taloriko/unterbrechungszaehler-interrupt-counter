# Flashing

This guide applies to **ESP32 Dev Module / ESP32-WROOM-32**.

## Arduino IDE

Install Arduino IDE 2.x and the **esp32 by Espressif Systems** board package.

Recommended settings:

```text
Board:            ESP32 Dev Module
Upload Speed:     115200
CPU Frequency:    240 MHz
Flash Frequency:  80 MHz
Flash Mode:       QIO
Flash Size:       4 MB
Partition Scheme: Default 4 MB
```

## Wi-Fi credentials

Inside `arduino/Unterbrechungszaehler/`:

1. Copy `Secrets.example.h` to `Secrets.h`.
2. Enter SSID and password.

`Secrets.h` is excluded through `.gitignore`.

## Upload

Open:

```text
arduino/Unterbrechungszaehler/Unterbrechungszaehler.ino
```

Connect the ESP32 using a USB data cable, select the correct port and upload the sketch.

Serial monitor baud rate:

```text
115200
```

## Web interface

Normal access:

```text
http://unterbrechungen.local
```

Fallback access without normal Wi-Fi:

```text
Wi-Fi: Unterbrechungszaehler
URL:   http://192.168.4.1
```

If mDNS is unavailable, use the IP address shown in the serial monitor.
