# Zeitarchitektur – Unterbrechungszähler 0.1.0 (Basis 1.6.0)

## Verantwortung

`TimeService` ist die einzige Instanz, die die aktive Zeitquelle auswählt. RTC-, NTP-, Browser-, UI- und spätere Eventlogik treffen keine eigene Quellenentscheidung.

```text
NtpTimeProvider -----+
                     |
RtcDs3231 -----------+--> TimeService --> API / Web-UI
                     |                  --> Projektlogik
Browser-Fallback ----+                  --> Display
                                        --> InterruptionService / Eventdaten
```

Priorität: **NTP > RTC > Browser > relative Zeit**.

## Asynchroner Bootablauf

1. `HardwareRegistry` liest die DS3231 beim Boot, falls aktiviert und erreichbar.
2. `WifiModule::begin()` startet STA ohne Verbindungs-Warteschleife.
3. `TimeService::begin()` lädt den gespeicherten NTP-Server.
4. Ist die RTC gültig, wird sie sofort als provisorische absolute Quelle verwendet. Sonst beginnt die Zeit relativ.
5. Solange der initiale WLAN-Verbindungsversuch noch läuft, bleibt die einmalige NTP-Prioritätsprüfung vorgemerkt.
6. Sobald STA verbunden ist **oder** der WLAN-Start endgültig in AP/Disconnected übergeht, führt `TimeService::update()` genau eine Bootprüfung aus.
7. NTP gültig: NTP wird Referenz, Systemzeit wird gesetzt und RTC wird auf UTC nachgeführt/verifiziert.
8. NTP ungültig: gültige RTC bleibt/werden Referenz.
9. NTP und RTC ungültig: Browser-Fallback wird einmalig erlaubt.
10. Keine absolute Quelle: `relative` mit monotonic/Uptime.

Nach diesem Bootablauf gibt es keinen periodischen NTP- oder RTC-Poller.

## UTC-Regel

Epochwerte und DS3231 werden intern als UTC behandelt. Für **Projektlogging und Statistik** leitet `ProjectTime` daraus zentral die lokale Zone `Europe/Berlin` (`CET-1CEST,M3.5.0,M10.5.0/3`) ab. Der Browser ist damit nicht für die Zuordnung zu lokalem Tag, Stunde, ISO-KW oder Monat verantwortlich.

Eine RTC, die vorher Lokalzeit enthielt, kann beim ersten erfolgreichen NTP-Abgleich einmalig eine Zeitzonendifferenz zeigen; danach ist ihr Inhalt UTC. Darstellung und Projektkalender werden anschließend getrennt aus UTC abgeleitet.

## Samples und Differenzen

Jedes Sample enthält:

- Verfügbarkeit
- Gültigkeit
- Epoch in ms
- monotonen Messzeitpunkt

Vergleiche werden auf einen gemeinsamen monotonen Zeitpunkt normalisiert. Dadurch wird die Laufzeit einer Netzwerkanfrage nicht als scheinbarer RTC-Drift gezählt.

Eine RTC mit OSF ist nicht vertrauenswürdig und darf nicht aktive Quelle sein. Ein plausibel lesbarer Kalenderwert bleibt aber als Diagnosesample sichtbar und kann gegen NTP verglichen werden.

## NTP

`NtpTimeProvider` verwendet UDP/123 und keinen dauerhaften SNTP-Client.

Prüfung:

- WLAN-STA muss verbunden sein
- Servername validieren
- DNS/IP auflösen
- UDP-Socket öffnen
- NTP-Anfrage mit Request-Nonce senden
- Servermodus, NTP-Version 3/4, Stratum und Leap-Status prüfen
- Originate-Timestamp gegen Nonce prüfen
- Zeitwert auf konfigurierten Jahresbereich plausibilisieren

UDP-Antwortwartezeit ist durch `NTP_RESPONSE_TIMEOUT_MS` begrenzt. Die Namensauflösung verwendet den synchronen Resolver des Arduino-ESP32-Core und ist damit die verbleibende nicht hart begrenzte Stelle dieser Zeitprüfung. Da NTP nur beim Boot oder auf ausdrückliche Bedienaktion geprüft wird, läuft daraus kein Hintergrundpolling.

Ein neuer Server wird erst nach einer gültigen Antwort in NVS (`espui-time` / `ntp`) gespeichert. Ein fehlgeschlagener Kandidat verändert weder den gespeicherten Server noch die aktive Zeit.

## NTP → RTC

Bei gültigem NTP:

1. RTC-Sample vor Korrektur sichern
2. Differenz gegen NTP normalisieren
3. Systemzeit setzen
4. DS3231 auf NTP/UTC schreiben
5. DS3231 erneut lesen
6. nur bei gültiger Verifikation `rtcSyncOk=true`

Ein RTC-Schreibfehler macht die weiterhin gültige NTP-Systemzeit nicht ungültig.

## Browser-Fallback

Die UI sendet Browserzeit nur, wenn die Firmware `browserFallbackAllowed=true` meldet. Die Firmware prüft die Priorität beim POST erneut.

Browserzeit:

- ist nur Fallback
- wird plausibilisiert
- setzt die ESP32-Systemzeit
- schreibt die RTC **nicht**
- wird nach erfolgreicher Annahme nicht beliebig erneut akzeptiert

Eine bewusste manuelle Zeitprüfung kann nach einem vollständig fehlgeschlagenen Quellencheck einen neuen Browser-Fallback-Versuch freigeben.

## EventLogging-Vertrag

Der Unterbrechungszähler verwendet bereits:

```cpp
const TimeTypes::Snapshot ts = TimeService::eventTimestamp();
```

Der Snapshot trägt atomar:

- `valid`
- `epochMs`
- `monotonicMs`
- `source`
- `quality`

Jede Unterbrechung speichert diesen Snapshot beim Entstehen kompakt im Raw-Ring. Historische Ereignisse werden durch eine spätere bessere Zeitquelle nicht rückwirkend umgerechnet. Für Tages-/Stundenstatistik wird bei gültiger absoluter Zeit zusätzlich die zentrale Projektzone `Europe/Berlin` abgeleitet.

Beispiel:

```text
10:43:17.247 | NTP      | DI1_CHANGED | HIGH
10:44:02.011 | RTC      | SOFTWARE    | ACTION
+00:03:17    | RELATIVE | DI3_CHANGED | LOW
```

Ein späterer Quellenwechsel kann als eigener `TIME_SOURCE_CHANGED`-/`TIME_SYNC`-Event protokolliert werden.

## API

| Methode | Endpoint | Wirkung |
|---|---|---|
| GET | `/api/time` | aktuellen fortgeführten Zustand lesen, keine neue Quellenprüfung |
| POST | `/api/time/check` | bewusste komplette RTC/NTP-Prüfung |
| POST | `/api/time/ntp?server=...` | Kandidat prüfen; nur bei Erfolg speichern und verwenden |
| POST | `/api/time/browser?epochMs=...&tzOffset=...` | einmaliger Browser-Fallback, nur wenn erlaubt |

## Keine Verantwortungsdopplung

- `RtcDs3231`: Register, I2C, Health, Kalenderwert
- `NtpTimeProvider`: NTP-Paket, Serverkonfiguration, NVS
- `TimeService`: Priorität, Systemzeit, Plausibilitätspolitik, Differenzen, RTC-Nachführung
- Web-UI: Darstellung und Bedienaktionen
- zukünftiges EventLog: speichert nur Snapshot + Ereignis


## Verwendung im Unterbrechungszähler

Jede Unterbrechung erhält genau beim Capture einen `TimeService::eventTimestamp()`-Snapshot. Gespeichert werden Zeitwert und Herkunft (`ntp`, `rtc`, `browser` oder `relative`). Es gibt **keine NTP- oder RTC-Abfrage pro Tastendruck**. Der lokale Kalendertag für Delta und Statistik wird anschließend über die Projektzeitzone `Europe/Berlin` abgeleitet. Historische Records behalten ihre ursprüngliche Zeitquelle auch nach einer späteren besseren Synchronisation.
