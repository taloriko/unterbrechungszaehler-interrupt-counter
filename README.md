# Unterbrechungszähler / Interrupt Counter

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt, anschließend aber praktisch getestet, überarbeitet und weiterentwickelt. Wer KI-generierten Code grundsätzlich nicht mag, darf natürlich trotzdem den Taster drücken. ;-)

> **Aktueller Stand:** `3.6.1`

[Deutsch](docs/de/README.md) · [English](docs/en/README.md) · [Schwäbisch](docs/swg/README.md)

---

## Für was brauche ich das Ding?

Du sitzt konzentriert an einer Aufgabe. Dann kommt ein Kollege. Dann klingelt das Telefon. Dann braucht jemand „nur ganz kurz“ etwas. Und irgendwann fragst du dich, was du eigentlich vor zwei Stunden machen wolltest.

Genau dafür gibt es den **Unterbrechungszähler**.

Die Grundidee bleibt simpel:

**Taster drücken → Unterbrechung speichern → später im Browser auswerten.**

Seit 3.6.x kommt eine wichtige Konkretisierung dazu: **Arbeitsbeginn und Arbeitsende sind keine Unterbrechungen.** Das Gerät erkennt deshalb einen lokalen Arbeitszyklus, ohne zweiten Taster und ohne zusätzlichen GPIO.

> [!WARNING]
> Ob die Daten deinen Chef anschließend interessieren, ist natürlich weiterhin eine völlig andere wissenschaftliche Fragestellung. ;-)

## Neu in 3.6.1: Arbeitszyklus neu integriert

Der vorhandene Taster auf **DI1/GPIO13** funktioniert jetzt bewusst direkt und ohne rückwirkende Klassifikation:

1. **Erster kurzer Druck:** START des Arbeitszyklus – wird nicht als Unterbrechung gezählt.
2. **Weitere kurze Drücke:** werden bei gültigem Abstand sofort als Unterbrechung gespeichert und erhalten sofort das normale Feedback.
3. **Langer Druck ab 2 Sekunden:** beendet den Arbeitszyklus ausdrücklich und zählt nicht als Unterbrechung.
4. **Kein langer Druck:** beim lokalen Tageswechsel wird der Zyklus automatisch geschlossen.
5. Ein bereits erfasster kurzer Druck wird später **nicht mehr** zum Arbeitsende umgedeutet.

Der in 3.6.0 ausprobierte Pending-/„letzter Druck wird später entschieden“-Ansatz wurde damit verworfen. Genau diese Verzögerung machte Zähler und Sound im Alltag unnötig schwer nachvollziehbar.

### Feierabend

Ein langer Druck von mindestens **2 Sekunden** beendet den Arbeitszyklus sofort. Danach zeigt das SH1106 für **10 Sekunden**:

```text
FEIERABEND

<Anzahl heute>
HEUTE UNTERBR.
```

Während dieser Anzeige werden weitere physische Eingaben ignoriert und die normale Sekunden-/Home-Anzeige darf nicht dazwischen zeichnen. Beim automatischen Tagesabschluss bleibt das Display unverändert.

## Anti-Spam bleibt erhalten

Kurze physische Unterbrechungsdrücke haben weiterhin die feste **10-Sekunden-Sperre** aus 3.4.x:

- der **START-Druck eröffnet die Sperre nicht**,
- erst eine echte angenommene Unterbrechung startet die 10 Sekunden,
- weitere kurze Drücke innerhalb der Zeit werden nicht gespeichert oder gezählt,
- ein verworfener Druck verlängert die Sperre nicht,
- Track 2 und die OLED-TV-Störung bleiben ausschließlich das Anti-Spam-Feedback,
- normale Unterbrechungen verwenden weiterhin Track 3 und höher,
- Web-Ereignisse bleiben unabhängig.

Der lange Feierabend-Druck ist von der Sperre ausgenommen.

## Datenkompatibilität

Die bestehende Unterbrechungsdatenbank bleibt in 3.6.1 bewusst unverändert:

- **100.000 Raw-Records × 9 Byte**
- **2.300 Tagesaggregate × 64 Byte**
- bestehende EventSource-Bits und CRC bleiben erhalten
- bestehende 3.x-Rohdaten benötigen keine Migration

START und ENDE werden **nicht** in den Unterbrechungs-Ring geschrieben. Der laufende Zyklus wird kompakt in NVS gehalten; zusätzlich gibt es das kleine separate `/cycles.log` für START-/END-Marken.

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
6. Morgens einmal kurz drücken: Der Arbeitszyklus startet – ohne Unterbrechungston und ohne Anti-Spam-Sperre.
7. Unterbrechungen wie gewohnt kurz drücken; sie werden sofort erfasst.
8. Zum Feierabend mindestens 2 Sekunden gedrückt halten. Falls das vergessen wird, schließt der Zyklus beim Tageswechsel automatisch.

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

Die vorhandenen Screenshots bleiben unter `docs/images/3.0.0/` erhalten. Sie zeigen die bestehende Weboberfläche; 3.6.1 verändert primär die physische Tastenlogik und die Feierabend-OLED-Anzeige.

## Warum Schwäbisch?

Weil technische Projekte nicht immer komplett ernst sein müssen. Software darf funktionieren **und** trotzdem ein bisschen Persönlichkeit haben.

Ob Schwäbisch die internationale Verbreitung des Projekts beschleunigt oder massiv behindert, wird die Zukunft zeigen.

## Lizenz

MIT. Benutzen, verändern, erweitern und daraus etwas Eigenes bauen ist ausdrücklich erlaubt. Wenn daraus irgendwann ein millionenschweres Produkt entsteht, freue ich mich weiterhin über eine Postkarte.

GitHub: [taloriko](https://github.com/taloriko)
