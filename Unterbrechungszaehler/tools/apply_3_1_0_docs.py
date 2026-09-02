#!/usr/bin/env python3
from pathlib import Path


def read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    if old not in text:
        raise SystemExit(f"marker missing in {path}: {old[:80]!r}")
    if text.count(old) != 1:
        raise SystemExit(f"marker not unique in {path}: {old[:80]!r}")
    write(path, text.replace(old, new, 1))


# Root README.
path = "README.md"
text = read(path)
text = text.replace("> **Aktueller Stand:** `3.0.1`", "> **Aktueller Stand:** `3.1.0`", 1)
old = """- **Tagesansicht, Verlauf, Details und Heatmaps**  
  Damit man schnell sieht, wann besonders viel los war – oder wann offenbar alle anderen Mittagspause hatten."""
new = """- **Tagesansicht, Verlauf, Details und umschaltbare Heatmaps**  
  Die Heatmaps zeigen wahlweise die **Anzahl der Unterbrechungen** oder den **durchschnittlichen abgeschlossenen Abstand bis zur nächsten Unterbrechung am selben Tag**. Der letzte Druck eines Tages zählt beim Durchschnitt bewusst nicht mit – ohne nächsten Druck gibt es schließlich noch keinen abgeschlossenen Abstand."""
if old not in text:
    raise SystemExit("root heatmap feature marker missing")
text = text.replace(old, new, 1)
old = """- **Optionales SH1106-OLED mit 128 × 64 Pixeln**  
  Technisch nicht zwingend notwendig, sieht aber sofort mindestens 37 % professioneller aus."""
new = """- **Optionales SH1106-OLED mit 128 × 64 Pixeln**  
  Technisch nicht zwingend notwendig, sieht aber sofort mindestens 37 % professioneller aus. Das Display kann persistent ein- und ausgeschaltet werden; beim echten Geräteboot bleibt das Startbild mindestens zwei Sekunden sichtbar."""
if old not in text:
    raise SystemExit("root display feature marker missing")
text = text.replace(old, new, 1)
marker = "## Schnellstart\n"
if marker not in text:
    raise SystemExit("root quickstart marker missing")
sound = """## DY-SV17F: Sounddateien aufspielen

Das **DY-SV17F** besitzt **32 Mbit / 4 MByte internen Flash-Speicher** und kann **MP3** und **WAV** direkt wiedergeben. Das Modul bietet außerdem einen integrierten **5-W-Class-D-Verstärker**, an den ein **4-Ω-Lautsprecher mit etwa 3–5 W** direkt angeschlossen werden kann. Es unterstützt IO-, Serial- und One-Line-Serial-Betrieb; dieses Projekt verwendet **Serial/UART mit 9600 Baud, 8N1**.

Unterstützte Abtastraten laut Moduldaten: **8 / 11.025 / 12 / 16 / 22.05 / 24 / 32 / 44.1 / 48 kHz**. Angegeben sind außerdem ein **24-Bit-DAC**, etwa **90 dB Dynamikbereich** und **85 dB Signal-Rausch-Verhältnis**.

Ein direkt nutzbares Startpaket liegt unter [`docs/sounds/`](docs/sounds/). Die aktuelle Zuordnung der mitgelieferten Dateien steht in [`docs/sounds/DATEIZUORDNUNG.txt`](docs/sounds/DATEIZUORDNUNG.txt).

So kommen eigene oder die mitgelieferten Töne auf das Modul:

1. DY-SV17F mit einem **echten Micro-USB-Datenkabel** an den Computer anschließen. Ein reines Ladekabel reicht nicht.
2. Den am Rechner eingebundenen internen Speicher des DY-SV17F öffnen.
3. Die Audiodateien **direkt ins Root-/Hauptverzeichnis** des Moduls kopieren. **Keine Unterordner verwenden.**
4. Dateien fünfstellig mit führenden Nullen benennen: `00001.mp3`, `00002.mp3`, `00003.mp3`, …; alternativ entsprechend `00001.wav` usw. Nicht gleichzeitig unterschiedliche Dateien mit derselben Tracknummer ablegen.
5. `00001` ist **Track 1 und ausschließlich der Boot-Ton**. `00002` und höher sind die Unterbrechungstöne. Im festen Modus spielt die Firmware den ausgewählten Track ab 2; im Rotationsmodus werden die erkannten Tracks **2…N** verwendet.
6. Datenträger anschließend sauber auswerfen und die Micro-USB-Verbindung zum Computer trennen.

Richtig:

```text
/00001.mp3
/00002.mp3
/00003.mp3
```

Falsch:

```text
/sounds/00001.mp3
/mp3/00002.mp3
```

> [!IMPORTANT]
> **Solange das DY-SV17F per Micro-USB mit dem Computer verbunden ist bzw. sein interner Speicher über USB verwendet wird, funktioniert die normale Soundausgabe nicht.** Nach dem Kopieren deshalb den Datenträger auswerfen, USB trennen und erst dann Soundtest, Boot-Ton oder Unterbrechungston prüfen.

"""
text = text.replace(marker, sound + marker, 1)
write(path, text)

# German docs index.
path = "docs/de/README.md"
text = read(path).replace("# Unterbrechungszähler 3.0.1", "# Unterbrechungszähler 3.1.0", 1)
text = text.replace("- Tageszähler, letzte Unterbrechung und Heatmaps", "- Tageszähler, letzte Unterbrechung und Heatmaps: Anzahl oder Ø abgeschlossener Abstand", 1)
text = text.replace("- SH1106 OLED", "- SH1106 OLED mit persistentem Ein/Aus-Schalter und mindestens 2 s Bootbild", 1)
text = text.replace("- DY-SV17F Soundmodul", "- DY-SV17F Soundmodul; Startpaket und USB-Kopieranleitung siehe [HARDWARE.md](HARDWARE.md)", 1)
write(path, text)

# English README.
path = "docs/en/README.md"
text = read(path)
text = text.replace("# Interruption Counter 3.0.1", "# Interruption Counter 3.1.0", 1)
text = text.replace("> **Current version:** `3.0.0`", "> **Current version:** `3.1.0`", 1)
old = """- **Daily view, history, details and heatmaps**  
  So you can quickly see when things were particularly busy – or when apparently everybody else was on lunch break."""
new = """- **Daily view, history, details and switchable heatmaps**  
  Heatmaps can show either the **number of interruptions** or the **average completed interval until the next interruption on the same day**. The final press of a day is deliberately excluded because, without another press, that interval is still open."""
if old not in text:
    raise SystemExit("English heatmap marker missing")
text = text.replace(old, new, 1)
old = """- **Optional SH1106 OLED with 128 × 64 pixels**  
  Technically not essential, but it instantly looks at least 37% more professional."""
new = """- **Optional SH1106 OLED with 128 × 64 pixels**  
  Technically not essential, but it instantly looks at least 37% more professional. The display can be persistently enabled or disabled; the real boot screen remains visible for at least two seconds."""
if old not in text:
    raise SystemExit("English display marker missing")
text = text.replace(old, new, 1)
marker = "## Quick start\n"
if marker not in text:
    raise SystemExit("English quickstart marker missing")
sound = """## DY-SV17F: copying audio files

The **DY-SV17F** provides **32 Mbit / 4 MB of internal flash** and decodes **MP3** and **WAV**. Its integrated **5 W Class-D amplifier** can directly drive a **4 Ω, roughly 3–5 W speaker**. The module supports IO, Serial and One-Line Serial modes; this project uses **Serial/UART at 9600 baud, 8N1**.

Documented sample rates are **8 / 11.025 / 12 / 16 / 22.05 / 24 / 32 / 44.1 / 48 kHz**, with a **24-bit DAC**, about **90 dB dynamic range** and **85 dB signal-to-noise ratio**.

A ready-to-copy starter pack is available in [`../sounds/`](../sounds/); the included file mapping is listed in [`../sounds/DATEIZUORDNUNG.txt`](../sounds/DATEIZUORDNUNG.txt).

1. Connect the DY-SV17F to the computer with a real **Micro-USB data cable**. A charge-only cable is not sufficient.
2. Open the module's internal flash drive on the computer.
3. Copy the audio files **directly to the root directory**. Do **not** use folders.
4. Use five-digit names with leading zeroes: `00001.mp3`, `00002.mp3`, `00003.mp3`, … or the corresponding `.wav` names. Do not keep two different files with the same track number.
5. `00001` is **track 1 and reserved exclusively for the boot sound**. `00002` and above are interruption sounds. Fixed mode uses the configured track >= 2; rotate mode uses detected tracks **2…N**.
6. Safely eject the drive and disconnect Micro-USB before testing playback.

Correct:

```text
/00001.mp3
/00002.mp3
/00003.mp3
```

Wrong:

```text
/sounds/00001.mp3
/mp3/00002.mp3
```

> [!IMPORTANT]
> **Normal audio playback does not work while the DY-SV17F is connected to the computer by Micro-USB / its flash is in USB storage use.** Eject the drive and disconnect USB before testing the sound output.

"""
text = text.replace(marker, sound + marker, 1)
write(path, text)

# Swabian README.
path = "docs/swg/README.md"
text = read(path)
text = text.replace("# Unterbrechungszähler 3.0.1 – Schwäbisch", "# Unterbrechungszähler 3.1.0 – Schwäbisch", 1)
text = text.replace("> **Aktueller Stand:** `3.0.0`", "> **Aktueller Stand:** `3.1.0`", 1)
old = """- **Tagesansicht, Verlauf, Details ond Heatmaps**  
  Damit ma schnell sieht, wann besonders viel los war – oder wann offenbar alle andere Mittag gmacht hend."""
new = """- **Tagesansicht, Verlauf, Details ond umschaltbare Heatmaps**  
  D Heatmaps zeiget entweder d **Anzahl von de Unterbrechunga** oder dr **durchschnittlich abgeschlossene Abstand bis zur nächste Unterbrechung am selba Tag**. Dr letzte Druck vom Tag zählt beim Durchschnitt net mit – ohne nächste Druck isch dr Abstand halt no net fertig."""
if old not in text:
    raise SystemExit("Swabian heatmap marker missing")
text = text.replace(old, new, 1)
old = """- **Optionales SH1106-OLED mit 128 × 64 Pixel**  
  Braucha tut ma s net zwingend, aber s sieht sofort mindestens 37 % professioneller aus."""
new = """- **Optionales SH1106-OLED mit 128 × 64 Pixel**  
  Braucha tut ma s net zwingend, aber s sieht sofort mindestens 37 % professioneller aus. S Display kann dauerhaft ei- oder ausgschaltet werda; beim echte Boot bleibt s Startbild mindestens zwoi Sekunda sichtbar."""
if old not in text:
    raise SystemExit("Swabian display marker missing")
text = text.replace(old, new, 1)
marker = "## Schnellstart\n"
if marker not in text:
    raise SystemExit("Swabian quickstart marker missing")
sound = """## DY-SV17F: Tön aufs Modul kopiera

S **DY-SV17F** hot **32 Mbit / 4 MByte internen Flash** für **MP3** ond **WAV**. Im Projekt läuft s Modul seriell über **UART mit 9600 Baud, 8N1**. S fertige Startpaket liegt unter [`../sounds/`](../sounds/), d Zuordnung steht in [`../sounds/DATEIZUORDNUNG.txt`](../sounds/DATEIZUORDNUNG.txt).

1. S Modul mit em **echte Micro-USB-Datenkabel** an Rechner hänga – a reines Ladekabel langt net.
2. Dr interne Speicher vom DY-SV17F am Rechner aufmacha.
3. D Dateien **direkt ins Hauptverzeichnis/Root** kopiera, net in Unterordner.
4. Fünfstellig benenna: `00001.mp3`, `00002.mp3`, `00003.mp3`, …; WAV geht entsprechend genauso.
5. `00001` isch bloß dr **Boot-Ton**. Ab `00002` send d Unterbrechungstön; Rotation nimmt Track **2…N**.
6. Speicher sauber auswerfa ond Micro-USB abzieha, bevor dr Ton getestet wird.

> [!IMPORTANT]
> **Solang s DY-SV17F per Micro-USB am Rechner hängt beziehungsweise dr Speicher über USB benutzt wird, kommt koi normale Soundausgabe.** Also erscht auswerfa, USB abzieha ond dann testa.

"""
text = text.replace(marker, sound + marker, 1)
write(path, text)

# Hardware docs.
path = "docs/de/HARDWARE.md"
text = read(path).replace("# Hardware – Unterbrechungszähler 3.0.1", "# Hardware – Unterbrechungszähler 3.1.0", 1)
old = """## Sounddateien

- Track 1: Boot-Ton
- Track 2 und höher: Unterbrechungstöne
- Im Rotationsmodus werden erkannte Tracks 2…N verwendet.

Die konkrete Dateibenennung auf dem Datenträger richtet sich nach dem vom DY-SV17F unterstützten Dateisystem/Sortierverhalten. Vor dem öffentlichen Verteilen eigener Audiodateien deren Lizenz/Herkunft prüfen.
"""
new = """## Sounddateien und interner Flash

Das DY-SV17F dekodiert **MP3 und WAV** und besitzt **32 Mbit / 4 MByte internen Flash**. Der integrierte **5-W-Class-D-Verstärker** kann einen **4-Ω-Lautsprecher mit etwa 3–5 W** direkt treiben. Unterstützte Abtastraten: **8 / 11.025 / 12 / 16 / 22.05 / 24 / 32 / 44.1 / 48 kHz**. Angegeben sind außerdem 24-Bit-DAC, ca. 90 dB Dynamikbereich und 85 dB Signal-Rausch-Verhältnis.

Das Projekt verwendet folgende Trackbelegung:

- `00001.mp3` bzw. `00001.wav`: Track 1, ausschließlich Boot-Ton
- `00002` und höher: Unterbrechungstöne
- Rotationsmodus: erkannte Tracks 2…N

Startpaket: [`../sounds/`](../sounds/)  
Dateizuordnung: [`../sounds/DATEIZUORDNUNG.txt`](../sounds/DATEIZUORDNUNG.txt)

### Dateien per Micro-USB übertragen

1. DY-SV17F mit einem **Micro-USB-Datenkabel** mit dem Computer verbinden. Ein Ladekabel ohne Datenleitungen funktioniert dafür nicht.
2. Den internen Speicher am Computer öffnen.
3. Dateien **direkt ins Root-/Hauptverzeichnis** kopieren. Keine Unterordner anlegen.
4. Fünfstellig mit führenden Nullen benennen: `00001.mp3`, `00002.mp3`, …; WAV entsprechend. Keine zwei Formate mit derselben Tracknummer gleichzeitig verwenden.
5. Datenträger sauber auswerfen und Micro-USB trennen.

```text
richtig: /00001.mp3
         /00002.mp3

falsch:  /sounds/00001.mp3
         /mp3/00002.mp3
```

**Wichtig:** Solange der DY-SV17F per Micro-USB mit dem Computer verbunden ist bzw. sein Flash über USB benutzt wird, steht die normale Soundausgabe nicht zur Verfügung. Erst USB trennen, dann Boot-/Unterbrechungston oder Hardwaretest prüfen.

Vor dem öffentlichen Verteilen eigener Audiodateien deren Lizenz/Herkunft prüfen.
"""
if old not in text:
    raise SystemExit("German hardware sound block missing")
text = text.replace(old, new, 1)
write(path, text)

path = "docs/en/HARDWARE.md"
text = read(path).replace("# Hardware – Interruption Counter 3.0.1", "# Hardware – Interruption Counter 3.1.0", 1)
old = """## Audio tracks

- Track 1: boot sound
- Track 2 and above: interruption sounds
- Rotate mode uses detected tracks 2…N

Check licensing before publishing audio files.
"""
new = """## Audio files and internal flash

The DY-SV17F decodes **MP3 and WAV** and contains **32 Mbit / 4 MB internal flash**. Its integrated **5 W Class-D amplifier** can directly drive a **4 Ω, roughly 3–5 W speaker**. Documented sample rates are **8 / 11.025 / 12 / 16 / 22.05 / 24 / 32 / 44.1 / 48 kHz**, with a 24-bit DAC, about 90 dB dynamic range and 85 dB SNR.

Project track mapping:

- `00001.mp3` / `00001.wav`: track 1, boot sound only
- `00002` and above: interruption sounds
- rotate mode: detected tracks 2…N

Starter pack: [`../sounds/`](../sounds/)  
File mapping: [`../sounds/DATEIZUORDNUNG.txt`](../sounds/DATEIZUORDNUNG.txt)

### Copying files over Micro-USB

1. Connect the DY-SV17F using a real **Micro-USB data cable**; a charge-only cable is not sufficient.
2. Open the module's internal flash drive on the computer.
3. Copy files **directly into the root directory**; do not use subfolders.
4. Use five digits with leading zeroes: `00001.mp3`, `00002.mp3`, …; WAV works the same way. Do not keep two formats with the same track number at once.
5. Safely eject the drive and disconnect Micro-USB.

**Important:** normal audio playback is unavailable while the DY-SV17F is connected to the computer / its flash is being used through USB. Disconnect USB before testing boot or interruption playback.

Check licensing before publishing your own audio files.
"""
if old not in text:
    raise SystemExit("English hardware sound block missing")
text = text.replace(old, new, 1)
write(path, text)

# Technical sketch documentation.
path = "Unterbrechungszaehler/README.md"
text = read(path)
text = text.replace("# Unterbrechungszähler 3.0.1 – technische Übersicht", "# Unterbrechungszähler 3.1.0 – technische Übersicht", 1)
text = text.replace("Dies ist der Sketchordner der Version **3.0.1**", "Dies ist der Sketchordner der Version **3.1.0**", 1)
old = "Statistiken lesen Tagesaggregate und nicht bei jeder Ansicht den vollständigen Raw-Ring."
new = "Die Anzahl-Heatmaps lesen weiterhin die kompakten Tagesaggregate. Die neue Metrik **Ø Abstand** wird nur auf Anforderung aus den noch vorhandenen Rohereignissen berechnet: Ein gültiger Abstand wird der Start-Unterbrechung zugeordnet; der letzte Druck eines Kalendertages bleibt ausgeschlossen und über Mitternacht wird nie ein Intervall gebildet."
if old not in text:
    raise SystemExit("technical analytics marker missing")
text = text.replace(old, new, 1)
old = "Track 1 ist Boot-Ton. Unterbrechungen verwenden einen festen Track ab 2 oder rotieren über erkannte Tracks 2…N. Sound, Displayflash, Anzeigeart, Helligkeit und Dimmer werden persistent gespeichert."
new = "Track 1 ist Boot-Ton. Unterbrechungen verwenden einen festen Track ab 2 oder rotieren über erkannte Tracks 2…N. Sound, Display-Master, Displayflash, Anzeigeart, Helligkeit und Dimmer werden persistent gespeichert. Das OLED-Bootbild bleibt mindestens 2 Sekunden sichtbar, ohne den Projektloop zu blockieren. Die Sounddateien und USB-Kopieranleitung stehen in [`../docs/de/HARDWARE.md`](../docs/de/HARDWARE.md)."
if old not in text:
    raise SystemExit("technical sound/display marker missing")
text = text.replace(old, new, 1)
write(path, text)

# Architecture.
path = "Unterbrechungszaehler/PROJECT_ARCHITECTURE.md"
text = read(path)
text = text.replace("# Projektarchitektur – Unterbrechungszähler 0.2.0", "# Projektarchitektur – Unterbrechungszähler 3.1.0", 1)
text = text.replace("Nur **DI1/GPIO13** ist für 0.2.0 aktiviert.", "Nur **DI1/GPIO13** ist im Projektprofil 3.1.0 aktiviert.", 1)
text = text.replace("## Projektpräferenzen 0.2.0", "## Projektpräferenzen 3.1.0", 1)
old = "Die drei Heatmaps lesen den Tagesaggregatring, nicht den Raw-Ring. Beim initialen/automatischen Auswertungsrefresh werden Storageinfo und alle drei Matrizen über **einen kombinierten Endpunkt** geliefert; bei gültiger Systemzeit werden alle drei Heatmaps dabei in einem gemeinsamen Tagesring-Durchlauf aufgebaut. Nur gezielte Filterbuttons verwenden die kleineren Einzelendpunkte. Lange Aggregatbesuche bedienen alle 32 Records zusätzlich den generischen Hardwareinputpfad und `serviceUrgent()` und geben mit `delay(0)` an den Scheduler zurück. Damit kann ein physischer Tastendruck auch während einer größeren Statistikantwort zeitnah erfasst/feedbacket werden."
new = """Die drei Heatmaps besitzen zwei Metriken. **Anzahl** liest weiterhin den Tagesaggregatring. **Ø Abstand** wird dagegen nur auf Anforderung aus den retained Rohereignissen aufgebaut, weil dort absolute Zeit und `deltaSeconds` vorhanden sind. Dabei werden ausschließlich unmittelbar aufeinanderfolgende retained Events mit gültiger absoluter Zeit, demselben lokalen Kalendertag und plausibler positiver Differenz verwendet. Der Messwert wird der Start-Unterbrechung zugeordnet; dadurch hat der letzte Druck jedes Tages automatisch kein Sample. Ein fehlendes/überschriebenes Vorgängerevent wird nie überbrückt.

Beim kombinierten Analytics-Endpunkt werden für Ø Abstand Summe und Samplezahl je Zelle in einem Raw-Ring-Durchlauf aufgebaut; der JSON-Payload liefert Durchschnitt, Samplezahl und Coverage. Ist der gewählte Zeitraum älter als die retained Rohdaten, kennzeichnet die UI die Abdeckung als unvollständig statt fehlende Werte als Null auszugeben. Lange Statistikbesuche bedienen weiterhin periodisch den generischen Hardwareinputpfad und `serviceUrgent()` und geben mit `delay(0)` an den Scheduler zurück."""
if old not in text:
    raise SystemExit("architecture analytics marker missing")
text = text.replace(old, new, 1)
text = text.replace("`ProjectPreferences` ist die einzige persistente Quelle für Unterbrechungston und OLED-Projektanzeige.", "`ProjectPreferences` ist die einzige persistente Quelle für Unterbrechungston und OLED-Projektanzeige einschließlich Display-Master-Schalter.", 1)
write(path, text)

# Test report.
path = "Unterbrechungszaehler/TEST_REPORT.md"
text = read(path)
text = text.replace("# Testbericht – Unterbrechungszähler 3.0.0", "# Testbericht – Unterbrechungszähler 3.1.0", 1)
text = text.replace("- Projektname und Version 3.0.0", "- Projektname und Version 3.1.0", 1)
text = text.replace("- 14/14 Storage-/Recovery-/Heatmap-Simulationen bestanden", "- 16/16 Storage-/Recovery-/Heatmap-Simulationen bestanden, einschließlich Ø-Abstandssemantik und Raw-Ring-Coverage", 1)
text = text.replace("- SH1106: Standard-/Zahl-/Letzte-Unterbrechung-Ansicht, Helligkeit und Flash", "- SH1106: Standard-/Zahl-/Letzte-Unterbrechung-Ansicht, Helligkeit, Flash, Display Ein/Aus und Bootbild >= 2 s", 1)
text = text.replace("- Heatmaps auf Desktop und schmalem Mobilgerät", "- Heatmaps Anzahl/Ø Abstand auf Desktop und schmalem Mobilgerät; letzter Tagesdruck und Mitternachtsgrenze praktisch prüfen", 1)
text = text.replace("3.0.0 darf nach erfolgreicher GitHub-CI als Software-Release veröffentlicht werden.", "3.1.0 darf nach erfolgreicher GitHub-CI als Software-Release veröffentlicht werden.", 1)
write(path, text)

# Changelog.
path = "CHANGELOG.md"
text = read(path)
if "## 3.1.0" in text:
    raise SystemExit("3.1.0 changelog already exists")
marker = "# Changelog\n\n"
if marker not in text:
    raise SystemExit("changelog heading marker missing")
entry = """## 3.1.0

- Heatmaps zwischen **Anzahl** und **Ø Abstand bis zur nächsten Unterbrechung** umschaltbar
- Ø-Abstand ausschließlich aus gültigen, unmittelbar aufeinanderfolgenden retained Rohereignissen desselben lokalen Tages
- letzter Druck eines Tages und Übergänge über Mitternacht bewusst aus der Durchschnittsberechnung ausgeschlossen
- Samplezahl und Raw-Ring-Coverage für die Ø-Abstandsansicht; kurze Abstände werden in der Heatmap stärker gewichtet
- persistenter Display-Master-Schalter analog zur Soundeinstellung
- SH1106-Bootbild mindestens 2 Sekunden sichtbar, nicht blockierend; Display-Aus wird danach respektiert
- manueller Displaytest kehrt zuverlässig zur Benutzeranzeige bzw. zu Display-Aus zurück
- DY-SV17F-Micro-USB-/Root-Verzeichnis-/Dateibenennungsdokumentation ergänzt
- Sound-Startpaket unter `docs/sounds/` dokumentiert
- Hosttests auf 16 Szenarien erweitert; Webbundle und Releasechecks für 3.1.0 aktualisiert

"""
write(path, text.replace(marker, marker + entry, 1))

# Release notes.
path = "Unterbrechungszaehler/RELEASE_NOTES.md"
text = read(path)
if text.startswith("# Release Notes – Unterbrechungszähler 3.1.0"):
    raise SystemExit("3.1.0 release notes already exist")
entry = """# Release Notes – Unterbrechungszähler 3.1.0

3.1.0 erweitert die Auswertung und Displaysteuerung, ohne das bestehende Raw-Record-Format oder die 100.000er Ringspeicherkapazität zu ändern.

## Neu

- Heatmap-Metrik **Anzahl** oder **Ø Abstand**
- abgeschlossene Intervalle werden der Start-Unterbrechung zugeordnet
- letzter Tagesdruck sowie Intervalle über Mitternacht werden nicht verwendet
- Ø-Abstand basiert auf retained Rohereignissen und weist unvollständige Coverage aus
- Display persistent ein-/ausschaltbar
- Bootscreen mindestens zwei Sekunden sichtbar, ohne `delay(2000)`
- DY-SV17F-Soundpaket und Micro-USB-Kopieranleitung dokumentiert

## Datenmodell

Das 9-Byte-Raw-Format und der 64-Byte-Tagesaggregate-Record bleiben unverändert. Anzahl-Heatmaps verwenden die Tagesaggregate; Ø-Abstand scannt nur bei Bedarf den retained Raw-Ring und bildet `sum(intervalSeconds) / sampleCount` je Zelle.

---

"""
write(path, entry + text)

print("3.1.0 documentation patch applied")
