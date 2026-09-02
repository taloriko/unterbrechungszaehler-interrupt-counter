# Baseline-Audit – ESP32 UI Base 1.6.0 FINAL

## Ziel des Audits

Rückwärtsprüfung des gesamten gewachsenen Standes gegen die ursprünglichen Grundregeln sowie gegen die später ausdrücklich beauftragten Erweiterungen. Schwerpunkt: Wiederverwendbarkeit, Ressourcen, Blockierungen, Verantwortungsgrenzen und Dokumentationskonsistenz.

## Ergebnis

**Freigabe als Projektbasis: JA, mit dokumentierten Plattform-/Wartungsgrenzen.**

Im Audit gefundene eigene Schwachstellen wurden vor dem Freeze korrigiert:

1. STA-WLAN-Start blockierte früher bis zum Timeout → jetzt vollständig millis()-gesteuert.
2. HTTP-Server konnte vor verfügbarer Netzwerkschnittstelle gestartet werden → Start erfolgt jetzt erst mit STA/AP-IP.
3. AP-only-Fallback wurde nach einem fehlgeschlagenen ersten `softAP()` nicht erneut versucht → kooperativer Retry ergänzt.
4. Pin-Konflikte wurden früher nur geloggt → harte Pinfehler blockieren jetzt die optionale Hardwareinitialisierung fail-safe.
5. GPIO34–39 mit unzulässigem internen Pull wurden nicht als Konfigurationsfehler erkannt → Validierung ergänzt.
6. GPIO-Output-Feedback konnte eine gewünschte Verzögerung nicht vollständig ohne Blockierung abbilden → Deadline-basierte Verifikation ergänzt.
7. Audio-Kommandos konnten pending Verifikationen überschreiben → Command-Path wird serialisiert.
8. unnötiges UART-`flush()` entfernt.
9. RTC-Kalenderprüfung und Schreibverifikation verschärft.
10. Browser-Zeitzonenparameter serverseitig streng validiert.
11. verstreute Frontendfarben auf Design-Tokens zurückgeführt.
12. tote Transportfunktionen entfernt.
13. Favicon folgt nun dem zentral konfigurierten Projekt-Icon statt dauerhaft dem Basis-Chip.
14. Gerätekartenreihenfolge auf Gerät → WLAN → Speicher → Zeit → Hardware → OTA festgelegt.
15. Teilweise fehlgeschlagene RTC-Registerreads werden fail-closed als ungültig markiert; kein partieller/staler Zeitwert kann aktive Quelle werden.
16. Manueller Audio-Healthcheck wird während einer laufenden Audio-Kommandobestätigung abgewiesen, damit Zustände nicht überschrieben werden.
17. Hardware-JSON wird direkt in den reservierten API-Antwortpuffer geschrieben; ein zusätzlicher großer temporärer `String` entfällt.
18. Status-Provider-Kapazität ist zentral konfiguriert (`STATUS_PROVIDER_CAPACITY = 16`) statt als lokale Magic Number.
19. Zielboard-Pinvalidierung verwendet die tatsächlich herausgeführten GPIOs des klassischen ESP32 Dev Module/WROOM-Pinplans.
20. Hardware-Check-API unterscheidet jetzt unbekannte Module (`404`) von bekannten, aber momentan belegten/abgewiesenen Checks (`409`); ein Busy-Zustand wird im Frontend nicht mehr als API-Ausfall fehlbewertet.
21. Audio-Verifikation kennt die erwartete Wirkung von Play/Stop. BUSY kann damit sowohl gestartete Wiedergabe als auch einen bestätigten Stop korrekt absichern, falls die UART-Statusantwort fehlt.
22. Fallback-AP-SSID wird ohne `strncpy` klar begrenzt und bei 32-Byte-Trunkierung nicht mitten in einer UTF-8-Fortsetzungssequenz abgeschnitten.
23. NTP-Antworten werden nur noch mit den unterstützten Protokollversionen 3/4 akzeptiert.
24. DS3231-Rohwerte werden zusätzlich auf gültiges BCD und gültigen Wochentag geprüft; beschädigte Registerdaten fallen fail-closed durch.

## Abgleich mit später beauftragten Erweiterungen

Die Ursprungsspezifikation schloss OTA, RTC, Audio und projektspezifische Zusatzhardware zunächst ausdrücklich aus. Diese Punkte wurden später vom Benutzer bewusst beauftragt. Im Freeze gelten sie deshalb als **genehmigte Infrastrukturerweiterungen**, nicht als Verstoß gegen das ursprüngliche Ballastverbot. Sie bleiben generisch, optional und von der späteren Unterbrechungszähler-/Projektlogik entkoppelt.

## Prüfung der ursprünglichen Architekturregeln

| Bereich | Ergebnis | Bemerkung |
|---|---|---|
| Stabilität vor Dekoration | PASS | Animationen auf kurze Bedienübergänge begrenzt; reduced-motion vorhanden |
| ESP32-Ressourcen | PASS | kleines Webasset, 1-KiB-OLED-Framebuffer, feste kleine Registries, keine Frontendframeworks |
| Vanilla HTML/CSS/JS | PASS | keine externen Frontendabhängigkeiten |
| Keine CDN-/Internetpflicht | PASS | gesamte Grund-UI lokal im PROGMEM |
| Arduino-Standardkomponenten bevorzugt | PASS | keine zusätzliche Library erforderlich |
| Lesbare Entwicklungsquellen + kompaktes Deployment | PASS | `ui-src/*` + `tools/build_web.py` + gzip `web_assets.h` |
| Zentrale Konfiguration | PASS | `config.h`, `hardware_config.h`, `UI_CONFIG` |
| Kontrollierter Frontend-State | PASS | zentraler `state`, gezielte Bindings/Patches |
| Transport/UI getrennt | PASS | `Transport`/API getrennt von Renderern |
| Kein permanentes Geräte-HTTP-Polling | PASS | Device einmal pro Page-Load; Hardware/Zeit nur Aktion; OTA-Reconnect nur nach OTA |
| Browser-Uhr lokal weiterführen | PASS | einziger dauerhafter 1-s-Frontendtick |
| i18n DE/EN/SWG | PASS | identische Keysets; localStorage; Browsererkennung; EN-Fallback |
| Theme System/Light/Dark | PASS | localStorage + `prefers-color-scheme` Event |
| Zentrale Designfarben | PASS | CSS Custom Properties; keine komponentenweisen freien Statusfarben |
| Native Semantik/Accessibility | PASS | header/nav/main/footer/section/button/select/input; focus-visible; aria-labels |
| SVG-Icons ohne Bibliothek | PASS | Inline-Sprite; dynamisches Projekt-Favicon |
| Modularer Headerstatus | PASS | `StatusRegistry`; Position nicht hart verdrahtet |
| Responsive 320–480 px | STATISCH PASS | Breakpoints/Stacking/Icon-Navigation; echter Geräte-Browsertest bleibt manuell |
| Kein horizontaler Navigationsscroll | PASS | Navigation komprimiert auf Icons statt Scrollcontainer |
| Universeller Kartenbaukasten | PASS | KV/Status/Select/Switch/Action/Meter/Notice/List/Upload/Hardware/Time |
| Ressourcenschonende Meter | PASS | ein Balkenelement; CSS-Segmentierung |
| Optional fehlende Werte | PASS | KV-Zeilen werden nur bei vorhandenen Daten gezeigt |
| JSON ohne ArduinoJson | PASS | gemeinsames Escaping in `JsonUtils` |
| Cache ohne Service Worker | PASS | gzip + ETag + must-revalidate |
| Kein projektspezifischer Ballast | PASS | Home bleibt neutral; spätere Nutzerwünsche sind Infrastrukturmodule |
| Keine Pseudocode-Lücken | PASS | benötigte Basisdateien vollständig |

## Später ausdrücklich geänderte Anforderungen

Die Ursprungsspezifikation sagte für die erste Grundversion unter anderem, OTA/RTC/Audio und projektspezifische Hardware noch nicht automatisch einzubauen. Diese Punkte wurden später durch ausdrückliche Benutzeraufträge erweitert. Deshalb sind heute vorhanden:

- OTA
- DS3231
- SH1106
- DY-SV17F
- DI/DO-Hardwarebasis
- zentrale Zeitverwaltung

Sie sind weiterhin **generische Infrastruktur** und enthalten keine Unterbrechungszähler- oder konkrete Anwendungslogik.

## Firmware-Verantwortungsgrenzen

### SerialLog

Einziger allgemeiner Serial-Ausgabedienst. Module loggen über diese API; keine zweite Logging-Infrastruktur.

### WifiModule

Besitzt WLAN, NVS-Credentials, STA/AP-Zustand und 15-s-Seriellstatus. Web/UI greifen nur auf die öffentliche Zustands-API zu.

### TimeService

Einzige Instanz für Zeitquellenpriorität. NTP/RTC/Browser entscheiden nicht selbst über die globale Referenz.

### HardwareRegistry

Besitzt Health-/Probe-Orchestrierung, nicht die konkreten Busprotokolle. Konkrete Fähigkeiten bleiben in GPIO/RTC/Display/Audio.

### StatusRegistry

Kleine feste Providerliste für Header/API. UI kennt dadurch keine Pin-/Busdetails.

### API/WebServer

API serialisiert Status; Hardwaremodule erzeugen kein HTML.

## Laufzeit- und Blockierungsanalyse

### Normaler Loop

```text
HardwareRegistry::update()
WifiModule::update()
handleWebServer()
TimeService::update()
OtaModule::update()
delay(2)  // scheduler yield
```

Eigene wiederkehrende Logik:

- GPIO-DI Scan: 5 ms, aktuell 4 Eingänge, nur `digitalRead` + Debouncezustand
- WLAN-Statecheck: 500 ms, kleine Statusprüfung
- WLAN-Serialstatus: 15 s
- Audio: nur Parser/Deadlines bei pending Kommandos
- TimeService: nach Bootprüfung praktisch idle
- OTA: nur pending Restart bzw. aktiver Upload

Keine Basisfunktion besitzt eine sekündliche ESP32-Web-/Hardwarepollschleife.

### Absichtlich synchrone/gebundene Wege

| Stelle | Grenze / Häufigkeit | Bewertung |
|---|---|---|
| `SerialLog::begin()` | 50 ms einmal Boot | unkritisch |
| I2C `Wire` | 50-ms Bus-Timeout | Boot/manuell/echte RTC-/Displayaktion; akzeptiert |
| NTP UDP Antwort | max. 1500 ms | nur Boot oder explizite Zeitprüfung |
| NTP DNS `hostByName()` | Core-/DNS-abhängig | bekannte verbleibende synchrone Stelle |
| OTA Upload/Flash | vom Upload abhängig | ausdrücklich gestartete Wartungsoperation |
| OTA Restart | 20 ms vor `ESP.restart()` | unkritisch, System beendet sich danach |
| eingebauter `WebServer` | Coreabhängig | siehe Upstream-Grenze unten |

### Relevante Upstream-Grenze

Arduino-ESP32 3.3.11 hat ein dokumentiertes Problem des synchronen `WebServer`, bei dem ein langsam/unvollständig gesendeter HTTP-Header `handleClient()` lange im Parser halten kann. Die Korrektur wurde danach upstream gemergt. Die Basis wechselt bewusst nicht auf ein externes Async-Webserver-Framework, weil die Grundregeln Standardkomponenten und geringe Abhängigkeit priorisieren.

Konsequenz: Die Basis ist kooperativ aufgebaut, aber **keine harte Echtzeitgarantie unter fehlerhaften/absichtlich langsamen HTTP-Clients**. Für Feldbetrieb einen stabilen Core mit dem Upstream-Fix verwenden, sobald veröffentlicht.

### DI und echte Ereigniserfassung

Die generische DI-Schicht ist ein entprellter Zustandsleser. Sie ist nicht als Hochgeschwindigkeits-Pulszähler gedacht. Kurze Impulse oder harte Timinganforderungen müssen im eigentlichen Projekt über Interrupt/Hardwarecounter erfasst werden; ISR nur markieren/zählen, Verarbeitung und Logging im normalen Kontext.

Das ist eine bewusste Trennung, keine versteckte Echtzeitbehauptung.

## Speicher-/Flashbewertung

Web-UI wird gzip-komprimiert in PROGMEM gehalten. Im finalen Bundle sind es **129.678 Byte unkomprimiert / 30.370 Byte gzip**. Das OLED benötigt exakt 1024 Byte statischen Framebuffer. StatusRegistry besitzt eine zentral konfigurierte feste Kapazität von 16 Providern und die GPIO-Callbackliste eine feste Kapazität von 4; diese Registries benötigen keine dynamischen Container. Die Gesamtbasis ist bewusst **nicht** als vollständig heapfrei deklariert: Arduino `String`, `WebServer`, WiFi und Core-Komponenten verwenden Heap. Große dynamische Objektgraphen/Frontendframeworks existieren jedoch nicht.

API-Antworten werden als lokale `String`-Puffer mit vorab reservierter typischer Größe erzeugt. Da diese Endpunkte selten und kurzlebig sind, ist das gegenüber einer zusätzlichen JSON-Library oder komplexem Streamingcode die bewusst einfachere Embedded-Lösung.

Die **echte finale Firmware-/Heapgröße** kann nur der reale Arduino-ESP32-Link auf dem Zielboard bestimmen. Die OTA-Karte zeigt diese Werte auf dem Gerät. Dieser Audit erfindet keine Binärgröße.

## Frontend-Audit

- kein `innerHTML`, `outerHTML`, `insertAdjacentHTML`, `document.write`
- dynamische Gerätedaten über `textContent`
- Event Delegation für wiederholte Aktionen/Controls
- nur System-Theme-Listener und lokaler NTP-Input als direkte Speziallistener
- genau ein permanentes `setInterval`: Uhr/Uptime im Browser
- bounded `setTimeout`s nur für Request-Abbruch, Hardware-Follow-up und OTA-Reconnect
- keine unsichtbaren Tabs mit periodischem Fetch
- keine externen Fonts/Bilder/Skripte
- keine Router-/PWA-Schicht

## NVS/Flash-Schreibzyklen

- WLAN-Credentials nur schreiben, wenn geändert
- NTP-Server nur schreiben, wenn nach erfolgreicher Prüfung geändert
- Sprache/Theme nur im Browser-localStorage
- OTA beschreibt App-Partition bewusst

Damit erzeugt normales UI-Bedienen keine laufenden ESP32-NVS-Schreibzyklen.

## Sicherheitsgrenze

Aktuell absichtlich Entwicklungsbasis:

- passwortgeschützter Fallback-AP
- OTA ohne Authentifizierung
- kein Login

Das entspricht der beauftragten Basis, darf aber nicht als fertiges Produkt-Sicherheitsmodell betrachtet werden.

## Finaler Erweiterungspunkt

Die eigentliche Anwendung kann jetzt auf diese Dienste setzen. Neue Projektmodule sollen nicht die Basisdienste kopieren, sondern nur ihre Fachlogik hinzufügen.
