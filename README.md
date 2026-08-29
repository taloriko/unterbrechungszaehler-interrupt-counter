# Unterbrechungszähler / Interrupt Counter

[🇬🇧 English documentation](docs/en/README.md)

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt und anschließend praktisch getestet und weiterentwickelt. Wem die Verwendung von KI-generiertem Code grundsätzlich nicht passt, kann hier aufhören.

## Aktueller Stand

**Firmware: `2026-08-29-17`**

Der aktuelle Stand enthält unter anderem:

- normalen Ringspeicher für aktuelle Ereignisse
- zusätzlichen Langzeit-Ringspeicher mit **100.000 Einträgen**
- schnelle, serverseitig aggregierte Heatmaps
- Heatmap nach Wochentag / Uhrzeit
- Heatmap nach Monat / Kalenderwoche mit Jahresauswahl
- Heatmap nach Jahr / Monat für die letzten fünf Kalenderjahre
- eigenen Reiter **Einstellungen**
- konfigurierbaren Heatmap-Zeitbereich
- Sprachwahl und Hell-/Dunkelmodus
- Fallback-WLAN bei nicht erreichbarem normalen WLAN
- serielle Diagnoseausgaben
- Browser-Favicon zur leichteren Wiedererkennung des Tabs

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

## Ereignisse erfassen

- physischer Taster bzw. potentialfreier Kontakt
- virtueller Taster in der Weboberfläche
- kurzer Tastendruck speichert ein Ereignis
- langer Tastendruck löscht den letzten Eintrag
- LED-Rückmeldung für Speichern, Löschen und Fehlerzustände

## Auswertung

- Ereignisse des aktuellen Tages
- durchschnittlicher und längster Abstand zwischen Ereignissen
- letzte Ereignisse des Tages
- Tagesverlauf und Übersicht mehrerer Tage
- detaillierte Ereignisliste pro Tag
- **Heatmap Wochentag / Uhrzeit**
  - frei einstellbarer Stundenbereich
  - Standard `05:00` bis `18:00`
  - Endstunde inklusive
- **Heatmap Monat / Kalenderwoche**
  - 12 Monate vertikal
  - Kalenderwochen 1 bis 52 horizontal
  - horizontales Scrollen
  - aktuelles Jahr als Vorauswahl
  - vorhandene Jahre per Dropdown auswählbar
- **Heatmap Jahr / Monat**
  - immer aktuelles Jahr plus vier Vorjahre
  - schnelle Monatsübersicht über fünf Kalenderjahre

## Schnelle Langzeitauswertung

Für die Heatmaps werden nicht mehr alle gespeicherten Zeitstempel an den Browser übertragen.

Der ESP32 berechnet die benötigten Werte über eine eigene Aggregat-API bereits vor:

`/api/aggregate`

Dadurch bleiben die Datenmengen klein und die Heatmaps auch bei vielen zehntausend gespeicherten Ereignissen schnell.

Die aggregierten Daten werden im RAM zwischengespeichert und nur neu aufgebaut, wenn sich das Langzeitarchiv geändert hat.

## Weboberfläche

Die Weboberfläche läuft vollständig lokal auf dem ESP32.

Enthalten sind aktuell die Reiter:

- Heute
- Verlauf
- Heatmap
- Details
- Export
- Gerät
- Einstellungen
- Autark **BETA**

Weitere Funktionen:

- responsive Darstellung
- Browser-Favicon
- automatische bzw. manuelle Hell-/Dunkeldarstellung
- Sprache lokal im Browser speicherbar
- Sprachen:
  - Deutsch
  - Schwäbisch
  - Englisch
  - Italienisch
  - Französisch
- Geräteinformationen zu ESP32, RAM, Flash, LittleFS und Ringspeichern
- grafische Speicherbelegung
- Anzeige der Firmware-Version

## Netzwerk und Zugriff

Im normalen Betrieb ist die Weboberfläche über mDNS erreichbar:

`http://unterbrechungen.local`

Falls mDNS auf dem verwendeten Gerät nicht funktioniert, kann stattdessen direkt die IP-Adresse des ESP32 verwendet werden, zum Beispiel:

`http://192.168.178.50`

### Fallback-WLAN

Wenn das konfigurierte WLAN nicht erreichbar ist, startet der ESP32 zusätzlich ein offenes lokales WLAN:

- SSID: `Unterbrechungszaehler`
- IP: `192.168.4.1`
- Adresse: `http://192.168.4.1`

Sobald die normale WLAN-Verbindung hergestellt ist, wird der Fallback-Hotspot wieder abgeschaltet.

## Zeit

- automatische NTP-Zeitsynchronisation
- automatische Sommer-/Winterzeit für Deutschland
- eigener primärer NTP-Server einstellbar
- Prüfung des NTP-Servers vor dem Speichern
- zusätzliche Fallback-NTP-Server:
  - Cloudflare
  - Google
- falls noch keine gültige Zeit vorhanden ist, kann die Browserzeit einmalig übernommen werden
- eine bereits gültige ESP32-Zeit wird nicht durch die Browserzeit überschrieben
- im Normalbetrieb werden Ereignisse nur mit gültiger absoluter Zeit gespeichert

## Speicherung

### Normaler Ringspeicher

Der normale Ringspeicher dient den schnellen aktuellen Ansichten:

- Datei: `/events.bin`
- Kapazität: **10.000 Ereignisse**
- binäre Speicherung
- ältester Eintrag wird bei vollem Ring automatisch überschrieben
- bestehende ältere Datensätze können übernommen werden

### Langzeit-Ringspeicher

Für langfristige Statistiken und Heatmaps gibt es zusätzlich ein eigenes Archiv:

- Datei: `/events_10y.bin`
- Kapazität: **100.000 Ereignisse**
- ungefähr 400 kB Rohdaten
- automatischer Ringbetrieb
- vorhandene Daten des normalen Ringspeichers werden beim ersten Anlegen übernommen
- neue normale Ereignisse werden anschließend parallel gespeichert
- Löschen des letzten normalen Ereignisses wird ebenfalls nachgeführt

Bei einem Beispielwert von durchschnittlich **30 Ereignissen pro Werktag** reicht der Langzeitspeicher rechnerisch für ungefähr **12,8 Jahre** und bietet damit Reserve für eine geplante Auswertung über zehn Jahre.

## Export

- CSV-Export der normalen gespeicherten Ereignisse
- Datum und Uhrzeit im Export-Dateinamen
- Export enthält:
  - Datum
  - Uhrzeit
  - Unix-Zeitstempel
- separater CSV-Export für Autark-Daten

## Gerät und Diagnose

Im Reiter **Gerät** werden unter anderem angezeigt:

- Datum und Uhrzeit
- Zeitquelle
- Uptime
- Firmware-Version
- Hostname
- ESP32-Modell
- Revision
- CPU-Kerne
- CPU-Takt
- RAM gesamt / frei / belegt
- WLAN-Status
- IP-Adresse
- Signalstärke
- Fallback-WLAN
- primärer NTP-Server
- LittleFS-Belegung
- normaler Ringspeicher `10.000`
- Langzeit-Ringspeicher `100.000`
- Flash-/Sketch-Belegung

### Serieller Monitor

Die serielle Diagnose läuft mit **115200 Baud** und zeigt unter anderem:

- Firmware-Version
- Reset-Grund
- ESP32-/CPU-Informationen
- freien Heap
- Flash-/Sketch-Informationen
- LittleFS-Status
- Ringspeicherstände
- normalen WLAN-Status
- Fallback-AP mit SSID, IP und URL
- Zeitstatus und Zeitquelle
- Autark-Modus
- neue Ereignisse
- Löschvorgänge
- physische Tasterereignisse
- UI-Aktionen
- regelmäßige kompakte Statuszeile

## Einstellungen

Der eigene Reiter **Einstellungen** enthält aktuell:

- Sprache
- Darstellung:
  - System
  - Hell
  - Dunkel
- Startstunde der Wochentag/Uhrzeit-Heatmap
- Endstunde der Wochentag/Uhrzeit-Heatmap

Sprache, Darstellung und Heatmap-Zeitbereich werden lokal im jeweiligen Browser gespeichert.

## Autark-Modus – BETA

- Betrieb mit Akku oder Powerbank
- Aktivierung über Schiebeschalter an GPIO33
- eigener Ringspeicher für bis zu 10.000 Autark-Datensätze
- Ereigniserfassung auch ohne verfügbare NTP-Zeit
- relative Zeitmessung innerhalb einer Session
- spätere Rekonstruktion absoluter Zeit über Start-/Endanker, soweit möglich
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

> Die Screenshots können vom aktuellen Entwicklungsstand leicht abweichen.

---

# Lizenz

MIT

---

GitHub: [taloriko](https://github.com/taloriko)
