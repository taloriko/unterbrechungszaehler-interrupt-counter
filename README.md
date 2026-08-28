# Unterbrechungszähler / Interrupt Counter

> [!WARNING]
> **KI-Hinweis / AI notice:** Dieses Projekt wurde maßgeblich mit KI erstellt und anschließend praktisch getestet und weiterentwickelt. Wer grundsätzlich keinen KI-generierten Code verwenden möchte, kann hier aufhören. / This project was created to a significant extent with AI and then practically tested and iterated. If you do not want to use AI-generated code on principle, you can stop here.

## Deutsch

### Warum es dieses Projekt gibt

Ein Taster, weil **„ich werde ständig unterbrochen“ offenbar noch keine Kennzahl ist.**

Dieses Projekt ist aus dem Schmerz heraus entstanden, an manchen Arbeitstagen kaum noch einen Gedanken zu Ende bringen zu können. Das Problem dabei: Eine persönliche Aussage wie „Ich werde sehr häufig unterbrochen“ ist schwer greifbar. In größeren Organisationen haben Tabellen, Kurven und Heatmaps manchmal einfach bessere Überlebenschancen als Sätze von Mitarbeitenden.

Also: Knopf drücken, Unterbrechung zählen, später auswerten.

Der wichtigste Punkt war von Anfang an, dass die Bedienung **so einfach sein muss, dass sie selbst an einem turbulenten Tag noch benutzt wird**. Kein Formular, kein Grund auswählen, kein Handy entsperren. Ein physischer Tastendruck genügt.

Ab hier wird es wieder sachlich.


## Screenshots

### Heute

![Reiter Heute](docs/images/Reiter%20-%20Heute.png)

### Heatmap

![Reiter Heatmap](docs/images/Reiter%20-%20Heatmap.png)

### Details

![Reiter Details](docs/images/Reiter%20-%20Details.png)

### Export

![Reiter Export](docs/images/Reiter%20-%20Export.png)

### Gerät

![Reiter Gerät](docs/images/Reiter%20-%20Geraet.png)

### Autark – Beta

![Reiter Autark Beta](docs/images/Reiter%20-%20Autark%20-%20Beta.png)

### Funktionen

- physischer Taster an GPIO27: kurzer Druck speichert eine Unterbrechung
- langer Druck von ca. 3 Sekunden löscht den letzten Eintrag
- virtueller Taster in der Weboberfläche
- visueller Karten-Puls: Speichern blau, Löschen rot
- NTP-Zeitsynchronisierung mit automatischer CET/CEST-Umschaltung
- primärer NTP-Server über die Geräte-Seite frei konfigurierbar; vor dem Speichern wird eine echte NTP-Antwort geprüft
- lokale Weboberfläche für Firefox, Chrome und Chromium-Browser
- Auswertung nach Tag und Stunde
- Tagesverlauf
- Heatmap nach Wochentag und Uhrzeit
- Tagesdetails mit Zeitabständen zwischen Unterbrechungen
- CSV-Export mit Export-Datum und -Uhrzeit im Dateinamen
- Geräte-Seite mit ESP32-, WLAN-, RAM-, Flash- und LittleFS-Informationen
- Flash, LittleFS, Ringspeicher und freier RAM als Balken
- binärer Ringspeicher für 10.000 normale Ereignisse
- ältester Eintrag wird beim Erreichen der Kapazität automatisch überschrieben
- Autarker Modus **BETA** für Akkubetrieb
- separater Autark-Ringspeicher mit 10.000 Datensätzen
- Autark-Betrieb auch ohne gültige NTP-Zeit über relative Zeit
- WLAN aus, CPU auf 80 MHz und Light-Sleep im Autarken Modus
- Dark Mode nach Browser-/Systemeinstellung
- mDNS: `http://unterbrechungen.local`

### Hardware

Getestete Zielplattform:

- ESP32 Dev Module / ESP32-WROOM-32
- Eaton M22 oder anderer potentialfreier Taster
- optionaler potentialfreier Schiebeschalter für den Autarken Modus

Verdrahtung:

```text
ESP32

GPIO27  -------- Taster ---------------- GND
GPIO33  -------- Schiebeschalter ------- GND
```

Beide Eingänge verwenden `INPUT_PULLUP`.

**Keine externe Spannung an GPIO27 oder GPIO33 anlegen.**

- GPIO33 offen: normaler Netz-/WLAN-Betrieb
- GPIO33 gegen GND: Autarker Modus **BETA**

### Bedienung

- **kurz drücken:** Unterbrechung speichern
- **ca. 3 s halten:** letzten Eintrag löschen
- **LED 1x kurz:** Speichern erfolgreich
- **LED 3x schnell:** Löschen erfolgreich
- **LED 2x schnell + 2x langsam:** Warnung, z. B. fehlende Zeit, fehlende Verbindung oder Speicherproblem

### Weboberfläche

Nach erfolgreicher WLAN-Verbindung:

```text
http://unterbrechungen.local
```

Falls mDNS im Netzwerk nicht funktioniert, kann die im seriellen Monitor ausgegebene IP-Adresse direkt verwendet werden.

Die Oberfläche bietet die Reiter:

- Heute
- Verlauf
- Heatmap
- Details
- Export
- Gerät
- Autark **BETA**

### Datenspeicherung

Normale Ereignisse liegen in `/events.bin` als binärer Ringspeicher mit **10.000 Zeitstempeln**. Ist der Speicher voll, wird der jeweils älteste Datensatz überschrieben. Die Datei wächst daher nicht unbegrenzt.

Der Autarke Modus verwendet zusätzlich `/autark.bin` mit ebenfalls **10.000 Datensätzen**. Dabei werden Session-Start, Pulse und Session-Ende gespeichert.

Bestehende ältere Daten aus `/events.txt` werden beim ersten Start übernommen und anschließend als `/events.txt.migrated` erhalten.

### Autarker Modus BETA

Der Autarke Modus ist für Akkubetrieb gedacht. Wird GPIO33 gegen GND geschaltet, startet eine neue Autark-Session und ein Marker `Akku-Betrieb` wird gespeichert.

Danach werden Unterbrechungen als relative Laufzeit seit Session-Beginn abgelegt. Dadurch kann der Zähler auch dann weiterarbeiten, wenn beim Start keine gültige NTP-Zeit verfügbar ist.

Im Autarken Modus werden softwareseitig reduziert:

- WLAN aus
- mDNS aus
- Webserver aus
- CPU auf 80 MHz
- Light-Sleep zwischen Eingaben
- Wake-up über GPIO27 und GPIO33

Beim Rückschalten auf Netzbetrieb werden WLAN und NTP wieder gestartet. Eine Session ohne Zeitanker kann dadurch nachträglich zeitlich eingeordnet werden.

**Einschränkung:** Wird der ESP32 während einer Autark-Session komplett stromlos, kann die stromlose Zeit ohne zusätzliche RTC nicht rekonstruiert werden.

### Flashen

Eine ausführliche Anleitung liegt unter [`docs/FLASHEN.md`](docs/FLASHEN.md).

Kurzfassung:

1. Arduino IDE 2.x installieren.
2. ESP32-Boardpaket von Espressif installieren.
3. Board `ESP32 Dev Module` auswählen.
4. `Secrets.example.h` nach `Secrets.h` kopieren und WLAN-Daten eintragen.
5. COM-Port auswählen.
6. Upload Speed zunächst `115200`.
7. Sketch hochladen.
8. Seriellen Monitor auf `115200 Baud` öffnen.

Empfohlene Einstellungen:

```text
Board:            ESP32 Dev Module
Upload Speed:     115200
CPU Frequency:    240 MHz
Flash Frequency:  80 MHz
Flash Mode:       QIO
Flash Size:       4 MB
Partition Scheme: Default 4MB with spiffs
```

### NTP-Server konfigurieren

Im Reiter **Gerät** kann der primäre NTP-Server geändert werden. Beim Klick auf **Prüfen & speichern** führt der ESP32 zuerst eine DNS-Auflösung und anschließend eine echte NTP-Anfrage über UDP/123 aus. Nur bei erfolgreicher Antwort wird der Server dauerhaft im NVS gespeichert. `time.cloudflare.com` und `time.google.com` bleiben als Fallback-Server aktiv.

Beispiele:

```text
pool.ntp.org
ntp.meinefirma.local
192.168.1.10
```

### Projekt-Repository auf GitHub

Empfohlener Repository-Name:

```text
unterbrechungszaehler-interrupt-counter
```

GitHub-Konto des Autors: [taloriko](https://github.com/taloriko)

Projekt-Repository: [unterbrechungszaehler-interrupt-counter](https://github.com/taloriko/unterbrechungszaehler-interrupt-counter)



### GitHub-Beschreibung – Deutsch

> ESP32-Unterbrechungszähler für den Arbeitsplatz: ein Tastendruck pro Unterbrechung, lokale Webauswertung mit Tagesverlauf, Heatmap, CSV-Export und optionalem Autark-/Akkubetrieb. Entstanden aus dem Wunsch, Unterbrechungen sichtbar und messbar zu machen.

### GitHub description – English

> ESP32 workplace interrupt counter: one button press per interruption, local web dashboard with daily trends, heatmap, CSV export and an optional standalone battery mode. Built to turn constant interruptions into measurable data.


### Lizenz

MIT

---

## English

### Why this project exists

A physical button, because apparently **“I keep getting interrupted” is not a KPI yet.**

This project grew out of the frustration of reaching the end of a busy workday and realizing that finishing a single train of thought had become surprisingly difficult. The problem is that a statement like “I get interrupted all the time” is hard to quantify. In large organizations, tables, charts and heatmaps sometimes have a better chance of changing something than an employee simply saying it out loud.

So the idea became deliberately simple: press a button, record the interruption, analyze it later.

The most important design requirement was that it had to remain **simple enough to use on exactly the kind of chaotic day it is meant to measure**. No form, no category selection, no phone unlock. Just press the button.

## Screenshots

### Heute

![Reiter Heute](docs/images/Reiter%20-%20Heute.png)

### Heatmap

![Reiter Heatmap](docs/images/Reiter%20-%20Heatmap.png)

### Details

![Reiter Details](docs/images/Reiter%20-%20Details.png)

### Export

![Reiter Export](docs/images/Reiter%20-%20Export.png)

### Gerät

![Reiter Gerät](docs/images/Reiter%20-%20Geraet.png)

### Autark – Beta

![Reiter Autark Beta](docs/images/Reiter%20-%20Autark%20-%20Beta.png)

### Funktionen

- physical button on GPIO27: short press records an interruption
- long press of about 3 seconds deletes the latest entry
- virtual button in the web interface
- visual card feedback: blue for a new event, red for deletion
- NTP synchronization with automatic CET/CEST handling
- configurable primary NTP server on the device page; a real NTP response is checked before saving
- local browser UI for Firefox, Chrome and Chromium-based browsers
- daily and hourly analysis
- daily history
- weekday/hour heatmap
- detailed daily event list with time gaps
- CSV export with export date and time in the filename
- device page with ESP32, Wi-Fi, RAM, Flash and LittleFS information
- usage bars for Flash, LittleFS, ring buffers and free RAM
- binary ring buffer for 10,000 normal events
- oldest event is overwritten when capacity is reached
- standalone battery mode **BETA**
- separate standalone ring buffer with 10,000 records
- standalone operation without valid NTP time using relative timestamps
- Wi-Fi disabled, CPU reduced to 80 MHz and Light Sleep in standalone mode
- automatic dark mode
- mDNS at `http://unterbrechungen.local`

### Hardware

Tested target platform:

- ESP32 Dev Module / ESP32-WROOM-32
- Eaton M22 or another dry-contact push button
- optional dry-contact slide switch for standalone mode

Wiring:

```text
ESP32

GPIO27  -------- push button -------- GND
GPIO33  -------- slide switch ------- GND
```

Both inputs use `INPUT_PULLUP`.

**Do not apply external voltage to GPIO27 or GPIO33.**

- GPIO33 open: normal network/Wi-Fi mode
- GPIO33 connected to GND: standalone mode **BETA**

### Storage

Normal events are stored in `/events.bin` as a binary ring buffer containing up to **10,000 timestamps**. Once full, the oldest record is overwritten, so the file does not grow indefinitely.

Standalone sessions use a second `/autark.bin` ring buffer with **10,000 records** for session markers and interruption events.

### Standalone mode BETA

Standalone mode is intended for battery operation. Switching GPIO33 to GND starts a new session and stores an `Akku-Betrieb` start marker.

Events are then stored using elapsed time relative to the session start, allowing the counter to operate even when no valid NTP time is available.

To reduce power consumption, standalone mode disables Wi-Fi, mDNS and the web server, reduces CPU frequency to 80 MHz and uses Light Sleep between interactions.

When network mode is restored, Wi-Fi and NTP are started again and sessions without a start-time anchor can be positioned in time afterwards.

**Limitation:** If the ESP32 completely loses power during a standalone session, the powered-off duration cannot be reconstructed without an additional RTC.

### Flashing

See [`docs/FLASHEN.md`](docs/FLASHEN.md) for the full flashing guide.

### Configuring the NTP server

The primary NTP server can be changed on the **Device** tab. **Check & save** first resolves the hostname and then sends a real NTP request over UDP/123. The setting is only stored in NVS after a valid response. `time.cloudflare.com` and `time.google.com` remain configured as fallback servers.


### GitHub repository

Recommended repository name:

```text
unterbrechungszaehler-interrupt-counter
```

Author profile: [taloriko](https://github.com/taloriko)

Project repository: [unterbrechungszaehler-interrupt-counter](https://github.com/taloriko/unterbrechungszaehler-interrupt-counter)

Once the repository has been created, the project URL can also be added to the device web interface.

### License

MIT
