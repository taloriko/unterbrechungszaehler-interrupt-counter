# Unterbrechungszähler / Interrupt Counter

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt, anschließend aber praktisch getestet, überarbeitet und weiterentwickelt. Wer KI-generierten Code grundsätzlich nicht mag, darf natürlich trotzdem den Taster drücken. ;-)

> **Aktueller Stand:** `3.6.0`

[Deutsch](docs/de/README.md) · [English](docs/en/README.md) · [Schwäbisch](docs/swg/README.md)

---

## Für was brauche ich das Ding?

Du sitzt konzentriert an einer Aufgabe. Dann kommt ein Kollege. Dann klingelt das Telefon. Dann braucht jemand „nur ganz kurz“ etwas. Und irgendwann fragst du dich, was du eigentlich vor zwei Stunden machen wolltest.

Genau dafür gibt es den **Unterbrechungszähler**.

Bisher war die Grundidee einfach:

**Taster drücken → Unterbrechung speichern → später im Browser auswerten.**

Ab Version 3.6.0 kommt eine wichtige Konkretisierung dazu: **Arbeitsbeginn und Arbeitsende sind keine Unterbrechungen.** Das Gerät erkennt deshalb einen lokalen Arbeitszyklus, ohne dass ein zweiter Taster oder zusätzlicher GPIO nötig wird.

> [!WARNING]
> Ob die Daten deinen Chef anschließend interessieren, ist natürlich weiterhin eine völlig andere wissenschaftliche Fragestellung. ;-)

## Neu in 3.6.0: Arbeitszyklus

Der vorhandene Taster auf **DI1/GPIO13** funktioniert jetzt so:

1. **Erster kurzer Druck:** START des Arbeitszyklus – wird nicht als Unterbrechung gezählt.
2. **Weitere kurze Drücke:** Der jeweils letzte Druck bleibt zunächst als Kandidat offen.
3. **Nächster gültiger kurzer Druck:** Der vorherige Kandidat wird endgültig als Unterbrechung gespeichert.
4. **Langer Druck ab 2 Sekunden:** Der Arbeitszyklus wird ausdrücklich beendet.
5. **Kein langer Druck:** Beim lokalen Tageswechsel wird der letzte offene Druck automatisch zum ENDE und nicht als Unterbrechung gezählt.

Kurz gesagt:

> **Erster Druck = Start. Letzter Druck = Ende. Alles dazwischen = Unterbrechung.**

Der wichtige Trick dabei: Der letzte kurze Druck wird nicht erst als Unterbrechung gespeichert und später wieder herausgerechnet. Er bleibt so lange ein kleiner persistenter Kandidat, bis klar ist, ob danach noch ein Druck kommt. Dadurch bleiben Tageszähler, Heatmaps, CSV, Fokus-Auswertungen und Durchschnittswerte von Anfang an sauber.

### Feierabend

Ein langer Druck von mindestens **2 Sekunden** beendet den Arbeitszyklus sofort. Ein davor noch offener kurzer Druck wird dabei als Unterbrechung bestätigt, weil der spätere lange Druck eindeutig das tatsächliche Ende markiert.

Danach zeigt das SH1106 für **10 Sekunden**:

```text
FEIERABEND

<Anzahl heute>
HEUTE UNTERBR.
```

Beim automatischen Tagesabschluss wird diese Anzeige bewusst nicht aktiviert.

## Anti-Spam bleibt erhalten

Kurze physische Tastendrücke haben weiterhin die feste **10-Sekunden-Sperre** aus 3.4.x:

- auch der START-Druck eröffnet die Sperre,
- weitere kurze Drücke innerhalb der Zeit werden nicht gespeichert oder gezählt,
- ein verworfener Druck verlängert die Sperre nicht,
- Track 2 und die OLED-TV-Störung bleiben das lokale Anti-Spam-Feedback,
- Web-Ereignisse bleiben unabhängig.

Der lange Feierabend-Druck ist davon ausgenommen. Sonst könnte man ausgerechnet dann nicht zuverlässig Schluss machen, wenn man gehen möchte.

## Datenkompatibilität

Die bestehende Unterbrechungsdatenbank bleibt in 3.6.0 bewusst unverändert:

- **100.000 Raw-Records × 9 Byte**
- **2.300 Tagesaggregate × 64 Byte**
- bestehende EventSource-Bits und CRC bleiben erhalten
- bestehende 3.x-Rohdaten benötigen keine Migration

START und ENDE werden **nicht** in den Unterbrechungs-Ring geschrieben. Der laufende Zyklus und der letzte offene Kandidat werden kompakt in NVS gehalten; zusätzlich gibt es das kleine separate `/cycles.log` für START-/END-Marken.

Damit bleiben alle bisherigen Unterbrechungs-Auswertungen kompatibel und zählen weiterhin ausschließlich echte Unterbrechungen.

Details: [Persistentes Datenformat](Unterbrechungszaehler/STORAGE_FORMAT.md)

## Was kann das Gerät sonst noch?

- **Lokale Weboberfläche ohne Cloud** – der ESP32 liefert die komplette Oberfläche selbst aus.
- **Heatmaps und Verlauf** – Anzahl oder durchschnittlich abgeschlossener Abstand, inklusive Herkunftsfilter GPIO/Web.
- **Fokus & Ruhe** – aktuelle Ruhephase, Tages-/Wochenbestwerte und 120-Minuten-Trend.
- **Arbeitsmuster** – beobachtete ruhige Zeitfenster und Unterbrechungsschwerpunkte.
- **CSV-Export** – wird beim Download aus dem Raw-Ring gestreamt.
- **DS3231-RTC** – Zeit auch ohne WLAN.
- **SH1106 OLED** – mehrere Ansichten, Display-Master, Helligkeit, Dimmer und 180°-Drehung.
- **DY-SV17F Sound** – Lautstärke, fester Track oder Rotation.
- **Fallback-WLAN** – lokaler Zugriff auch ohne vorhandenes WLAN.
- **OTA-Update** – mit eigener Partitionstabelle und Release-BIN.
- **UI-Sprachen** – Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch.

Die ausführliche README-Dokumentation wird weiterhin bewusst nur in **Deutsch, Englisch und Schwäbisch** gepflegt.

## Pinbelegung

| Funktion | ESP32 |
|---|---:|
| Unterbrechungstaster / Arbeitszyklus DI1 | GPIO13 gegen GND |
| I2C SDA – RTC + OLED | GPIO21 |
| I2C SCL – RTC + OLED | GPIO22 |
| DY-SV17F TX → ESP32 RX | GPIO18 |
| ESP32 TX → DY-SV17F RX | GPIO19 |
| DY-SV17F CON3/BUSY | GPIO39 / VN |

Für CON3/BUSY ist weiterhin ein externer ca. **10-kΩ-Pull-up an V33 des DY-SV17F** erforderlich. CON1 und CON2 liegen für den UART-Modus auf GND.

Details: [Hardware / Wiring](docs/de/HARDWARE.md)

## DY-SV17F Trackbelegung

- `00001` = Boot/Test
- `00002` = Anti-Spam
- `00003` und höher = normale Unterbrechungstöne

Die Dateien kommen direkt ins Root-Verzeichnis des DY-SV17F. Ein Startpaket liegt unter [`docs/sounds/`](docs/sounds/).

> [!IMPORTANT]
> Solange das DY-SV17F per Micro-USB als Datenträger am Computer hängt, funktioniert die normale Soundausgabe nicht. Datenträger sauber auswerfen, USB trennen und danach testen.

## Schnellstart

1. [Hardware und Verdrahtung](docs/de/HARDWARE.md)
2. [Software, Build und Flashen](docs/de/SOFTWARE.md)
3. WLAN-Platzhalter in `Unterbrechungszaehler/config.h` lokal anpassen.
4. `Unterbrechungszaehler/Unterbrechungszaehler.ino` in der Arduino IDE öffnen.
5. **ESP32 Dev Module** auswählen, kompilieren und flashen.
6. Morgens einmal kurz drücken: Der Arbeitszyklus startet.
7. Unterbrechungen wie gewohnt kurz drücken.
8. Zum Feierabend optional mindestens 2 Sekunden gedrückt halten. Wenn du es vergisst, übernimmt der Tageswechsel den letzten Druck automatisch als Ende.

## Technische Dokumentation

- [Sketch-Dokumentation](Unterbrechungszaehler/README.md)
- [Hardware-Wiring](Unterbrechungszaehler/HARDWARE_WIRING.md)
- [Architektur](Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Speicherformat](Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Zeitarchitektur](Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Testbericht](Unterbrechungszaehler/TEST_REPORT.md)
- [Release Notes](Unterbrechungszaehler/RELEASE_NOTES.md)
- [Changelog](CHANGELOG.md)

## Screenshots

Die vorhandenen Screenshots bleiben unter `docs/images/3.0.0/` erhalten. Sie zeigen die bestehende Weboberfläche; 3.6.0 verändert primär die physische Tastenlogik und die neue Feierabend-OLED-Anzeige.

## Warum Schwäbisch?

Weil technische Projekte nicht immer komplett ernst sein müssen. Software darf funktionieren **und** trotzdem ein bisschen Persönlichkeit haben.

Ob Schwäbisch die internationale Verbreitung des Projekts beschleunigt oder massiv behindert, wird die Zukunft zeigen.

## Lizenz

MIT. Benutzen, verändern, erweitern und daraus etwas Eigenes bauen ist ausdrücklich erlaubt. Wenn daraus irgendwann ein millionenschweres Produkt entsteht, freue ich mich weiterhin über eine Postkarte.

GitHub: [taloriko](https://github.com/taloriko)
