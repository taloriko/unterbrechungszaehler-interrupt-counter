# ADR – 433-MHz-Multi-Source-Erfassung

Status: **Entwurf / Diskussionsgrundlage**  
Basis: aktueller `main`-Stand 3.2.0  
Ziel dieses PRs: **nur Architektur und Umsetzungsplan, keine Firmwarefunktion, kein Versionssprung, kein Release**

## 1. Ausgangslage im aktuellen Stand

Der Unterbrechungszähler hat bereits die richtige Grundarchitektur für mehrere Erfassungsquellen: GPIO und Webbutton laufen zentral in `InterruptionService`, der Raw-Ring bleibt Source of Truth, Aggregate sind abgeleitet und der Capture-Pfad bleibt klein und nicht blockierend.

Aktuell speichert jeder Raw-Record 9 Byte:

- 4 Byte Zeitwert
- 17 Bit Delta
- 3 Bit `TimeSource`
- 3 Bit `EventSource`
- 1 Bit `absoluteValid`
- 1 Byte Sequenz-Tag
- 1 Byte CRC8

Damit stehen heute nur **3 Bit für `EventSource` = 8 numerische Werte** zur Verfügung. `0` ist `Unknown`; im Enum sind bereits `PhysicalButton`, `WebButton`, `Software`, `Api` und `Hardware` definiert. Im aktuellen Capture-Pfad werden praktisch nur lokaler physischer Taster und Webbutton verwendet.

Für das Zielbild mit ungefähr **10 Funkbuttons + Master + mindestens einer technischen Quelle** reichen 3 Bit nicht aus. Selbst wenn das bestehende Feld vollständig als logische Source-ID neu interpretiert würde, blieben nur 7 nutzbare IDs neben `0=Unknown`.

Der bestehende Daily-Aggregate-Record ist 64 Byte groß und bereits vollständig belegt: Tagesgesamtwert, 24 Stundenwerte, Sequenzcheckpoint, Header und CRC. Er enthält bewusst keine Source-Dimension.

## 2. Entscheidungsvorschlag

### 2.1 Raw-Record bleibt 9 Byte

Der wichtigste Vorschlag ist, den Raw-Record **nicht auf 10 Byte zu vergrößern**.

Stattdessen wird die 24-Bit-Packfläche neu codiert:

```text
Bit  0..16   deltaSeconds            17 Bit
Bit 17..18   persistedTimeSource      2 Bit
Bit 19..23   sourceId                 5 Bit
```

`absoluteValid` wird nicht mehr als eigenes Persistenzbit benötigt. Für tatsächlich gespeicherte Events gibt es heute genau vier relevante Zeitfälle:

- Relative / keine absolute Zeit
- NTP
- RTC
- Browser

Diese vier Zustände passen in 2 Bit. `absoluteValid` lässt sich beim Dekodieren eindeutig aus dem gespeicherten Zeitcode ableiten: nur `Relative` ist nicht absolut gültig. `TimeTypes::Source::None` bleibt ein RAM-/Initialzustand und wird nicht als Raw-Event-Quelle persistiert.

Damit entstehen **5 Bit Source-ID = 32 stabile IDs**, ohne einen einzigen zusätzlichen Byte pro Ereignis.

### 2.2 Source-ID-Aufteilung

Die numerischen Altwerte werden absichtlich erhalten:

| Source-ID | Bedeutung |
|---:|---|
| 0 | Unknown / Legacy-unbekannt |
| 1 | Master-Taster lokal |
| 2 | Webbutton |
| 3 | Software / interne technische Quelle |
| 4 | API |
| 5 | Hardware / technische Reserve |
| 6..31 | logische Funk-/Teamquellen |

Damit stehen **26 logische Funk-IDs** zur Verfügung. Das bedeutet ausdrücklich **nicht**, dass 26 Funkbuttons gleichzeitig unterstützt werden müssen.

Für die erste Team-Version wird ein **festes Limit von 12 gleichzeitig angelernten Funkbuttons** vorgeschlagen. Das deckt die gewünschte Zielgröße von 10 Buttons ab und lässt zwei aktive Reserven. Die größere 5-Bit-ID-Menge ist nützlich, weil alte Source-IDs nicht sofort recycelt werden müssen, wenn Personen oder Taster später ausgetauscht werden.

## 3. Warum 9 Byte klar vorzuziehen sind

Aktuelles Speicherbudget:

```text
100.000 Raw-Events × 9 B   = 900.000 B
2.300 Daily-Records × 64 B = 147.200 B
Metadaten                  =     168 B
---------------------------------------
Nutzdaten                  = 1.047.368 B
LittleFS-Partition         = 1.245.184 B
Reserve vor FS-Overhead    ≈   197.816 B
```

Vergleich:

| Variante | Raw-Speicher | Folge |
|---|---:|---|
| heute / vorgeschlagen 9 B | 900.000 B | 100.000 Events bleiben erhalten |
| 10-Byte-Record | 1.000.000 B | +100 kB, Reserve fast halbiert |
| 10 Byte bei gleichem Raw-Budget | 900.000 B | nur noch 90.000 Events |
| separates 4-Bit-Sidecar | +50.000 B | zweiter transaktional gekoppelter Ring nötig |
| 1-Byte-Source-Sidecar | +100.000 B | ähnliche Kosten wie 10-Byte-Record |

Ein Sidecar spart auf dem Papier Bytes, verschlechtert aber die bewährte Transaktionslogik: Raw-Record, Source-Datei und Metadaten müssten gemeinsam geschrieben, zurückgerollt und recovered werden. Das ist für 4–5 zusätzliche Bits unverhältnismäßig.

**Empfehlung: 9-Byte-Record beibehalten und nur den Codec versionieren.**

## 4. Source-ID ist logisch, Funkcode ist nur Bindung

Die persistierte `sourceId` beschreibt die **logische Quelle**, nicht den physischen 433-MHz-Sender.

Beispiel:

```text
Source-ID 8 = "Anna"
RF-Code A  -> Source-ID 8
Taster geht kaputt
RF-Code B  -> Source-ID 8
```

Alle historischen Events bleiben Source-ID 8. Es wird kein Raw-Event geändert.

Ein Quellenname wird ebenfalls **nicht pro Event** gespeichert. Namen und Funkbindungen liegen separat in einer kleinen Source-Konfiguration.

Wichtige Regel:

- **Taster ersetzen:** neue RF-Bindung auf bestehende Source-ID
- **Name ändern:** nur Source-Konfiguration ändern
- **Quelle stilllegen:** Source-ID und Name für Historie behalten, RF-Bindung deaktivieren
- **wirklich neue Person / neue logische Quelle:** neue Source-ID vergeben
- Source-IDs werden **nicht automatisch recycelt**

Dadurch bleiben historische Zuordnungen stabil, ohne Namen oder RF-Codes in 100.000 Raw-Records zu duplizieren.

## 5. Schlanke SourceRegistry

Vorgeschlagen wird eine kleine `SourceRegistry` in der Projektschicht.

### 5.1 Feste Quellen

IDs 0..5 sind im Code definiert. Dafür ist keine dynamische Konfiguration nötig.

### 5.2 Funkquellen

Für IDs 6..31 werden Namen separat gehalten. Gleichzeitig sind maximal 12 RF-Bindungen aktiv.

Eine Funkbindung braucht nur ungefähr:

```text
sourceId     1 B
protocol     1 B
bitLength    1 B
flags        1 B
rfCode       4 B
----------------
             8 B
```

12 aktive Bindungen benötigen damit nur etwa **96 Byte** Nutzdaten.

Namen können als kleine feste UTF-8-Felder gespeichert werden, z. B. maximal 20 Byte pro logischer Funk-ID. Selbst für alle 26 Funk-IDs bleibt die gesamte Source-Konfiguration im Bereich deutlich unter 1 kB Nutzdaten.

### 5.3 Persistenz

Keine JSON-Datei und keine Event-Namensduplikate.

Empfohlen:

- versionierter binärer Blob in eigenem NVS-Namespace
- zwei alternierende Slots
- Commitcounter
- CRC32
- beim Boot neuesten gültigen Slot wählen

Änderungen an Namen/Pairings sind selten. Ein kleiner vollständiger Blob ist deshalb einfacher und robuster als viele unabhängig geschriebene NVS-Keys.

## 6. Trennung von Source und Transport

`EventSource` vermischt heute logische Quelle und Capture-Art. Für die Team-Version sollte das getrennt werden.

Persistiert wird nur:

```text
SourceId
```

Nicht persistent nötig sind Dinge wie:

```text
CaptureOrigin::LocalButton
CaptureOrigin::Radio
CaptureOrigin::Web
CaptureOrigin::Api
```

Die Transportart ist nur für Laufzeitverhalten interessant, z. B. ob Feedback sofort vorgezogen wird. Sie muss nicht 100.000-mal im Flash stehen, wenn die logische Source-ID die relevante historische Information ist.

Der Capture-Pfad bleibt sinngemäß:

```text
Master GPIO ---- Source 1 --┐
Web ---------- Source 2 ----┤
433 MHz ------ Source 6..31 ┤
                            ▼
                   InterruptionService
                            ▼
                       Raw Ring
```

Namen, Funkcode und Protokoll bleiben außerhalb des Raw-Rings.

## 7. Rückwärtskompatibilität ohne 900-kB-Umschreiben

Eine komplette In-Place-Konvertierung von bis zu 100.000 Records wäre technisch möglich, aber unnötig riskant: viel Flash-I/O, lange Laufzeit und zusätzlicher Migrationsjournal-Code.

Besser ist eine **Dual-Codec-Migration über eine Sequenzgrenze**.

### 7.1 Prinzip

Beim ersten Start der neuen Firmware mit einem gültigen Raw-v2-Ring:

1. vorhandene Raw-v2-Metadaten lesen
2. `encodingSwitchSequence = oldTotalSequence + 1` festhalten
3. Raw-Metadaten auf v3 erweitern, Ringposition/Kapazität unverändert übernehmen
4. bestehende Records bleiben bytegenau v2
5. alle neuen Records ab `encodingSwitchSequence` werden v3 geschrieben

Beim Lesen entscheidet die logische Sequenz:

```text
sequence < encodingSwitchSequence  -> decodeV2()
sequence >= encodingSwitchSequence -> decodeV3()
```

Die numerischen Alt-`EventSource`-Werte 0..5 werden beim v2-Decode direkt auf die gleich nummerierten Source-IDs 0..5 abgebildet.

Damit gibt es:

- keinen 900-kB-Rewrite
- keinen Kapazitätsverlust
- keine zweite Raw-Datei
- keinen langen blockierenden Upgradevorgang

Nach spätestens 100.000 neuen Events ist der letzte alte v2-Record natürlich aus dem Ring gefallen. Ab dann besteht der Ring vollständig aus v3.

### 7.2 Recovery während der Übergangszeit

Die Sequenzgrenze ist während einer gemischten v2/v3-Phase für korrektes Dekodieren wichtig. Sie sollte deshalb redundant gespeichert werden:

- in beiden neuen Raw-Metadaten-Slots
- zusätzlich in einem winzigen versionierten/CRC-geschützten Formatmarker im NVS

Der bestehende CRC8 und der 8-Bit-Sequenztag jedes Raw-Records bleiben unverändert.

Wenn während der Übergangsphase beide Raw-Metadaten **und** der redundante Formatmarker gleichzeitig verloren sind, darf Recovery nicht raten. In diesem extremen Fall gilt weiter das bestehende Prinzip: **fail safe statt falsche historische Zuordnung**.

Sobald keine v2-Records mehr im Ring liegen, ist die zusätzliche Sequenzgrenze nicht mehr notwendig.

### 7.3 Versionierung

Technisch ist das ein neues internes Raw-Encoding. Wenn die Dual-Codec-Migration inklusive Recovery durch Tests nachweisbar sicher ist, kann die Änderung ohne Datenverlust erfolgen.

Falls sich diese Übergangslogik in Tests als unnötig komplex oder nicht eindeutig recoverbar herausstellt, ist ein **Major-Release mit sauberem Formatbruch** besser als ein halbzuverlässiger Konverter. Datenintegrität hat Vorrang vor einer kosmetisch kleineren Versionsnummer.

## 8. Daily Aggregates zunächst unverändert lassen

Der vorhandene Daily-Record bleibt bei **64 Byte** und enthält weiterhin nur Gesamtwerte.

Eine Source-Dimension direkt dort einzubauen wäre teuer. Bereits 16 zusätzliche `uint16`-Source-Zähler würden pro Tag 32 Byte hinzufügen:

```text
2.300 Tage × 32 B = 73.600 B zusätzlich
```

Stündliche Source-Zähler würden den Record noch wesentlich stärker aufblasen.

Für die erste Team-Version deshalb:

- Gesamt-Heatmaps bleiben exakt auf dem bestehenden Daily-Aggregate-System
- Source-spezifische Auswertungen werden **nur auf Anforderung** aus dem retained Raw-Ring berechnet
- der Raw-Ring hat die Source-ID bereits pro Event
- Scans laufen kooperativ wie die bestehende Ø-Abstand-Auswertung
- Coverage wird sichtbar gemacht, wenn der gewählte Zeitraum älter als die retained 100.000 Raw-Events ist

Ein festes Arbeitsarray für alle Source-IDs kostet nur:

```text
32 × uint32_t = 128 Byte RAM
```

Das ist deutlich günstiger als dauerhaft zehntausende zusätzliche Aggregatebytes zu persistieren.

Erst wenn echte Nutzung zeigt, dass langfristige Source-Statistiken **über die Raw-Retention hinaus** benötigt werden, sollte ein separater, abgeleiteter Source-Aggregatstore entworfen werden. Der bestehende 64-Byte-Daily-Store bleibt dann unangetastet.

## 9. Bedeutung von `deltaSeconds`

`deltaSeconds` bleibt weiterhin der Abstand zum unmittelbar vorherigen **globalen** Event desselben lokalen Tages.

Es wird **kein zweiter per-Source-Delta-Wert** in den Raw-Record aufgenommen.

Falls später ein „durchschnittlicher Abstand pro Person“ gewünscht wird, wird dieser bei der Source-Auswertung aus den Zeitstempeln aufeinanderfolgender Events derselben Source-ID abgeleitet. Damit bleibt das persistente Format klein und die bestehende globale Ø-Abstand-Logik unverändert.

## 10. CC1101 / RF1100SE – Hardwarearchitektur

Der CC1101 wird als eigenes generisches Hardwaremodul integriert und kennt den Begriff „Unterbrechung“ nicht.

Vorgeschlagene freie GPIOs auf dem aktuellen ESP32-Profil:

| CC1101 | ESP32 | Begründung |
|---|---:|---|
| SCK | GPIO14 | DI2 ist deaktiviert |
| MISO | GPIO32 | DI3 ist deaktiviert |
| MOSI | GPIO23 | DO4 ist deaktiviert |
| CS/SS | GPIO25 | DO1 ist deaktiviert |
| GDO0 | GPIO26 | DO2 ist deaktiviert, gut für Edge/IRQ |
| GDO2 | zunächst nicht benötigt | GPIO27/33 bleiben als Reserve |

Bestehende Belegung bleibt damit unberührt:

- Master-Taster GPIO13
- I2C GPIO21/22
- DY-SV17F UART GPIO18/19
- DY-SV17F BUSY GPIO39

SPI wird explizit auf diese Pins gelegt; die deaktivierten generischen GPIO-Kanäle dürfen dafür nicht parallel aktiviert werden.

## 11. RF-Protokoll bewusst klein halten

„433 MHz“ allein garantiert keine Protokollkompatibilität. Günstige Taster können unterschiedliche OOK/ASK-/Fixed-Code-Verfahren verwenden.

Für die erste Version deshalb kein universeller SDR-Decoder und keine großen Rohpuls-Logs.

Vorgeschlagen:

1. CC1101 auf einen getesteten 433,92-MHz-OOK/ASK-Empfangsmodus konfigurieren
2. zunächst eine kleine, nachweislich kompatible Fixed-Code-Familie unterstützen
3. RF-Frame in eine kompakte Identität normalisieren (`protocol`, `bitLength`, `rfCode`)
4. nur diese Identität an die Projektschicht weitergeben

Der genaue unterstützte Button-/Protokolltyp wird vor der Implementierung mit realer Hardware festgelegt und dokumentiert. „Beliebiger 433-MHz-Taster“ ist ausdrücklich kein Ziel der ersten Version.

## 12. Laufzeitverhalten und Entprellung

Funkbuttons senden einen Tastendruck typischerweise mehrfach. Mehrere identische RF-Frames dürfen nicht als mehrere Unterbrechungen gezählt werden.

Vorgeschlagen:

- ISR macht nur minimale Edge-/Frame-Arbeit
- keine Dateisystem-, JSON-, Web- oder Statistikarbeit im ISR
- kleine feste Decoderzustände, keine Heap-Allokation pro Frame
- ein Frame gilt erst nach plausibler Wiederholung/Validierung als gelernt bzw. gedrückt
- kurzer per-Source-Dedupe-Zeitraum gegen Sendewiederholungen
- danach normaler `InterruptionService::capture(sourceId)`-Pfad

Der Dedupe-Zustand kann als kleines festes Array geführt werden. Selbst 12 `uint32_t`-Zeitstempel sind nur 48 Byte RAM.

## 13. Anlernmodell

Anlernen ist eine seltene Konfigurationsoperation und darf daher etwas mehr UI-Komfort haben, ohne den normalen Eventpfad aufzublähen.

Vorgeschlagener Ablauf:

1. Benutzer wählt „Funkbutton anlernen“
2. entweder neue logische Source-ID anlegen oder bestehende Source-ID zum Tasterersatz auswählen
3. Master öffnet ein zeitlich begrenztes Learn-Fenster
4. nächster mehrfach bestätigter unbekannter RF-Code wird übernommen
5. Name wird separat gespeichert
6. während Learn-Modus wird dieses RF-Signal **nicht** als Unterbrechung gezählt
7. Speichern der SourceRegistry erfolgt transaktional

Keine RF-Codes werden als Textlogs oder JSON-Historie persistiert.

## 14. Web/API später

Die Weboberfläche soll nur die kleinen Konfigurations- und Ergebnisdaten laden, die gerade benötigt werden.

Spätere Endpunkte können z. B. liefern:

- Liste der Source-IDs mit Name, Typ und Aktivstatus
- aktive RF-Bindungen ohne unnötige Rohpulsdaten
- Learn-Status
- Rename / Rebind / Disable
- Source-Filter für Auswertung

Gesamt-Home bleibt klein. Die Source-Liste muss nicht sekündlich gepollt werden.

CSV kann später zusätzlich mindestens `source_id` und den beim Export aufgelösten aktuellen `source_name` enthalten. Der Name bleibt dabei reine Exportdarstellung und wird weiterhin nicht pro Event gespeichert.

## 15. Sinnvolle maximale Buttonanzahl

Es sind drei verschiedene Grenzen zu unterscheiden:

### Persistente Source-ID-Grenze

5 Bit = **32 IDs insgesamt**, davon 26 für Funk-/Teamquellen nach den reservierten technischen IDs 0..5.

### Gleichzeitig angelernte Funkbuttons

Für die erste Version bewusst **12**.

Das ist keine Speichergrenze, sondern eine Produkt-/Komplexitätsgrenze. 10 Teambuttons plus 2 Reserve sind ausreichend; mehr aktive Sender erhöhen UI-, Test- und RF-Kollisionsaufwand, ohne den Kernnutzen zu verbessern.

### Historische Quellen

Da logische IDs nicht automatisch recycelt werden, können über die Lebensdauer mehr unterschiedliche Personen/Tasterhistorien sauber getrennt bleiben als gleichzeitig aktiv sind.

## 16. Umsetzungsplan

### Phase A – Raw-Codec und Source-ID

- `SourceId` als kleinen numerischen Typ einführen
- persistierte Source von Capture-Transport trennen
- Raw-v3-Codec mit 17/2/5-Bit-Packing
- v2-Decoder erhalten
- Dual-Codec-Sequenzgrenze und redundanten Formatmarker implementieren
- bestehende 100.000er Kapazität unverändert lassen
- CSV auf `source_id` vorbereiten

**Noch kein RF.** Zuerst muss Datenintegrität vollständig bewiesen sein.

### Phase B – SourceRegistry

- feste IDs 0..5
- Funk-ID-Pool 6..31
- maximal 12 aktive Bindungen
- kompakter binärer Double-Slot-NVS-Store mit CRC
- Create/Rename/Rebind/Disable
- keine automatische ID-Wiederverwendung

### Phase C – CC1101-Treiber

- SPI-Pins aus zentralem `hardware_config.h`
- Hardwarestatus in vorhandene Registry integrieren
- kooperatives `update()`
- kleiner RX-/Decoderzustand
- kein dynamisches Eventlogging

### Phase D – RF-Decoder und Pairing

- zunächst genau eine getestete Fixed-Code-Protokollfamilie
- stabile Fingerprintbildung
- Wiederholungsfilter
- Learn-Modus
- Mapping RF-Fingerprint -> Source-ID
- Master-Taster bleibt unverändert aktiv

### Phase E – UI

- neue Quellen-/Funkbutton-Karte unter Einstellungen
- Name, Aktivstatus, Anlernen, Ersetzen
- keine globale Save-Logik
- vorhandenes responsive/i18n-System weiterverwenden

### Phase F – Source-Auswertung

- Gesamtstatistik unverändert über Daily Aggregates
- Source-Filter nur bei Bedarf
- Source-spezifische Counts aus einem kooperativen Raw-Scan
- Coverage anzeigen
- erst nach Messung über zusätzlichen persistenten Source-Aggregatstore entscheiden

## 17. Test- und Release-Gates für die spätere Implementierung

Vor einem Release müssen zusätzlich zu den bestehenden Checks mindestens folgende Szenarien grün sein:

### Datenformat

- v2-only Raw-Ring vollständig lesbar
- leerer v2-Ring -> v3
- gemischter v2/v3-Ring an Sequenzgrenze
- Ring-Wrap während der Übergangszeit
- Dual-Meta-Rollback unverändert korrekt
- Metadaten-Recovery mit vorhandenem Formatmarker
- verlorener Formatmarker wird niemals durch Raten ersetzt
- Daily-Rebuild aus gemischtem v2/v3-Raw-Ring
- 100.000 Raw-Slots bleiben erhalten

### SourceRegistry

- CRC-/Slot-Fallback
- Stromverlust zwischen Registry-Slotwrites
- Rename ändert keine Raw-Events
- Rebind erhält Source-ID
- Disable erhält historische Source-ID
- keine automatische Wiederverwendung alter IDs

### Funk

- ein Tastendruck mit mehreren RF-Wiederholungen zählt exakt einmal
- zwei echte nacheinander gedrückte Tasten zählen zweimal
- unbekannter Sender wird außerhalb Learn-Modus ignoriert
- Learn-Modus zählt das Lerntelegramm nicht als Unterbrechung
- lokaler Master-Taster funktioniert auch bei RF-Störung weiter
- RF-Ausfall beeinflusst Storage/Web/Display/Audio nicht

### Ressourcen

- ESP32-Core-Compile grün
- bestehendes Flash-Reserve-Gate bleibt grün
- RAM-Mehrverbrauch dokumentiert
- kein Heap-Wachstum pro RF-Event
- kein permanentes RF-JSON-/Textlog

## 18. Fazit

Die Zielgröße von ungefähr **10 Funkbuttons plus Master** ist mit dem bestehenden ESP32 und der aktuellen Speicherarchitektur sehr gut machbar, ohne die 100.000 Raw-Events zu reduzieren.

Die schlankste Lösung ist:

- CC1101 als zusätzliches Hardwaremodul
- stabile numerische `SourceId`
- Namen und RF-Bindungen nur einmal in einer kleinen transaktionalen SourceRegistry
- **Raw-Record weiterhin 9 Byte**
- 5-Bit-Source-ID durch kompakteres Zeitquellen-Encoding
- maximal 12 aktive Funkbuttons in der ersten Team-Version
- 26 historische/logische Funk-IDs als Reserve ohne zusätzliche Eventbytes
- Daily Aggregates zunächst unverändert
- Source-Auswertungen nur on demand aus dem Raw-Ring
- v2/v3-Kompatibilität über Sequenzgrenze statt massenhaftem Flash-Rewrite

Damit bleiben Effizienz, Datenintegrität, Recovery-Verhalten und die bestehende Architektur wichtiger als maximale Buttonanzahl oder Komfortfunktionen.