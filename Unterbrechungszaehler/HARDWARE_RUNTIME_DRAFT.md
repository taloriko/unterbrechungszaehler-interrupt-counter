# Hardware-Laufzeitarchitektur – Draft 3.3.0-dev433

Dieser Draft überarbeitet **nicht** die Fachlogik des Unterbrechungszählers. Er zieht nur die optionale Hardware wieder auf die ursprünglichen Basisregeln zurück: kleine unabhängige Treiber, zentrale Lifecycle-Orchestrierung, keine Projektbedeutung im Hardwarecode, keine unnötigen Interruptlasten und keine erfundenen Bestätigungen.

## Verbindliche Verantwortungsgrenzen

```text
HardwareRegistry
  ├─ GpioModule          GPIO / lokaler Zustand
  ├─ RtcDs3231           I2C / RTC
  ├─ DisplaySh1106       I2C / OLED
  ├─ AudioDySv17f        UART2 + BUSY
  └─ Rf433Cc1101         SPI + RMT RX + Funkdecoder

Rf433Service              SourceRegistry + Projektzuordnung
InterruptionService       Unterbrechungsereignis / Feedback / Speicherung
```

`HardwareRegistry` ist die **einzige** Stelle, die konkrete Hardwaretreiber startet und zyklisch bedient. `Rf433Service` initialisiert oder tickt den CC1101-Treiber nicht mehr; es konsumiert nur fertige Frames und ordnet sie stabilen Source-IDs zu.

## GPIO

DI1 bleibt ein Human-Button mit minimalem Edge-Latch. Die ISR setzt nur ein Flag. Entprellung, Callback und Projektarbeit laufen im normalen Loop. Deaktivierte generische Pins werden nicht konfiguriert und stehen optionalen Modulen zur Verfügung.

## DS3231

Der RTC-Treiber bleibt bewusst unverändert: gemeinsamer 400-kHz-I2C-Bus, BCD-/Kalenderprüfung, OSF-Auswertung, Temperatur und Schreibverifikation. Teilreads werden nicht als gültige Zeit ausgegeben.

## SH1106

Der Displaytreiber bleibt bewusst unverändert: fester 1024-Byte-Framebuffer, kleine I2C-Chunks, Controllerkommandos für Ein/Aus, Kontrast und 180°-Rotation. Ein Displayfehler beeinflusst keine Ereigniserfassung.

## DY-SV17F

UART bleibt 9600 8N1 auf GPIO18/19. CON3/BUSY auf GPIO39 ist nach der Mode-Select-Phase das unabhängige physische Playback-Feedback.

Die Laufzeit wurde vereinfacht:

- `Play specified music (0x07)` erwartet laut Protokoll **keine** UART-Antwort.
- Ein normaler Ton wird deshalb nicht mehr mit einer zusätzlichen Statusabfrage gekoppelt.
- Ein Wiedergabestart gilt nur dann als BUSY-bestätigt, wenn nach dem Play-Kommando eine **frische** BUSY-Sequenz beobachtet wird. Ein bereits vorher LOW stehendes BUSY bestätigt kein neues Kommando.
- Ein nicht bestätigter Play-Befehl wird höchstens einmal wiederholt.
- Lautstärke `0x13` ist command-only. 0–100 % wird zentral auf die dokumentierten 31 Stufen 0–30 abgebildet. Weil das Modul darauf keine Antwort sendet, wird dieselbe Einstellung zweimal mit Abstand gesendet; die UI nennt sie ausdrücklich Soll-/Sendewert, nicht bestätigten Istwert.
- Diagnoseabfragen `0x01`, `0x09`, `0x0A`, `0x0C`, `0x0D` laufen sequenziell und niedrig priorisiert. Echte Wiedergabe darf eine Diagnose abbrechen.
- Fehlgeschlagene Diagnoseabfragen löschen keine zuvor gültig gelesenen Werte.
- Nach dem Boot läuft einmal eine niedrige Prioritätsdiagnose, damit Trackanzahl/Datenträger für Rotation und Hardwarekarte verfügbar werden.

Die Hardwarekarte zeigt BUSY, Wiedergabestatus, Online-/aktiven Datenträger, Dateizahl, aktuellen Track, Soll-Lautstärke, gesendete Modulstufe sowie UART-/Playback-Zähler.

## CC1101 / RF1100SE

Der CC1101 bleibt im asynchronen OOK-Modus, weil Universal-Festcodes und Somfy RTS unterschiedliche proprietäre Rohpulse benötigen. **Die Rohflanken werden aber nicht mehr per GPIO-CHANGE-ISR in Software vermessen.**

Stattdessen besitzt der ESP32 einen RMT-RX-Kanal auf GDO0:

- 1 MHz Auflösung = 1 µs
- statischer 160-Symbol-Puffer
- 60-µs-Hardware-Glitchfilter
- 8-ms-Idlegrenze beendet eine Aufnahme
- Callback veröffentlicht nur `frame ready + symbol count`
- Universal-/Somfy-Decoding läuft vollständig im normalen Loop
- nach Kopie in einen kleinen lokalen Arbeitsbuffer wird RMT sofort wieder scharfgeschaltet

Damit sinkt die Interruptlast von „eine ISR pro RF-Flanke“ auf ungefähr „ein RX-done-Callback pro Aufnahme“. Das schützt insbesondere UART2, WLAN und den restlichen kooperativen Loop vor zufälliger 433-MHz-Rauschlast.

Die Hardwarekarte zeigt zusätzlich RMT-Bereitschaft, Carrier Sense, Anzahl Aufnahmen, letzte Symbolzahl, RMT-Fehler, Decodererfolge/-verwerfungen und Überläufe. Der bestehende 10-s-Empfangstest bleibt erhalten.

## Bus-/Prioritätsmodell

```text
Loop-Priorität:
1. HardwareRegistry: DI
2. HardwareRegistry: Audio UART/BUSY
3. HardwareRegistry: RF RMT-Auswertung
4. Rf433Service: fertige Frames -> Source-ID
5. InterruptionService: Feedback + Persistenzpipeline
6. WLAN/Web/Zeit/OTA
```

I2C, SPI, UART2 und RMT besitzen getrennte Verantwortungsbereiche. Kein Modul darf ein anderes Modul direkt initialisieren oder dessen Fehlerstatus überschreiben.

## Diagnosesemantik

Ein Diagnosewert ist nur dann „bestätigt“, wenn seine reale Rückmeldung dazu passt:

- I2C/SPI: Bus-/Registerantwort
- Audio-Wiedergabe: externe BUSY-Flanke
- Audio-Kommandos ohne Response (z. B. Volume): nur „gesendet“, nie „bestätigt“
- RF-Konfiguration: SPI-Readback
- RF-Empfang: RMT-Aufnahme + gültiger Protokolldecoder

Ein optionales Modul darf ausfallen, ohne Raw-Event, PendingQueue oder andere Hardwaremodule zu beschädigen.
