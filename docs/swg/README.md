# Unterbrechungszähler 3.1.0 – Schwäbisch

> [!WARNING]
> **KI-Hinweis:** Des Projekt isch mit ordentlich KI-Unterstützung entstanden, danach aber in echt getestet, verbessert ond weitergebaut worda. Wenn du KI-generierten Code grundsätzlich net leiden kasch, darfst trotzdem dr Knopf drucka. ;-)

> **Aktueller Stand:** `3.1.0`

[Deutsch](../../README.md) · [English](../en/README.md) · [Projektstartseite](../../README.md)

---

## Für was brauch i des Ding?

**Du sitzsch konzentriert an dr Arbeit.**

- Dann kommt a Kollege.
- Dann klingelt s Telefon.
- Dann braucht oiner „bloß ganz kurz“ ebbes.
- Dann kommt dr Chef.
- Ond irgendwann fragsch di, was du vor zwoi Stunda eigentlich no macha wolltsch.

Genau dafür gibt s dr **Unterbrechungszähler**.

**Knopf drucka → Zeitpunkt speichera → später im Browser auswerta.**

Dann isch s nemme bloß so a Gfühl wie:

> „Heit bin i irgendwie zu nix komma.“

Sondern du kasch tatsächlich seh, **wie oft ond wann du unterbrocha worda bisch**.

> [!WARNING]
> Ob des dr Chef nachher wirklich interessiera tut, isch natürlich a ganz andere wissenschaftliche Frag. ;-)

## Kann des Ding no mehr?

Ja.

Dr Unterbrechungszähler isch technisch net auf an Taster beschränkt.

Statt em Taster kasch praktisch jeden geeigneten **potentialfreien Kontakt** neh.

Zum Beispiel:

- Störmeldung von ere Maschine über Relaiskontakt
- Unterbrechung von ere Lichtschranke
- Tür- oder Fensterkontakt
- Schaltkontakt von ere Anlage
- Betriebs- oder Fehlermeldunga
- Kontakt von em externen Taster
- oder sonst irgend ebbes, wo du später wissa willsch: **Wann isch des eigentlich passiert?**

Kurz gsagt:

**Kontakt schaltet → Ereignis wird gspeichert → Daten werdet sichtbar gmacht.**

Sei kreativ.

Wenn des Projekt irgendwann oiner nimmt, um Kühlschranköffnunga zu zähla, will i des allerdings wissa.

---

## Warum hab i des gmacht?

Über d Jahre hat sich bei ons im Betrieb einiges verändert.

- Mir hen mehr Mitarbeiter kriegt.
- Dann mehr Aufgaben.
- Dann no mehr Mitarbeiter.
- Dann no mehr Aufgaben.

Ond wenn d wirtschaftliche Lage schwieriger wird, kennt wahrscheinlich jeder des bewährte Managementkonzept:

- **No mehr Aufgaben.**

Irgendwann hab i des Gfühl ghabt, i krieg koi klaren Gedanken mehr zam – geschweige denn, dass i a Aufgabe von vielleicht 60 Minuta einmal konzentriert ond ohne Unterbrechung fertig krieg.

S Problem:

In em größeren Betrieb reicht

> „So kann i net vernünftig schaffa.“

irgendwann als Argument halt nemme unbedingt.

Also isch d Idee komma, dem System ebbes zurückzugeben, was große Unternehmen besonders gern hend:

**Daten, Statistika, Dokumentation ond Auswertunga.**

Oder anders gsagt:

I dreh d Bürokratie- ond Dokumentationswut einfach gegen s System. ;-)

Weil mei Arbeitstag sowieso scho genug Chaos hat, durfte des natürlich net au no zu ere zusätzlichen Verwaltungsaufgabe werda.

D wichtigste Anforderung war deshalb von Anfang an:

**Oin Knopfdruck. Fertig.**

Koine App aufmacha.
Koi Formular ausfülla.
Koine Kategorie auswähla.
Koine Excel-Lischte pflega.

Einfach drucka ond weiter schaffa.

Dr Rest macht s Gerät.

---

## Was kann s Gerät?

- **Ereignisse per Taster oder potentialfreiem Kontakt erfassa**
  Schnell, simpel ond ohne jedes Mal an Verwaltungsakt draus zu macha.

- **Lokale Weboberfläche ohne Cloud**
  Sehr wichtig. Net alles muss erst über drei Rechenzentren laufa, bloß damit ma an Knopf zähla kann. ;-)

- **Tagesansicht, Verlauf, Details ond umschaltbare Heatmaps**
  D Heatmaps zeiget entweder d **Anzahl von de Unterbrechunga** oder dr **durchschnittlich abgeschlossene Abstand bis zur nächste Unterbrechung am selba Tag**. Dr letzte Druck vom Tag zählt beim Durchschnitt net mit – ohne nächste Druck isch dr Abstand halt no net fertig.

- **CSV-Export ond Langzeit-Ringspeicher**
  Für dr Moment, wenn aus „I werd ständig unterbrocha“ plötzlich „Zeig mir d Daten“ wird.

- **DS3231-RTC**
  Damit s Gerät au ohne WLAN weiß, wie spät s isch. Revolutionäre Technik.

- **Optionales SH1106-OLED mit 128 × 64 Pixel**
  Braucha tut ma s net zwingend, aber s sieht sofort mindestens 37 % professioneller aus. S Display kann dauerhaft ei- oder ausgschaltet werda; beim echte Boot bleibt s Startbild mindestens zwoi Sekunda sichtbar.

- **Fallback-WLAN für lokalen Zugriff**
  Wenn ma koi Cloud will, sollt ma s Gerät trotzdem no erreicha könna. Dr Fallback-AP isch mit `Unterbrechungszähler` gschützt.

- **Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch ond Oberschwäbisch in dr Oberfläche**
  D README-Dokumentation gibt s absichtlich bloß auf Deutsch, Englisch ond Schwäbisch. Internationalisierung muss ja irgendwo anfanga.

- **MagSafe-Ring für Akku oder Halterunga**
  Klettband funktioniert zwar, aber Magnete sehet halt mehr nach Zukunft aus.

## 3.0.0 isch a harter Schnitt

D bisherigen 1.x/2.x-Stände warad Entwicklungs- ond Teststände. **3.0.0 isch dr neue Ausgangspunkt.** Es gibt deshalb koi zugesicherte Hardware-, Daten- oder OTA-Migration von 2.x. Wer von em alten Testaufbau kommt, verdrahtet nach dr aktuellen 3.0.0-Dokumentation neu.

## Aktuelle Pinbelegung

| Funktion | ESP32 |
|---|---:|
| Unterbrechungstaster / DI1 | GPIO13 gegen GND |
| I2C SDA – RTC + OLED | GPIO21 |
| I2C SCL – RTC + OLED | GPIO22 |
| DY-SV17F TX → ESP32 RX | GPIO18 |
| ESP32 TX → DY-SV17F RX | GPIO19 |
| DY-SV17F CON3/BUSY | GPIO39 / VN |

Für CON3/BUSY braucht s an externen ca. **10-kΩ-Pull-up an V33 vom DY-SV17F**. CON1 ond CON2 lieget für dr UART-Modus auf GND. Details: [Hardware / Wiring](../../Unterbrechungszaehler/HARDWARE_WIRING.md).

## DY-SV17F: Tön aufs Modul kopiera

S **DY-SV17F** hot **32 Mbit / 4 MByte internen Flash** für **MP3** ond **WAV**. Im Projekt läuft s Modul seriell über **UART mit 9600 Baud, 8N1**. S fertige Startpaket liegt unter [`../sounds/`](../sounds/), d Zuordnung steht in [`../sounds/DATEIZUORDNUNG.txt`](../sounds/DATEIZUORDNUNG.txt).

1. S Modul mit em **echte Micro-USB-Datenkabel** an Rechner hänga – a reines Ladekabel langt net.
2. Dr interne Speicher vom DY-SV17F am Rechner aufmacha.
3. D Dateien **direkt ins Hauptverzeichnis/Root** kopiera, net in Unterordner.
4. Fünfstellig benenna: `00001.mp3`, `00002.mp3`, `00003.mp3`, …; WAV geht entsprechend genauso.
5. `00001` isch bloß dr **Boot-Ton**. Ab `00002` send d Unterbrechungstön; Rotation nimmt Track **2…N**.
6. Speicher sauber auswerfa ond Micro-USB abzieha, bevor dr Ton getestet wird.

> [!IMPORTANT]
> **Solang s DY-SV17F per Micro-USB am Rechner hängt beziehungsweise dr Speicher über USB benutzt wird, kommt koi normale Soundausgabe.** Also erscht auswerfa, USB abzieha ond dann testa.

## Schnellstart

1. [Hardware ond Verdrahtung](../../Unterbrechungszaehler/HARDWARE_WIRING.md)
2. [Software, Build ond Flashen](../de/SOFTWARE.md)
3. WLAN-Platzhalter in `Unterbrechungszaehler/config.h` lokal anpassa.
4. `Unterbrechungszaehler/Unterbrechungszaehler.ino` in dr Arduino IDE aufmacha.
5. **ESP32 Dev Module** auswähla, kompiliera ond flasha.
6. Dr Knopf drucka. Wenn di keiner unterbricht, war dr Aufbau vielleicht a bissle zu erfolgreich.

## Technische Dokumentation

- [Sketch-Dokumentation](../../Unterbrechungszaehler/README.md)
- [Hardware-Wiring](../../Unterbrechungszaehler/HARDWARE_WIRING.md)
- [Architektur](../../Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Speicherformat](../../Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Zeitarchitektur](../../Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Testbericht](../../Unterbrechungszaehler/TEST_REPORT.md)
- [Changelog](../../CHANGELOG.md)

## Screenshots

![Home mit Tageszähler ond Feedback/Display](../images/3.0.0/de/de-home-1.png)

![Home mit Tageszähler ond Feedback/Display](../images/3.0.0/de/de-home-2.png)

![Auswertung mit Heatmap/Display](../images/3.0.0/de/de-auswertung-1.png)

![Auswertung mit Heatmap/Display](../images/3.0.0/de/de-auswertung-2.png)

![Auswertung mit Heatmap/Display](../images/3.0.0/de/de-auswertung-3.png)

![Auswertung mit Heatmap/Display](../images/3.0.0/de/de-auswertung-4.png)

![Einstellungen](../images/3.0.0/de/de-einstellungen-1.png)

![Gerät](../images/3.0.0/de/de-geraet-1.png)

![Gerät](../images/3.0.0/de/de-geraet-2.png)

![Gerät](../images/3.0.0/de/de-geraet-3.png)

![Gerät](../images/3.0.0/de/de-geraet-4.png)

## Warum Schwäbisch?

Weil technische Projekte net immer komplett ernst sei müsset.

Software darf funktioniera **ond** trotzdem a bissle Persönlichkeit hend.

I mag Schwäbisch ond wollt außerdem irgendwo a kleines Easter Egg einbaua.

Drum gibt s d Oberfläche au auf Schwäbisch – ond inzwischen sogar zusätzlich als Alb-Schwäbisch ond Oberschwäbisch.

Ob des d internationale Verbreitung vom Projekt beschleunigt oder massiv behindert, wird d Zukunft zeiga.

## Lizenz

MIT. Benutza, ändra, erweitera ond ebbes Eigenes draus baua isch ausdrücklich erlaubt. Wenn daraus irgendwann a millionenschweres Produkt wird, freu i mi immer no über a Postkarte.

GitHub: [taloriko](https://github.com/taloriko)
