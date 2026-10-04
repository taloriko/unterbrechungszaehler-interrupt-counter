# Testbericht – Unterbrechungszähler 3.6.1

Auditdatum: 16.09.2026
Ziel: ESP32 Dev Module / ESP32-WROOM-32, Arduino-ESP32

## Schwerpunkt 3.6.1

3.6.1 ersetzt den im 3.6.0-Teststand verwendeten verzögerten Pending-Kandidaten durch eine direkte, dünne Arbeitszyklus-Klassifikation vor dem bewährten Unterbrechungsservice.

Geprüfte Sollregeln:

- erster kurzer DI1-Druck bei inaktivem Zyklus = START, keine Unterbrechung
- START startet keine 10-Sekunden-Anti-Spam-Sperre und darf Track 2 nicht auslösen
- gültiger kurzer DI1-Druck bei aktivem Zyklus = sofort `InterruptionService::capture(PhysicalButton)`
- normaler physischer Capture behält den bewährten sofortigen Feedbackpfad mit Track 3+
- nur ein weiterer Kurzdruck innerhalb der 10-Sekunden-Sperre erhält Track 2 / OLED-Anti-Spam
- langer Druck ab 2 Sekunden = END, unabhängig vom Kurzdruck-Cooldown
- kein Pending-Kandidat und keine rückwirkende Umklassifizierung bereits erfasster Unterbrechungen
- vergessener END-Druck = automatisches Schließen des Zyklus beim lokalen Tageswechsel
- manuelles END = 10 Sekunden FEIERABEND plus Tageszahl; physische Eingabe und normale OLED-Ansicht bleiben in diesem Fenster blockiert
- alter CYC1-Zustand aus dem 3.6.0-Teststand wird nicht als CYC2 geladen
- Raw-Format bleibt 9 Byte und Daily-Format 64 Byte

## Automatisierte Prüfungen

Der Releasecheck `tools/release_check.py` prüft unter anderem:

- Projektname und Version 3.6.1
- 100.000er Raw-Ring und 2.300 Tagesaggregate
- unverändertes 9-Byte-Raw-Format
- DI1 auf GPIO13 mit aktivem Edge-Latch
- WorkCycle als alleinige DI1-Klassifikationsschicht
- START ohne `PhysicalButtonGuard::accept()`, Audioaufruf oder Suppression
- Anti-Spam-Guard vor dem normalen Capture echter Unterbrechungen
- direkten Aufruf des normalen `InterruptionService::capture(PhysicalButton)`
- vollständige Entfernung der 3.6.0-`captureAtEpoch()`-/Pending-Schnittstelle
- Track 2 ausschließlich im Suppression-Pfad
- 2-Sekunden-END und 10-Sekunden-FEIERABEND
- exklusiven Goodbye-Displaypfad
- DY-SV17F auf GPIO18/19 plus BUSY GPIO39
- Projekt-API-Routen
- Storage-/Recovery-Simulationen
- JavaScript-Syntax
- deterministisches Webbundle, ETag und gzip-Roundtrip

GitHub Actions führt diese Checks zusätzlich aus, regeneriert `web_assets.h` zur Diff-Prüfung und kompiliert den echten Sketch mit Arduino-ESP32. Ein Release wird blockiert, wenn weniger als 64 KiB Reserve im App-Slot verbleiben.

## Bestehende Hosttests

- Storage-/Recovery-/Heatmap-Simulationen einschließlich Ø-Abstandssemantik und Raw-Ring-Coverage
- 100.000er Ring-Wrap
- `PhysicalButtonGuard`-Regeln für akzeptierte und verworfene Drücke
- Focus-&-Insights-Regeln
- identische i18n-Keysets der Basis-Sprachpakete
- zusätzliche UI-Pakete Italienisch, Französisch, Alb-Schwäbisch und Oberschwäbisch
- Webbundle/ETag/gzip-Prüfung

Diese Hostprüfungen ersetzen nicht den echten Arduino-Build; deshalb ist die GitHub-CI das verbindliche Software-Releasegate.

## Reale Hardwaretests für 3.6.1

Nach dem CI-Build auf einem Zielgerät besonders in dieser Reihenfolge prüfen:

1. **frischer Startzustand:** kurzer Druck startet Zyklus; kein Track 2, kein normaler Unterbrechungston, Tageszähler unverändert
2. **erste echte Unterbrechung direkt danach:** auch deutlich unter 10 Sekunden nach START drücken; sie muss sofort zählen und einen normalen Track ab 3 spielen
3. **Anti-Spam:** innerhalb von 10 Sekunden nochmals kurz drücken; dieser zweite Druck muss verworfen werden und Track 2/OLED-TV-Feedback auslösen
4. **nächste gültige Unterbrechung:** nach Ablauf der 10 Sekunden kurz drücken; Zähler muss sofort um genau eins steigen
5. **Feierabend während Cooldown:** direkt nach einer Unterbrechung mindestens 2 Sekunden halten; END muss trotz laufendem Cooldown funktionieren und darf Track 2 nicht auslösen
6. **Goodbye-Lock:** während der 10 Sekunden FEIERABEND dürfen weder Sekunden-/Home-Anzeige aufblitzen noch physische Drücke erfasst werden
7. **nach Goodbye:** nächster vollständiger kurzer Druck startet einen neuen Zyklus sauber
8. **Reboot im aktiven Zyklus:** CYC2-Zustand bleibt aktiv; normale Unterbrechungserfassung funktioniert danach weiter
9. **Update von 3.6.0-Teststand:** alter CYC1/Pending-Zustand darf nicht geladen werden; 3.6.1 beginnt mit sauberer Zyklussemantik
10. **Tageswechsel ohne END:** Zyklus schließt automatisch, bereits gespeicherte Unterbrechungen bleiben unverändert; erster Druck des neuen Tages startet neu

Zusätzlich weiterhin prüfen:

- Debounce und kurzer Tastendruck während längerer HTTP-/CSV-Ausgabe
- SH1106-Ansichten, Helligkeit, Flash, Display Ein/Aus und Bootbild
- DY-SV17F: Boot/Test Track 1, Anti-Spam Track 2, normale Töne ab Track 3, Lautstärke und BUSY-Diagnose
- DS3231-Fallback und NTP-Zeit
- Offlinebetrieb ohne WLAN
- LittleFS-Persistenz über Reboot
- OTA mit Erhalt von NVS/LittleFS
- CSV mit echten Ereignissen
- Heatmaps Anzahl/Ø Abstand auf Desktop und Mobilgerät
- wiederholte Schreibvorgänge/Reboots im Langzeittest

## Freigabe

3.6.1 darf nach erfolgreicher GitHub-CI als Software-Releasekandidat verwendet werden. Die oben genannten realen Hardwaretests bleiben die praktische Gerätevalidierung; insbesondere Tonfolge, DI1-Kurz/Langdruck und das 10-Sekunden-Goodbye-Overlay müssen am echten Aufbau bestätigt werden.
