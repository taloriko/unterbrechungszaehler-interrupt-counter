# Changelog

## 3.6.1

- physischen Arbeitszyklus auf DI1/GPIO13 nach Praxistest neu integriert; der verzögerte Pending-Kandidaten-Ansatz des 3.6.0-Teststands wurde entfernt
- erster kurzer Druck startet den Zyklus weiterhin ohne Unterbrechung, erzeugt jetzt aber weder Anti-Spam-Ton noch eine 10-Sekunden-Sperre
- jeder gültige kurze Druck im aktiven Zyklus wird sofort über den bewährten Unterbrechungspfad gespeichert, gezählt, angezeigt und mit einem normalen Track ab 3 bestätigt
- Track 2 und OLED-TV-Störung werden ausschließlich bei tatsächlich zu schnellen weiteren Kurzdrucken innerhalb der bestehenden 10-Sekunden-Sperre ausgelöst
- langer Druck ab 2 Sekunden beendet den Zyklus unabhängig von der Kurzdruck-Sperre; START und ENDE bleiben außerhalb des Unterbrechungs-Rings
- vergessener Feierabend wird beim lokalen Tageswechsel durch Schließen des Zyklus abgefangen; bereits erfasste Unterbrechungen werden nicht rückwirkend zu einem Ende umklassifiziert
- manuelles Ende zeigt 10 Sekunden `FEIERABEND` plus Tageszahl; währenddessen werden physische Eingaben ignoriert und die normale OLED-Ansicht darf die Abschlussanzeige nicht überschreiben
- neuer kompakter CYC2-Zykluszustand verwirft den alten 3.6.0-Pending-Zustand bewusst, damit ein OTA mit sauberer Zyklussemantik startet
- RawEvent bleibt 9 Byte, Tagesaggregate bleiben 64 Byte; vorhandene 3.x-Unterbrechungsdaten benötigen keine Migration

## 3.5.0

- neue Home-Karte **Fokus & Ruhe** mit aktueller Ruhephase, längster Ruhephase heute, längster Ruhephase der laufenden Woche und erklärtem 120-Minuten-Trend
- Ruhephasen werden ausschließlich zwischen gültigen Unterbrechungen desselben lokalen Tages bzw. von der letzten heutigen Unterbrechung bis jetzt berechnet; keine künstlichen Nacht- oder Mitternachtsphasen
- Trend vergleicht die letzten 60 Minuten mit den 60 Minuten davor; Differenzen ab ±2 werden als steigend/fallend, kleinere Abweichungen als stabil dargestellt
- neue Auswertung **Arbeitsmuster**: ruhigstes vollständig beobachtetes 2-Stunden-Fenster und vollständig beobachtete Einzelstunde mit den meisten Unterbrechungen
- Arbeitsmuster verwenden maximal 30 abgeschlossene Tage, mindestens fünf abgedeckte Tage pro Ergebnis und ausschließlich Zeitfenster innerhalb der beobachteten Tagesaktivität
- zentrale RAM-Cache-Berechnung scannt den Raw-Ring nur bis zum benötigten Zeithorizont, wird nach neuen gültigen Ereignissen ungültig und spätestens nach 60 Sekunden aktualisiert
- neue OLED-Modi **Ruhephasen** und **Arbeitsmuster** mit großen, nicht blockierend wechselnden Seiten; bestehende Displaymodi bleiben unverändert
- bisherige Home-Projekteinstellungen unverändert nach **Einstellungen → Projekteinstellungen** verschoben; API, NVS-Keys, Wertebereiche und Persistenz bleiben gleich
- neue Focus-/Insight-Texte in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch
- RawEvent bleibt 9 Byte; kein neues persistentes Focus-/Insight-Format und keine zusätzlichen Flash-Schreibvorgänge

## 3.4.1

- Weboberfläche verfolgt einen ausdrücklich gestarteten DY-SV17F-Audiotest bis zum Firmware-Abschluss; temporär 500 ms, maximal 120 s, kein zusätzliches permanentes Polling
- OLED-TV-Störung startet nur beim ersten verworfenen Druck einer laufenden 10-s-Sperre und endet mit dieser Sperre; weitere verworfene Drücke starten die Animation nicht neu
- Track-2-Fast-Feedback, 10-s-Cooldown und Regel „verworfen verlängert nicht“ bleiben unverändert
- README auf den aktuellen technischen Funktionsstand und die aktuelle Trackbelegung synchronisiert; Versionshistorie zentral im Changelog geführt

## 3.4.0

- 10-Sekunden-Anti-Spam ausschließlich für den physischen DI1/GPIO-Knopf; verworfene Drücke erreichen weder Raw-Ring noch Tagesaggregate, CSV, Heatmaps oder Ø-Abstände
- verworfene Drücke verlängern die Sperrzeit nicht; Web-Ereignisse bleiben unabhängig
- Track 2 ist fest als Anti-Spam-Ton reserviert, normale Unterbrechungstöne und Rotation beginnen bei Track 3
- schneller Feedbackpfad: Anti-Spam-Tonkommando und erster OLED-Störframe laufen vor Logging/Persistenzarbeit
- nicht blockierendes, deterministisches OLED-Flimmern im Stil eines alten Fernsehers mit kurzem sprachabhängigem Hinweis
- GPIO-Diagnose zeigt 10-s-Sperre, seit Boot verworfene Drücke und den letzten verworfenen Druck

## 3.3.2

- DY-SV17F-Audiotest beendet sich jetzt zuverlässig über den aktiv abgefragten UART-Wiedergabestatus statt auf eine weitere BUSY-Flanke angewiesen zu sein
- Während eines ausdrücklich gestarteten Audiotests wird der Status mit sparsamen 500-ms-Abständen abgefragt; außerhalb des Tests gibt es weiterhin kein zyklisches UART- oder BUSY-Polling
- BUSY-Flanken beschleunigen eine Statusprüfung weiterhin, sind aber nur Zusatzdiagnose und keine Voraussetzung für einen erfolgreichen Test
- Wenn UART sowohl `Spielt` als auch anschließend `Gestoppt` bestätigt, endet der Audiotest mit OK; eine nicht bestätigte BUSY-Polarität wird separat angezeigt
- Der 120-s-Sicherheits-Timeout beendet den Test nun auch dann sicher, wenn gerade eine Statusabfrage geplant oder aktiv ist

## 3.3.1

- DY-SV17F-Diagnose trennt UART-Status und BUSY-Hardwarepegel klar voneinander und zeigt deren Messzeitpunkte
- BUSY wird nicht mehr permanent im Audio-Loop gepollt; ein Snapshot erfolgt nur bei Boot/Prüfung und Flanken werden ausschließlich während des manuellen Audiotests beobachtet
- **Prüfen** bleibt lautlos und fragt Wiedergabestatus, Datenträger und Dateianzahl gezielt per UART ab
- **Ton testen** wird zum nicht blockierenden End-to-End-Diagnosetest: UART PLAY bestätigen, BUSY-Flanken beobachten und ein vermutetes Ende einmal per UART gegenprüfen
- BUSY-Polarität startet bewusst unbestätigt und wird nur nach einem vollständigen Ruhe → Wiedergabe → Ruhe-Zyklus als HIGH/LOW=Wiedergabe übernommen
- Ein unplausibles BUSY-Signal beschädigt die funktionierende Wiedergabe nicht; fehlende UART-Antwort bleibt dagegen ein echter Diagnosefehler
- Datenträgerwert wird nicht geraten: Klartext `Antwort erhalten` plus technischer Rohwert (z. B. `0x04`)

## 3.3.0

- Datenbank kann nach Eingabe des exakten Projektnamens vollständig gelöscht werden; Rohdaten und Tagesaggregate werden entfernt und das Gerät startet sauber neu
- Heatmaps erhalten einen Herkunftsfilter: **Beides** (Standard), **Knopf / GPIO** oder **Web**; das API-Modell bleibt für weitere Quellen wie API/Software/Hardware erweiterbar
- Herkunft ist weiterhin Bestandteil jedes kompakten 9-Byte-Rohdatensatzes; gefilterte Heatmaps werden ehrlich aus dem retained Raw-Ring berechnet und melden unvollständige Abdeckung
- Ø-Abstand behält die 3.1-Regel bei: der Filter bezieht sich auf die Start-Unterbrechung, der letzte Druck des Tages bleibt ausgeschlossen und es wird nie über Mitternacht gerechnet
- Gerät → Speicher zeigt bei Fehlern die konkrete Rohdaten-/Statistik-Ursache statt nur einen allgemeinen Fehlerzustand
- Webbundle, API-Routen und Releasechecks auf 3.3.0 erweitert

## 3.2.0

- OLED-Inhalte folgen der persistent synchronisierten UI-Sprache; kompakte OLED-Transliteration für Umlaute und Akzente
- SH1106-Bootscreen auf mindestens 4 Sekunden verlängert, weiterhin nicht blockierend
- persistente 180°-Displaydrehung ergänzt
- frische Helligkeitsdefaults auf 65 % normal und 5 % gedimmt gesetzt
- Geräteeinstellungen nach Display, Display-Feedback und DY-SV17F-Sound gruppiert
- DY-SV17F-Lautstärke von 0–100 %, Standard 100 %, zentral auf den Modulbereich 0…30 abgebildet
- Wechsel-/Rotationsmodus ist bei einer frischen Konfiguration der neue Tonstandard; Track 1 bleibt Boot vorbehalten
- OLED-Modi **Tagesfortschritt** und **Fokus** ergänzt
- Arduino-IDE-Hinweis zum Erzeugen einer Sketch-BIN vollständig aus dem OTA-UI entfernt; Releases liefern die fertige OTA-BIN
- Webbundle, Sprachchecks und Releasechecks auf 3.2.0 erweitert

## 3.1.0

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

## 3.0.1

Patch-Release für die OTA-Oberfläche und den Fallback-Access-Point. Die 3.0.0-Baseline bleibt unverändert; 3.0.1 korrigiert und verbessert ausschließlich den aktuellen 3.x-Stand.

### Änderungen

- Fallback-AP ist mit dem festen Passwort `Unterbrechungszähler` geschützt
- irreführender Hinweis auf einen offenen Fallback-AP vollständig aus der OTA-Oberfläche und allen UI-Sprachpaketen entfernt
- statischer Hinweis auf eine fehlende zweite OTA-App-Partition aus der normalen OTA-Karte entfernt; echte OTA-Fehlerdiagnose bei einem tatsächlichen Partitionsfehler bleibt erhalten
- OTA-Speicherwerte bleiben als Byte-Angaben sichtbar und werden zusätzlich als segmentierter Auslastungsbalken dargestellt
- Geräte-API liefert dafür `ota.usedPercent`
- Releasechecks prüfen AP-Schutz, entfernte Alt-Hinweise und den OTA-Auslastungsbalken
- Dokumentation für Deutsch, Englisch und Schwäbisch auf den passwortgeschützten Fallback-AP und den aktuellen Stand 3.0.1 aktualisiert

## 3.0.0

Version 3.0.0 ist der neue stabile Ausgangspunkt des Unterbrechungszählers. Frühere 1.x/2.x-Releases waren Entwicklungs- und Teststände; für sie wird keine Migration oder Hardwarekompatibilität zugesichert.

### Highlights

- vollständig neue modulare Firmwarestruktur für den ESP32
- lokaler Unterbrechungszähler per GPIO13/DI1 und Webbutton
- binärer Ringspeicher für 100.000 Rohereignisse
- separater Tagesaggregatring für 2.300 Tage
- drei Heatmap-Auswertungen sowie gestreamter CSV-Export
- persistente Sound- und Displayeinstellungen
- lokale Weboberfläche ohne Cloud-Abhängigkeit

### Hardware

- Unterbrechungstaster: GPIO13 gegen GND, `INPUT_PULLUP`, active-low
- DS3231 und SH1106 gemeinsam auf I2C GPIO21/22
- DY-SV17F auf UART2: RX GPIO18, TX GPIO19
- DY-SV17F CON3/BUSY auf GPIO39/VN mit externem ca. 10-kΩ-Pull-up an V33
- CON1 und CON2 des DY-SV17F für UART-Modus auf GND
- eigene 4-MiB-Partitionstabelle mit zwei OTA-App-Slots und LittleFS
