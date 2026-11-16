# Release 3.6.8

- Wechselton bleibt bei beschaeftigtem DY-SV17F vorgemerkt und wird erst nach erfolgreicher Abspielbefehlannahme weitergeschaltet.
- Feierabend-Fortschrittsbalken nur bei gueltiger Uhrzeit zu Beginn des Haltens. Kurze Druecke unter 2 Sekunden zaehlen weiterhin.
- Einstellungsfehler erscheinen nur in der jeweiligen eigenen Karte.
- Web-Bundle auf den aktuellen Stand gebracht fuer Arduino-IDE und OTA.
- Bewusst akzeptiert: seltene Gleichzeitigkeit von Tastendruck und automatischem Ende; Stromausfall ohne Zeitpuffer, Wiederanlaufabschluss zur aktuellen Zeit; fehlgeschlagene NVS-Schreibvorgaenge werden nicht weiter behandelt.
- Vor Release: praktischer Test von DI1 kurz/lang, OLED, OTA, Audio und Einstellungen auf dem echten ESP32.

# Release 3.6.7

- OLED-Zyklustexte in DE/EN/IT/FR und drei schwaebischen Varianten: START, Haltebalken, Loslassen, FEIERABEND, PAUSE und unbekannte Zeit.
- Kompakte, statische Sprachtexte ohne neue Heap-Allokation oder Display-Puffer.
- Zaehlung, Arbeitszyklus- und Datenformate unveraendert.

# Release 3.6.6

- Bei einem aktiven Arbeitszyklus erscheint nach 250 ms ununterbrochenem Halten des physischen Knopfes eine OLED-Vorschau: FEIERABEND, breiter Fortschrittsbalken und lokalisierte Aufforderung.
- Bei 2 Sekunden ist der Balken voll. Erst Loslassen nach Erreichen der Halteschwelle fuehrt wie bisher zum wirklichen Feierabend, mit 10-Sekunden-Abschlussanzeige.
- Fruehes Loslassen setzt die Anzeige zurueck; jeder neue Druck beginnt wieder bei null. Kurzes Druecken bleibt eine normale Unterbrechung.
- Displayvorschau aktualisiert maximal alle 85 ms im bestehenden Loop, ohne weitere Tasks, Flash-Schreibzugriffe oder neuen Puffer.
- Keine Aenderungen bestehender Persistenzformate oder Auswertungen.

# Release 3.6.5

- Die Einstellung "Automatisches Zyklusende" liegt in einer eigenen, ausfuehrlich erklaerten Arbeitszyklus-Karte.
- OLED-Standardansicht: beim aktiven Zyklus links unten die letzte Unterbrechung, nach manuellem oder automatischem Ende "PAUSE". "FEIERABEND" bleibt zuvor fuer 10 Sekunden.
- Web-Startseite: dasselbe Signal "PAUSE" mit Erklaerung, sobald der Arbeitszyklus geschlossen ist. Bei ungueltiger lokaler Uhrzeit ein eigener Status statt falsch suggerierter Pause.
- Zyklusstatus nutzt bestehende Live-Abfrage, kein neuer Hintergrundtimer; Datenformate und Flash-Schreibstrategie unveraendert.
- Alle sieben UI-Sprachen aktualisiert.

# Release 3.6.4

- OLED TV-Anti-Spam-Animation zeigt grosse lesbare Warnung in einer oder zwei Zeilen, bei allen Animationsbildern.
- Beim Start des Zyklus erscheint fuer zwei Sekunden ein grosses lokalisiertes "LOS GEHT'S!" (kein Zaehlen, kein normaler Ton, keine Anti-Spam-Sperre).
- Einstellbare automatische Endzeit der Arbeitszyklen in der Weboberflaeche (HH:MM, Standard 00:00); ein Start nach der Endzeit laeuft bis zur Endzeit des folgenden lokalen Tages.
- Der normale Kalendertag fuer Tageszaehler/Analytik wechselt unveraendert um 00:00; manueller Abschluss per langem Druck bleibt.
- Ein einzelner kompakter NVS-Wert wird nur nach tatsaechlicher Aenderung geschrieben; keine Aenderung bestehender Ring-/Aggregat-/Zyklusdatenformate.
- Firmware-Build erzeugt das deterministische Web-Asset vor dem Kompilieren neu.

# Release 3.6.3

- Standard-OLED links oben nur noch zwei gross dargestellte Icons fuer WLAN und gueltige Uhrzeit
- links unten nur die vergangene Zeit seit der letzten Unterbrechung (ohne Uhrzeit)
- rechts bleibt die dynamisch maximierte Tageszahl
- keine Aenderung der Datenformate oder anderen OLED-Modi

# Release 3.6.2

- neue SH1106-Standardansicht mit rechts maximierter Tageszahl (dynamische horizontale und vertikale Skalierung)
- links oben WLAN-, Uhr-, Speicher- und Tonstatus, links unten Zeitpunkt und Alter der letzten Unterbrechung
- Zufallsstart für normalen Wechselton nach Neustart, anschließend unveränderte Rotation 3…N; fester Basiston bei unbekannter Trackanzahl
- kein neues persistentes Format, kein zusätzlicher Flash-/NVS-Schreibpfad und keine Änderung anderer OLED-Modi
- vorhandene 3.6.1-Zykluslogik vollständig beibehalten

# Release 3.6.1

- Arbeitszyklus auf DI1/GPIO13 **neu integriert**, nachdem der verzögerte Pending-Ansatz aus dem 3.6.0-Teststand im Alltag zu falschem Feedback und verspäteter Erfassung geführt hat
- erster kurzer Druck startet den Arbeitszyklus und zählt nicht als Unterbrechung
- der START-Druck öffnet **keine** 10-Sekunden-Anti-Spam-Sperre und erzeugt weder Track 2 noch einen normalen Unterbrechungston
- jeder gültige kurze Druck während eines aktiven Zyklus wird **sofort** über den bestehenden `InterruptionService` gespeichert, gezählt und mit dem normalen Track ab 3 bestätigt
- nur tatsächlich zu schnelle weitere Kurzdrucke innerhalb der 10-Sekunden-Sperre werden verworfen und erhalten Track 2 plus OLED-Anti-Spam-Feedback
- langer Druck ab **2 Sekunden** beendet den Zyklus ausdrücklich und ist von der Kurzdruck-Sperre unabhängig
- wird der lange Enddruck vergessen, schließt der Zyklus beim lokalen Tageswechsel automatisch; bereits erfasste Unterbrechungen werden dabei **nicht rückwirkend umklassifiziert**
- nach manuellem Ende zeigt das SH1106 **10 Sekunden `FEIERABEND` plus heutige Unterbrechungszahl**; währenddessen werden weitere physische Eingaben ignoriert und die normale Home-/Sekundenanzeige darf die Abschlussanzeige nicht überschreiben
- alter 3.6.0-Pending-Zustand wird durch eine neue Zyklus-Zustandskennung nicht übernommen; nach dem Update startet die neue Logik mit einem sauberen Zustand
- START/ENDE bleiben getrennt in `/cycles.log`; der aktive Zyklus benötigt nur einen kleinen NVS-Zustand ohne Pending-Kandidaten
- bestehender Raw-Ring bleibt **100.000 × 9 Byte** und bytekompatibel zu 3.5.x; Tagesaggregate, CSV, Heatmaps, Fokus und Herkunftsfilter bleiben auf echten Unterbrechungen aufgebaut
- Webbutton bleibt unabhängig und erzeugt weiterhin sofort Unterbrechungen

## Korrigiertes Bedienmodell

```text
inaktiv + kurzer Druck     -> START, still, keine Sperre
aktiv + kurzer Druck       -> sofort UNTERBRECHUNG
< 10 s nach Unterbrechung  -> weiterer Kurzdruck wird unterdrückt, Track 2
aktiv + Druck >= 2 s       -> ENDE, 10 s FEIERABEND
kein ENDE bis Tageswechsel -> Zyklus automatisch schließen
```

## Kompatibilität

Vorhandene 3.x-Unterbrechungsdaten benötigen keine Migration. Der 3.6.0-Pending-Teststand wird nicht als Datenmodell fortgeführt; eine dort noch offene Zyklus-Pending-Struktur wird von 3.6.1 absichtlich nicht geladen.

# Release 3.5.0

- Fokus-&-Ruhe-Karte auf Home mit aktueller/längster Tages-/Wochenphase und erklärtem 120-Minuten-Trend
- Arbeitsmuster aus bis zu 30 abgeschlossenen Tagen: ruhigstes vollständig beobachtetes 2-h-Fenster und stärkste vollständig beobachtete 1-h-Stunde, jeweils ab fünf abgedeckten Tagen
- zwei neue große OLED-Modi für Ruhephasen und Arbeitsmuster, nicht blockierend und sprachabhängig
- Projekteinstellungen aus Home unverändert in eine eigene Kachel unter Einstellungen verschoben
- zentrale, maximal 60 s alte RAM-Auswertung statt permanenter Raw-Ring-Scans; kein neues Speicherformat, RawEvent weiterhin 9 Byte
- alle sieben vorhandenen UI-Sprachen nachgezogen

# Release 3.4.1

- temporäre 500-ms-Web-Nachführung eines manuellen DY-SV17F-Audiotests bis zum Abschluss, maximal 120 s
- keine zusätzliche permanente Frontend-Abfrage außerhalb eines gestarteten Audiotests
- OLED-TV-Störung läuft nach dem ersten verworfenen physischen Druck bis zum Ende der aktiven 10-s-Sperre und wird durch weitere Spam-Drücke nicht neu gestartet
- Track-2-Fast-Path und bestehende Anti-Spam-Zähl-/Speicherlogik bleiben unverändert
- README technisch auf den aktuellen 3.x-Versionsstand synchronisiert

# Release 3.4.0

- 10-s-Anti-Spam für DI1/GPIO ohne Einfluss auf gespeicherte Unterbrechungsdaten
- Track 2 reserviert für Spam-Feedback; normale Rotation ab Track 3
- sofortiger Track-2-Fast-Path und nicht blockierendes OLED-TV-Flimmern
- boot-lokaler Diagnosezähler für verworfene physische Drücke

# Release 3.3.2

- manueller DY-SV17F-Audiotest beendet sich anhand gezielter UART-Statusabfragen sicher, auch wenn keine weitere BUSY-Flanke kommt
- UART wird nur während des ausdrücklich gestarteten Audiotests in 500-ms-Abständen auf das Trackende geprüft; außerhalb des Tests bleibt die Diagnose ereignisgesteuert
- BUSY bleibt zusätzliche Hardwarediagnose: eine unbestätigte Polarität verhindert weder funktionierende Wiedergabe noch einen erfolgreichen UART-End-to-End-Test
- funktionierender PLAY-Pfad, Boot-Ton, Rotation, Lautstärke und normale Unterbrechungstöne bleiben unverändert

# Release 3.3.1

- DY-SV17F-Diagnose zeigt UART-Wiedergabestatus und BUSY-Rohpegel getrennt samt Messzeit
- kein permanentes BUSY-Polling mehr; BUSY-Flanken nur während des manuellen Audiotests
- `Prüfen` bleibt lautlos, `Ton testen` sammelt UART/BUSY-End-to-End-Diagnose
- BUSY-Polarität wird nicht geraten und kann die funktionierende Wiedergabe nicht blockieren
- Wiedergabepfad, Tracklogik, Lautstärke und UART-Pinbelegung bleiben unverändert

# Release 3.3.0

- Passwortgeschütztes vollständiges Löschen der Ereignisdatenbank (Bestätigung = Projektname)
- Heatmap-Herkunftsfilter für Beides / Knopf-GPIO / Web, mit offenem API-Modell für spätere Quellen
- konkrete Speicherfehler in Gerät → Speicher
- bestehende 3.1/3.2-Regeln für Ø-Abstand, Raw-Ring-Coverage, Embedded-Effizienz und nicht blockierende Bedienung bleiben erhalten

# Unterbrechungszähler 3.2.0

- OLED folgt der gewählten UI-Sprache mit ressourcenschonender Transliteration für Sonderzeichen
- Bootscreen mindestens 4 Sekunden, weiterhin nicht blockierend
- persistente 180°-Drehung und fünf Displaymodi inklusive Tagesfortschritt/Fokus
- frische Displaydefaults 65 % / 5 %
- DY-SV17F-Lautstärke 0–100 %, Standard 100 %; Wechselmodus ist neuer Standard
- Einstellungen klar nach Display, Display-Feedback und Sound gegliedert
- veralteter Arduino-Sketch-BIN-Hinweis im OTA-UI entfernt

# Release Notes – Unterbrechungszähler 3.1.0

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

# Release Notes – Unterbrechungszähler 3.0.1

3.0.1 ist ein Patch-Release auf Basis von 3.0.0.

## Änderungen gegenüber 3.0.0

- passwortgeschützter Fallback-AP mit `Unterbrechungszähler`
- alte statische OTA-/AP-Warnungen aus der normalen Oberfläche entfernt
- echte OTA-Fehlerdiagnose bleibt für tatsächliche Updatefehler erhalten
- OTA-Speicher zusätzlich als segmentierter Auslastungsbalken
- `ota.usedPercent` in der Geräte-API
- aktualisierte Releasechecks und Dokumentation

---

# Release Notes – Unterbrechungszähler 3.0.0

3.0.0 ist der neue Ausgangspunkt des Projekts. Frühere 1.x/2.x-Stände waren Entwicklungs- und Testversionen; es gibt keine zugesicherte Migration.

## Enthalten

- Unterbrechungstaster auf DI1/GPIO13 und Webbutton
- Live-Tageszähler und letzte Unterbrechung
- Europe/Berlin-Kalenderauswertung
- 100.000er Raw-Ringspeicher
- 2.300 Tagesaggregate
- CSV-Streamingexport
- drei Heatmap-Ansichten
- SH1106-Projektansichten und Displayflash
- DY-SV17F Boot-/Unterbrechungstöne
- fester oder rotierender Unterbrechungstrack
- NVS-gespeicherte Sound-/Displayeinstellungen
- DS3231/NTP/Browser-Zeitquellen
- OTA mit eigener 4-MiB-Partitionstabelle
- UI in Deutsch, Englisch, Italienisch, Französisch, Schwäbisch, Alb-Schwäbisch und Oberschwäbisch

Die ausführliche README-Dokumentation wird bewusst nur in Deutsch, Englisch und Schwäbisch gepflegt.

## Hardware 3.0.0

- DI1 GPIO13
- I2C GPIO21/22
- DY-SV17F UART GPIO18/19
- DY-SV17F BUSY GPIO39 mit externem Pull-up

## Release-Gates

Portable Hostchecks und der echte Arduino-ESP32-Build laufen über GitHub Actions. Der Release wird erst nach erfolgreichem Build auf `main` erzeugt.

Reale Hardwaretests bleiben zusätzlich erforderlich; Details stehen in `TEST_REPORT.md`.
