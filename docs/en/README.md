# Interruption Counter 3.0.1

> [!WARNING]
> **AI notice:** This project was created with substantial support from AI, then tested in practice, revised and developed further. If you fundamentally dislike AI-generated code, you are of course still allowed to press the button. ;-)

> **Current version:** `3.0.0`

[Deutsch](../../README.md) · [Schwäbisch](../swg/README.md) · [Project home](../../README.md)

---

## What do I need this thing for?

**You are sitting at your desk, focused on a task.**

- Then a colleague shows up.  
- Then the phone rings.  
- Then somebody needs “just one quick thing”.  
- Then the boss appears.  
- And at some point you start wondering what you were actually trying to do two hours ago.

That is exactly what the **Interruption Counter** is for.

**Press the button → store the timestamp → analyse it later in the browser.**

So instead of it just feeling like:

> “I somehow got nothing done today.”

You can actually see **how often and when you were interrupted**.

> [!WARNING]
> Whether your boss is interested in the result afterwards is, of course, an entirely different scientific question. ;-)

## Can this thing do anything else?

Yes.

Technically, the Interruption Counter is not limited to a push button.

Instead of the button, you can use practically any suitable **potential-free / dry contact**.

For example:

- a machine fault signal via relay contact
- interruption of a light barrier
- a door or window contact
- a switching contact from a system
- operating or fault messages
- an external push button contact
- or any other contact where you later want to know: **When exactly did that happen?**

In short:

**Contact switches → event is stored → data is visualised.**

Be creative.

If somebody eventually uses the project to count how often the refrigerator is opened, though, I would like to hear about it.

---

## Why did I build this?

Over the years, quite a few things changed in our company.

- We got more employees.
- Then more tasks.
- Then even more employees.
- Then even more tasks.

And when the economic situation gets more difficult, everybody probably knows the well-established management concept:

- **Even more tasks.**

At some point I felt as if I could no longer form a clear thought – let alone finish a task that might take 60 minutes in one focused session without being interrupted.

The problem:

In a larger company,

> “I cannot work properly like this.”

eventually stops being a sufficiently convincing argument on its own.

So the idea was to give back something that large organisations particularly love:

**Data, statistics, documentation and reports.**

Or, put differently:

I am simply turning the bureaucracy and documentation obsession back against the system. ;-)

Since my working day already contains enough chaos, this was obviously not allowed to become yet another administrative task.

The most important requirement from the very beginning was therefore:

**One button press. Done.**

No opening an app.  
No filling out a form.  
No choosing a category.  
No maintaining an Excel sheet.

Just press the button and keep working.

The device handles the rest.

---

## What can the device do?

- **Capture events using a push button or potential-free contact**  
  Simple, fast and without turning every interruption into an administrative procedure.

- **Local web interface without a cloud dependency**  
  Very important. Not everything needs to travel through three data centres just so somebody can count a button press. ;-)

- **Daily view, history, details and heatmaps**  
  So you can quickly see when things were particularly busy – or when apparently everybody else was on lunch break.

- **CSV export and long-term ring storage**  
  For the moment when “I keep getting interrupted” turns into “Show me the data”.

- **DS3231 RTC**  
  So the device still knows what time it is without Wi-Fi. Revolutionary technology.

- **Optional SH1106 OLED with 128 × 64 pixels**  
  Technically not essential, but it instantly looks at least 37% more professional.

- **Fallback Wi-Fi for local access**  
  If you do not want a cloud, you should still be able to reach the device somehow. The fallback AP is protected with the password `Unterbrechungszähler`.

- **German, English, Italian, French, Swabian, Alb-Swabian and Upper Swabian in the user interface**  
  The README documentation intentionally remains available in German, English and Swabian only. Internationalisation has to start somewhere.

- **MagSafe ring for a battery pack or mounting accessories**  
  Because Velcro works, but magnets simply look more like the future.

## 3.0.0 is a hard cut

The previous 1.x/2.x versions were development and test builds. **3.0.0 is the new baseline.** There is therefore no guaranteed hardware, data or OTA migration from 2.x. If you are coming from an older test setup, rebuild the wiring according to the current 3.0.0 documentation.

## Current pin assignment

| Function | ESP32 |
|---|---:|
| Interruption button / DI1 | GPIO13 to GND |
| I2C SDA – RTC + OLED | GPIO21 |
| I2C SCL – RTC + OLED | GPIO22 |
| DY-SV17F TX → ESP32 RX | GPIO18 |
| ESP32 TX → DY-SV17F RX | GPIO19 |
| DY-SV17F CON3/BUSY | GPIO39 / VN |

CON3/BUSY requires an external approx. **10 kΩ pull-up to the DY-SV17F V33 pin**. CON1 and CON2 are tied to GND for UART mode. Details: [Hardware / Wiring](HARDWARE.md).

## Quick start

1. [Hardware and wiring](HARDWARE.md)
2. [Software, build and flashing](SOFTWARE.md)
3. Adjust the Wi-Fi placeholders in `Unterbrechungszaehler/config.h` locally.
4. Open `Unterbrechungszaehler/Unterbrechungszaehler.ino` in the Arduino IDE.
5. Select **ESP32 Dev Module**, compile and flash.
6. Press the button. If nobody interrupts you, the setup may have worked a little too well.

## Technical documentation

- [Sketch documentation](../../Unterbrechungszaehler/README.md)
- [Hardware wiring](../../Unterbrechungszaehler/HARDWARE_WIRING.md)
- [Architecture](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Storage format](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Time architecture](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Test report](../../Unterbrechungszaehler/TEST_REPORT.md)
- [Changelog](../../CHANGELOG.md)

## Screenshots

![Home with daily counter and feedback/display](../images/3.0.0/de/de-home-1.png)

![Home with daily counter and feedback/display](../images/3.0.0/de/de-home-2.png)

![Analytics with heatmap/display](../images/3.0.0/de/de-auswertung-1.png)

![Analytics with heatmap/display](../images/3.0.0/de/de-auswertung-2.png)

![Analytics with heatmap/display](../images/3.0.0/de/de-auswertung-3.png)

![Analytics with heatmap/display](../images/3.0.0/de/de-auswertung-4.png)

![Settings](../images/3.0.0/de/de-einstellungen-1.png)

![Device](../images/3.0.0/de/de-geraet-1.png)

![Device](../images/3.0.0/de/de-geraet-2.png)

![Device](../images/3.0.0/de/de-geraet-3.png)

![Device](../images/3.0.0/de/de-geraet-4.png)

## Why Swabian?

Because technical projects do not always have to be completely serious.

Software can work **and** still have a little personality.

I like Swabian and also wanted to hide a small Easter egg somewhere.

So the user interface is available in Swabian – and now also in Alb-Swabian and Upper Swabian variants.

Whether that accelerates the international adoption of the project or massively gets in the way remains to be seen.

## License

MIT. You are explicitly allowed to use it, modify it, extend it and build something of your own from it. If it somehow turns into a multi-million-dollar product one day, I will still be happy to receive a postcard.

GitHub: [taloriko](https://github.com/taloriko)
