# Release 2.1.2

## Schwerpunkt

Version 2.1.2 optimiert die DS3231-RTC und korrigiert die Moduldiagnose des Watchdogs.

## RTC DS3231

- RTC wird nicht mehr durch Weboberflaeche oder Display laufend ueber I2C gelesen.
- Zentraler RTC-Healthcheck im Abstand von 60 Sekunden.
- Zeit, Status und Temperatur werden in einem gemeinsamen Register-Burst gelesen und zwischengespeichert.
- Web-/Display-Ausgaben verwenden den Cache; die angezeigte RTC-Zeit wird aus Cache plus vergangener Laufzeit fortgeschrieben.
- OSF (Oscillator Stop Flag) wird ausgewertet, damit ein moeglicher Verlust der gueltigen RTC-Zeit erkannt wird.
- Interne DS3231-Temperatur wird mit 0,25 Grad C Aufloesung erfasst.
- Abweichung zwischen RTC und ESP32-Systemzeit wird nach einem Healthcheck berechnet, sobald eine gueltige Systemzeit vorhanden ist.
- Ungenutzter 32-kHz-Ausgang sowie ungenutzte Alarm-/Square-Wave-Ausgaben werden deaktiviert. Die fest verdrahtete Power-LED vieler RTC-Module kann weiterhin nicht per Software abgeschaltet werden.
- Eine echte Batteriespannung bzw. ein Prozentwert der Backup-Batterie kann der DS3231 nicht liefern. OSF dient stattdessen als Hinweis auf einen moeglichen Zeitverlust.
- Beim erfolgreichen NTP- oder Browser-Zeitabgleich wird die RTC weiterhin synchronisiert.

## Watchdog / Moduldiagnose

- MainLoop misst jetzt den kompletten Loop-Zyklus statt einen verschachtelten Modulblock.
- Modulpfade behalten ihre eigenen Laufzeitmessungen.
- RTC-Lebenszeichen entsteht nur noch nach einem echten RTC-Healthcheck und nicht kuenstlich in jedem Loop.
- Passive Module wie Storage und Analytics werden nicht mehr pro Loop mit einem Fake-Heartbeat auf "jetzt" gehalten.
- Heartbeats loeschen die letzte gemessene Laufzeit nicht mehr.
- Langsam-Erkennung verwendet modulspezifische Grenzwerte statt eines pauschalen Grenzwerts fuer alle Module.
- Fuer langsame Aufrufe werden Zeitpunkt und Dauer des letzten langsamen Laufs intern gespeichert.
- Die RTC besitzt wegen ihres 60-Sekunden-Zyklus ein eigenes groesseres Heartbeat-Zeitfenster.
- Optionale, nicht vorhandene Hardware bleibt ein Fachzustand und wird nicht als ESP32-Hardware-Watchdogfehler behandelt.

## Diagnoseausgabe

Die serielle Diagnose zeigt fuer die RTC zusaetzlich:

- RTC-Datum und RTC-Zeit
- OSF-Status
- DS3231-Temperatur
- Abweichung zur ESP32-Systemzeit
- Laufzeit des letzten RTC-Healthchecks

## Testpunkte auf Hardware

1. RTC angeschlossen: Erkennung, Zeit und Temperatur plausibel.
2. Weboberflaeche laenger offen lassen: RTC-I2C-Zugriffe sollen weiterhin nur etwa einmal pro Minute stattfinden.
3. RTC abziehen und nach spaetestens einem Healthcheck als nicht erkannt pruefen.
4. RTC wieder anstecken und automatische Wiedererkennung pruefen.
5. NTP-Synchronisation ausloesen und kontrollieren, dass RTC und Systemzeit wieder uebereinstimmen.
6. Watchdog pruefen: RTC-Lebenszeichen soll nicht permanent "jetzt" anzeigen.
7. Langsame Webzugriffe beobachten: MainLoop und Web duerfen unterschiedliche diagnostische Bedeutung behalten.
