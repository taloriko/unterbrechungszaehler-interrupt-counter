# Unterbrechungszähler / Interrupt Counter

[🇬🇧 English documentation](docs/en/README.md)

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt und anschließend praktisch getestet und weiterentwickelt. Wem die Verwendung von KI-generiertem Code grundsätzlich nicht passt, kann hier aufhören.

## Was ist das?

Der Unterbrechungszähler ist ein ESP32-basiertes Modul zum **einfachen Erfassen und Auswerten von Ereignissen**.

Ursprünglich wurde das Projekt entwickelt, um Unterbrechungen im Arbeitsalltag sichtbar zu machen:

**Taster drücken → Ereignis wird gespeichert → später auswerten.**

Datum und Uhrzeit werden automatisch erfasst und über eine lokale Weboberfläche ausgewertet.

## Warum habe ich das gebaut?

Ein Taster, weil **„ich werde ständig unterbrochen“ offenbar noch keine Kennzahl ist.**

Dieses Projekt ist aus dem Problem entstanden, an manchen Arbeitstagen kaum noch einen Gedanken zu Ende bringen zu können.

Eine Aussage wie „Ich werde sehr häufig unterbrochen“ ist jedoch schwer greifbar. Zahlen, Tagesverläufe und Heatmaps machen solche Unterbrechungen deutlich sichtbarer.

Der wichtigste Punkt war deshalb:

**Die Erfassung muss so einfach sein, dass sie selbst an einem turbulenten Arbeitstag noch benutzt wird.**

Kein Formular. Kein Smartphone. Keine Auswahl eines Grundes. Ein Tastendruck genügt.

## Nicht nur ein Unterbrechungszähler

Der Taster ist nur eine mögliche Anwendung.

Der Eingang des ESP32 reagiert auf einen potentialfreien Kontakt. Damit können auch andere Ereignisse automatisch erfasst werden.

Zum Beispiel:

- Taster
- Schalter
- Relaiskontakt
- Türkontakt
- Störmeldekontakt
- Betriebsrückmeldung
- Maschinenkontakt

Damit kann das Projekt grundsätzlich als **Ereignis- oder Kontaktzähler** für viele andere Anwendungen verwendet werden.

---

# Nachbauen

Der Weg zum fertigen Gerät ist in einzelne Schritte aufgeteilt. Jeder Schritt hat eine eigene Seite.

## 1. Hardware beschaffen

Welche Bauteile werden benötigt?

→ [Hardware beschaffen](docs/de/HARDWARE-BESCHAFFEN.md)

## 2. Hardware zusammenbauen

ESP32, Taster, Schalter und Stromversorgung anschließen.

→ [Hardware zusammenbauen](docs/de/HARDWARE-ZUSAMMENBAU.md)

## 3. Software

Software herunterladen und konfigurieren.

→ [Software](docs/de/SOFTWARE.md)

## 4. Flashen

Firmware auf den ESP32 übertragen.

→ [Flashen](docs/de/FLASHEN.md)

## 5. Normale Nutzung

Betrieb mit Stromversorgung, WLAN und Weboberfläche.

→ [Normale Nutzung](docs/de/NUTZUNG-NORMAL.md)

## 6. Autarke Nutzung

Mobiler Betrieb ohne feste Stromversorgung, zum Beispiel mit Akku oder USB-Powerbank.

→ [Autarke Nutzung](docs/de/NUTZUNG-AUTARK.md)

---

# Funktionen

- **Ereignisse erfassen**
  - physischer Taster bzw. potentialfreier Kontakt
  - virtueller Taster in der Weboberfläche
  - kurzer Tastendruck speichert ein Ereignis
  - langer Tastendruck löscht den letzten Eintrag
  - LED-Rückmeldung

- **Auswertung**
  - Ereignisse des aktuellen Tages
  - Tagesverlauf und Verlauf mehrerer Tage
  - Heatmap nach Wochentag und Uhrzeit
  - detaillierte Ereignisliste
  - Zeitabstände zwischen Ereignissen

- **Weboberfläche**
  - vollständig lokal auf dem ESP32
  - responsive Darstellung
  - automatische helle/dunkle Darstellung
  - erreichbar über `unterbrechungen.local`

- **Export**
  - CSV-Export der gespeicherten Ereignisse
  - Datum und Uhrzeit im Export-Dateinamen

- **Zeit**
  - automatische NTP-Zeitsynchronisation
  - automatische Sommer-/Winterzeit für Deutschland
  - eigener primärer NTP-Server einstellbar
  - zusätzliche Fallback-NTP-Server

- **Speicherung**
  - binärer Ringspeicher
  - bis zu 10.000 normale Ereignisse
  - automatische Überschreibung des ältesten Eintrags
  - Übernahme vorhandener älterer Datensätze

- **Geräteinformationen**
  - ESP32-, WLAN-, RAM-, Flash- und LittleFS-Informationen
  - grafische Speicherbelegung
  - Softwareversion

- **Autark-Modus – BETA**
  - Betrieb mit Akku oder Powerbank
  - eigener Speicher für bis zu 10.000 Datensätze
  - Ereigniserfassung auch ohne verfügbare NTP-Zeit
  - WLAN, mDNS und Webserver deaktiviert
  - CPU-Takt auf 80 MHz reduziert
  - Light-Sleep zwischen Eingaben
  - Rückkehr in den normalen WLAN-Betrieb über Schalter

---

# Screenshots

## Heute

![Reiter Heute](docs/images/Reiter%20-%20Heute.png)

## Heatmap

![Reiter Heatmap](docs/images/Reiter%20-%20Heatmap.png)

## Details

![Reiter Details](docs/images/Reiter%20-%20Details.png)

## Export

![Reiter Export](docs/images/Reiter%20-%20Export.png)

## Gerät

![Reiter Gerät](docs/images/Reiter%20-%20Geraet.png)

## Autark – Beta

![Reiter Autark Beta](docs/images/Reiter%20-%20Autark%20-%20Beta.png)

---

# Lizenz

MIT

---

GitHub: [taloriko](https://github.com/taloriko)
