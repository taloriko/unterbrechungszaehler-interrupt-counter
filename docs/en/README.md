# Interrupt Counter

[🇩🇪 Deutsche Dokumentation](../../README.md)

> [!WARNING]
> **AI notice:** This project was created to a significant extent with the help of AI and then practically tested and further developed. If you fundamentally do not want to use AI-generated code, you can stop here.

## What is it?

The Interrupt Counter is an ESP32-based module for **simple event recording and analysis**.

It was originally built to make interruptions during the workday measurable:

**Press the button → store the event → analyze it later.**

Date and time are recorded automatically and evaluated in a local web interface.

## Why I built it

A button, because apparently **“I keep getting interrupted” is not a KPI yet.**

The project grew out of workdays where it became difficult to finish a single train of thought. Statements such as “I get interrupted very often” are hard to quantify, while counts, daily trends and heatmaps make the problem much easier to see.

The main requirement was simple:

**Recording an interruption must be easy enough to use even on a chaotic day.**

No form. No smartphone. No reason selection. One button press is enough.

## More than an interruption counter

The push button is only one possible use case.

The ESP32 input reacts to a dry contact, so the module can also record events from:

- push buttons
- switches
- relay contacts
- door contacts
- fault contacts
- operating feedback contacts
- machine contacts

This means the project can also be used as a general **event or contact counter**.

---

# Build guide

## 1. Get the hardware

→ [Hardware](HARDWARE.md)

## 2. Assemble the hardware

→ [Assembly](ASSEMBLY.md)

## 3. Software

→ [Software](SOFTWARE.md)

## 4. Flash the ESP32

→ [Flashing](FLASHING.md)

## 5. Normal operation

→ [Normal operation](NORMAL-OPERATION.md)

## 6. Standalone operation

For mobile operation using a battery, USB power bank or other mobile 5 V supply.

→ [Standalone operation](STANDALONE-OPERATION.md)

---

# Features

- **Event recording**
  - physical button or dry contact
  - virtual button in the web interface
  - short press stores an event
  - long press deletes the latest event
  - LED feedback

- **Analysis**
  - current day
  - history
  - weekday/hour heatmap
  - detailed event list
  - time gaps between events

- **Web interface**
  - hosted locally on the ESP32
  - responsive layout
  - automatic light/dark mode
  - available via `unterbrechungen.local`

- **Export**
  - CSV export
  - date and time included in the export filename

- **Time**
  - automatic NTP synchronization
  - automatic CET/CEST handling
  - configurable primary NTP server
  - fallback NTP servers

- **Storage**
  - binary ring buffer
  - up to 10,000 normal events
  - oldest event is overwritten when full

- **Device information**
  - ESP32, Wi-Fi, RAM, Flash and LittleFS information
  - graphical storage usage
  - software version

- **Standalone mode – BETA**
  - operation from battery or power bank
  - separate storage for up to 10,000 records
  - event recording without valid NTP time
  - Wi-Fi, mDNS and web server disabled
  - CPU reduced to 80 MHz
  - Light Sleep between inputs
