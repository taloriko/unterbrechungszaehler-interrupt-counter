# Unterbrechungszähler / Interrupt Counter

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt, anschließend aber praktisch getestet, überarbeitet und weiterentwickelt. Wer KI-generierten Code grundsätzlich nicht mag, darf natürlich trotzdem den Taster drücken. ;-)

> **Aktueller Stand:** `3.0.0`

[Deutsch](docs/de/README.md) · [English](docs/en/README.md)

---

## Für was brauche ich das Ding?

Du sitzt konzentriert an einer Aufgabe. Dann kommt ein Kollege. Dann klingelt das Telefon. Dann braucht jemand „nur ganz kurz“ etwas. Irgendwann fragst du dich, warum die eigentliche Arbeit schon wieder nicht fertig geworden ist.

Genau dafür gibt es den **Unterbrechungszähler**:

**Taster drücken → Zeitpunkt speichern → später im Browser auswerten.**

Keine App öffnen. Kein Formular. Keine Excel-Liste. Ein Knopfdruck, fertig.

Technisch ist das Projekt nicht auf einen Bürotaster beschränkt. Der Eingang kann auch von einem geeigneten potentialfreien Kontakt kommen – zum Beispiel von einer Maschine, Lichtschranke oder einem Türkontakt. Wenn damit irgendwann Kühlschranköffnungen gezählt werden, möchte ich allerdings die Statistik sehen. ;-)

## Was kann Version 3.0.0?

- Ereigniserfassung per physischem Taster an **GPIO13 / DI1** oder Webbutton
- lokale Weboberfläche ohne Cloud-Abhängigkeit
- Live-Tageszähler und letzte Unterbrechung
- drei Heatmap-Auswertungen und CSV-Export
- binärer Ringspeicher für **100.000 Rohereignisse**
- separater Tagesaggregatring für **2.300 Tage**
- DS3231-RTC und SH1106-OLED
- DY-SV17F-Soundmodul mit Boot- und Unterbrechungstönen
- feste oder wechselnde Unterbrechungstracks; Track 1 bleibt Boot-Ton
- persistente Display-/Soundeinstellungen in NVS
- WLAN mit lokalem Fallback-AP
- OTA-Update mit eigener 4-MiB-Partitionstabelle
- lokale Zeitlogik für `Europe/Berlin`
- Benutzeroberfläche in **Deutsch, Englisch und Schwäbisch**

## 3.0.0 ist ein harter Schnitt

Die bisherigen 1.x/2.x-Stände waren Entwicklungs- und Teststände. **3.0.0 ist der neue Ausgangspunkt.** Es gibt deshalb keine zugesicherte Hardware-, Daten- oder OTA-Migration von 2.x. Wer von einem alten Testaufbau kommt, baut die Verdrahtung nach der aktuellen 3.0.0-Dokumentation neu auf.

## Aktuelle Pinbelegung

| Funktion | ESP32 |
|---|---:|
| Unterbrechungstaster / DI1 | GPIO13 gegen GND |
| I2C SDA – RTC + OLED | GPIO21 |
| I2C SCL – RTC + OLED | GPIO22 |
| DY-SV17F TX → ESP32 RX | GPIO18 |
| ESP32 TX → DY-SV17F RX | GPIO19 |
| DY-SV17F CON3/BUSY | GPIO39 / VN |

Für CON3/BUSY ist ein externer ca. **10-kΩ-Pull-up an V33 des DY-SV17F** erforderlich. CON1 und CON2 liegen für den UART-Modus auf GND. Details: [Hardware / Wiring](docs/de/HARDWARE.md).

## Schnellstart

1. [Hardware und Verdrahtung](docs/de/HARDWARE.md)
2. [Software, Build und Flashen](docs/de/SOFTWARE.md)
3. WLAN-Platzhalter in `Unterbrechungszaehler/config.h` lokal anpassen.
4. `Unterbrechungszaehler/Unterbrechungszaehler.ino` in der Arduino IDE öffnen.
5. **ESP32 Dev Module** auswählen, kompilieren und flashen.
6. Taster drücken. Falls dich niemand unterbricht, war der Aufbau vermutlich zu erfolgreich.

## Technische Dokumentation

- [Sketch-Dokumentation](Unterbrechungszaehler/README.md)
- [Hardware-Wiring](Unterbrechungszaehler/HARDWARE_WIRING.md)
- [Architektur](Unterbrechungszaehler/PROJECT_ARCHITECTURE.md)
- [Speicherformat](Unterbrechungszaehler/STORAGE_FORMAT.md)
- [Zeitarchitektur](Unterbrechungszaehler/TIME_ARCHITECTURE.md)
- [Testbericht](Unterbrechungszaehler/TEST_REPORT.md)
- [Changelog](CHANGELOG.md)

## Screenshots

Die finalen Screenshots werden separat ergänzt. Vorgesehene Dateien unter `docs/images/`:

- `de-home.png` – Home mit Tageszähler und Feedback/Display
- `de-auswertung.png` – Auswertung mit Heatmap
- `de-geraet.png` – Geräte-/Hardwarestatus
- `de-einstellungen.png` – Sprache und Darstellung
- `en-home.png` – English Home
- `en-analytics.png` – English Analytics

Bis die Bilder hochgeladen sind, enthält die README bewusst keine kaputten Bildlinks.

## Lizenz

MIT. Benutzen, verändern, erweitern und daraus etwas Eigenes bauen ist ausdrücklich erlaubt. Wenn daraus irgendwann ein millionenschweres Produkt entsteht, freue ich mich weiterhin über eine Postkarte.

GitHub: [taloriko](https://github.com/taloriko)
