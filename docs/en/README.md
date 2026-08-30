# Interruption Counter

> [!WARNING]
> **AI notice:** This project was created with substantial help from AI, then tested, revised and developed further in real-world use.  
> If AI-generated code fundamentally offends your worldview, this is probably a good place to stop. Saves us both some time. ;-)

> **README version:** `1.0.1`

[Deutsch](../de/README.md) · [Schwäbisch](../swg/README.md) · [Project home](../../README.md)

---

## What do I need this thing for?

You are finally focused on a task.

Then a colleague shows up.  
Then the phone rings.  
Then someone needs “just one quick thing”.  
Then the boss appears.  
And at some point you start wondering what you were actually trying to do two hours ago.

That is exactly what the **Interruption Counter** is for.

**Press button → store timestamp → evaluate it later in your browser**

Now it is no longer just a vague feeling like:

> “I somehow got absolutely nothing done today.”

Instead, you can actually see **how often and when you were interrupted**.

Whether your boss is interested in that afterwards is, of course, an entirely different scientific question. ;-)

---

## Can it do more?

Yes.

Technically, the Interruption Counter is not limited to a push button.

Instead of the button, you can use practically any suitable **dry contact / potential-free contact**.

For example:

- machine fault indication via relay contact
- interruption of a light barrier
- door or window contact
- switching contact from a system
- operating or fault messages
- external push button contact
- or basically any other contact where you later want to know: **When did that actually happen?**

In short:

**Contact switches → event is stored → data is visualized.**

Be creative.

If somebody eventually uses this project to count how often the fridge is opened, however, I would like to hear about it.

---

## Why did I build this?

Over the years, quite a lot changed at our company.

We got more employees.  
Then more tasks.  
Then even more employees.  
Then even more tasks.

And when the economic situation gets difficult, everyone probably knows the proven management concept:

**More tasks.**

At some point I had the feeling that I could no longer form a clear thought, let alone finish a task that might take 60 minutes without being interrupted somewhere along the way.

The problem:

In a larger company,

> “I cannot work properly like this.”

is not always considered sufficient evidence anymore.

So I came up with the idea of giving large organizations something they truly love:

**Data, statistics, documentation and reports.**

Or, put differently:

I am simply using the obsession with bureaucracy and documentation against the system. ;-)

Since my workday already contains enough chaos, the solution was not allowed to create even more administrative work.

That led to the most important requirement from day one:

**One button press. Done.**

No opening an app.  
No filling out forms.  
No choosing a category.  
No maintaining an Excel sheet.

Just press the button and keep working.

The device handles the rest.

---

## What can the device do?

- **Record events via push button or dry contact**  
  Fast, simple and without turning every interruption into an administrative procedure.

- **Local web interface without cloud dependency**  
  Very important. Not everything needs to travel through three data centers just so a button press can be counted. ;-)

- **Daily view, history, details and heatmaps**  
  So you can quickly see when things got especially busy – or when everyone else was apparently on lunch break.

- **CSV export and long-term ring buffer**  
  Useful when “I get interrupted all the time” eventually turns into “show me the data”.

- **Optional DS3231 RTC**  
  So the device can still tell the time without Wi-Fi. Revolutionary technology.

- **Optional SH1106 OLED, 128 × 64 pixels**  
  Not technically necessary, but it instantly looks at least 37% more professional.

- **Fallback Wi-Fi for local access**  
  If you do not want cloud access, you should still be able to reach the device somehow.

- **Standalone mode using battery or power bank**  
  For places without a convenient power outlet – or if you eventually have to document interruptions while working on a deserted island.

- **German, English and Swabian user interface**  
  Internationalization has to start somewhere.

- **MagSafe ring for battery packs or mounts**  
  Velcro works, sure. Magnets simply look more like the future.

---

## Quick start

1. [Hardware](HARDWARE.md)
2. [Assembly](ASSEMBLY.md)
3. [Software configuration](SOFTWARE.md)
4. [Flashing](FLASHING.md)
5. [Normal operation](NORMAL-OPERATION.md)

After that, all you really need is someone to interrupt you.

In most offices, that part should not be particularly difficult.

---

## More documentation

- [Standalone operation](STANDALONE-OPERATION.md)
- [Software architecture](../de/SOFTWARE-ARCHITEKTUR.md)
- [Changes / Changelog](../../CHANGELOG.md)

---

## Screenshots

![Reiter Heute](docs/images/de-heute.png)

![Reiter Verlauf](docs/images/de-verlauf.png)

![Reiter Heatmap](docs/images/de-heatmap.png)

![Reiter Details](docs/images/de-details.png)

![Reiter Export](docs/images/de-export.png)

![Reiter Gerät](docs/images/de-gerät.png)

![Reiter Einstellungen 1](docs/images/de-einstellungen-1.png)

![Reiter Einstellungen 2](docs/images/de-einstellungen-2.png)

![Reiter Autark](docs/images/de-autark.png)

---

## Why Swabian?

Because technical projects do not always have to take themselves completely seriously.

Software can work properly **and** still have a bit of personality.

I like the Swabian dialect, and I also wanted a small Easter egg somewhere in the project.

So the interface is available in Swabian too.

Whether that accelerates the project's international adoption or completely destroys it remains to be seen.

---

## License

This project is released under the **MIT License**.

In short: use it, modify it, extend it and build your own version from it.

If it somehow turns into a multimillion-dollar product one day, a postcard would be appreciated.
