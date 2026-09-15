# Persistentes Datenformat – Unterbrechungszähler 3.6.0

Die persistenten Projektdaten liegen in der eigenen LittleFS-Partition sowie – für den kleinen aktuellen Arbeitszykluszustand – in NVS. Der bestehende Raw-Ring bleibt die **Source of Truth für bestätigte Unterbrechungen**. Tagesaggregate sind ausschließlich abgeleitete Statistikdaten.

## Grundregel ab 3.6.0

Arbeitsbeginn und Arbeitsende sind **keine Unterbrechungen** und werden deshalb nicht in den bestehenden Unterbrechungs-Ring geschrieben. `WorkCycle` hält den jeweils letzten physischen Kurzdruck zunächst als Kandidaten zurück. Erst wenn ein weiterer gültiger Kurzdruck folgt, wird der vorherige Kandidat mit seinem ursprünglichen Zeitstempel über `InterruptionService::captureAtEpoch()` als echte Unterbrechung übernommen.

Dadurch bleiben Raw-Ring, Tagesaggregate, Heatmaps, Fokus-Auswertungen und CSV semantisch sauber: Sie enthalten weiterhin ausschließlich Unterbrechungen.

## Raw Ring

Datei: `/interrupt.raw`

Kapazität: **100.000 Records × 9 Byte = 900.000 Byte**.

Das 3.5.x-Format bleibt in 3.6.0 bytekompatibel und wird nicht erweitert.

| Byte | Inhalt |
|---:|---|
| 0–3 | `timeValueSeconds` little-endian (`epoch` oder relative Uptime) |
| 4–6 | 24 Bit gepackt: 17 Bit Delta, 3 Bit TimeSource, 3 Bit EventSource, 1 Bit absolute-valid |
| 7 | Low-Byte der logischen Sequenz |
| 8 | CRC8 über Byte 0–7 |

Delta-Sonderwerte:

- `0..131069`: Abstand in Sekunden
- `131070`: unbekannt
- `131071`: erstes bestätigtes Unterbrechungsereignis des lokalen Tages

Die vollständige Sequenz wird über Ringmetadaten und Ringposition rekonstruiert. Das Low-Byte im Record dient zusätzlich der Konsistenz-/Recoveryprüfung.

## Raw-Metadaten – Format v2

Datei: `/interrupt.meta`

Zwei alternierende **44-Byte-Slots** speichern jeweils Magic/Version, Recordgröße, Kapazität, `writeIndex`, `count`, `totalSequence`, den letzten gültigen Kalenderanker, Commitcounter und CRC32.

Append bleibt transaktional: Erst Raw-Record schreiben und flushen, danach Metadaten committen. Bei einem Metadatenfehler wird der RAM-Stand zurückgerollt; bei vollem Ring wird zusätzlich der verdrängte 9-Byte-Record restauriert. Orphan-Recovery und kooperative Recovery bleiben unverändert.

## Arbeitszyklus-Zustand in NVS

Namespace: `interruptcyc`

Key: `state`

Gespeichert wird nur der kleine Zustand, der einen Neustart überleben muss:

- Magic/Formatkennung
- lokaler `dayIndex`
- `active`
- `pending`
- UTC-Epoch des Zyklusstarts
- UTC-Epoch des letzten noch nicht klassifizierten Kurzdrucks

Der Zustand wird nur bei Start, neuem Kandidaten und Zyklusende geändert. Es gibt keinen periodischen Schreibvorgang.

### Klassifikation

```text
1. kurzer Druck bei inaktiv
   -> START in Zyklusjournal
   -> kein Raw-Event

2. erster kurzer Druck bei aktiv
   -> nur als pending in NVS

3. weiterer kurzer Druck
   -> vorheriges pending wird Raw-Unterbrechung
   -> neuer Druck wird pending

4a. langer Druck >= 2 s
   -> vorhandenes pending wird Unterbrechung
   -> langer Druck = END

4b. lokaler Tageswechsel ohne langen Druck
   -> vorhandenes pending = END
   -> kein Raw-Event für diesen letzten Druck
```

## Zyklusjournal

Datei: `/cycles.log`

START und END werden zusätzlich als kleine **8-Byte-Records** protokolliert. Sie sind Ergänzungsdaten und verändern weder den Raw-Ring noch die Tagesaggregate.

| Byte | Inhalt |
|---:|---|
| 0–3 | UTC-Epoch Sekunden |
| 4–5 | lokaler `dayIndex` |
| 6 | Typ: `1 = START`, `2 = END` |
| 7 | Grund: `0 = Start`, `1 = manuelles Ende`, `2 = automatischer Tageswechsel` |

Das Journal ist absichtlich klein und append-only. Der für die laufende Logik notwendige Zustand liegt unabhängig davon in NVS, damit ein fehlgeschlagener Journalzugriff keinen laufenden Zyklus zerstört.

## Daily Aggregate Ring

Datei: `/daily.bin`

Kapazität: **2.300 Slots × 64 Byte = 147.200 Byte** (> 6,2 Jahre).

Ein Tagesrecord enthält lokalen `dayIndex`, Format-/Validflags, Tagesgesamtzahl, CRC16, letzte bereits eingerechnete Raw-Sequenz und 24 × `uint16` Stundenwerte.

Nur bestätigte Unterbrechungen gelangen in diese Aggregate. START, END und der noch offene Kandidat eines Zyklus werden nie mitgezählt.

## Aggregate-Metadaten

Datei: `/daily.meta`

Zwei alternierende **40-Byte-Slots** mit CRC32 speichern `writeIndex`, `count`, letzte verarbeitete Raw-Sequenz, Zahl nicht zuordenbarer Unterbrechungen und Commitcounter. Rebuilds bleiben vollständig aus dem Raw-Ring möglich.

## Speicherbudget

Custom LittleFS: **1.245.184 Byte**

```text
Raw maximal        900.000 B
Daily maximal      147.200 B
Raw Meta                 88 B
Daily Meta                80 B
--------------------------------
Basisdaten         1.047.368 B
```

`/cycles.log` kommt in 3.6.0 zusätzlich hinzu. Mit 8 Byte pro START/END sind das normalerweise nur 16 Byte pro Arbeitstag; selbst mehrere Jahre bleiben im Vergleich zur vorhandenen LittleFS-Reserve klein. Der aktuelle Zykluszustand liegt in NVS und benötigt keinen LittleFS-Slot.

## CSV

CSV bleibt ein abgeleitetes Exportformat. Es wird beim Download aus den bestätigten Raw-Unterbrechungen gestreamt. START/END erscheinen dort bewusst nicht und verändern daher keine bestehenden Import-/Auswertungsabläufe.

## Herkunftsfilter und Löschfunktion

Der im 9-Byte-RawEvent gespeicherte `eventSource` bleibt unverändert für Heatmap-Filter verfügbar. **Beides** kann weiterhin die kompakten Langzeit-Tagesaggregate nutzen; einzelne Herkunftsfilter werden aus dem retained Raw-Ring berechnet und melden unvollständige Abdeckung ehrlich.

Die bestehende manuelle Datenbank-Löschung für Raw- und Tagesdaten bleibt unverändert. Zyklusdaten sind von dieser Unterbrechungsdatenbank logisch getrennt; ein Release-/Wartungsschritt darf sie nur dann zusätzlich löschen, wenn dies ausdrücklich dokumentiert und bestätigt wird.
