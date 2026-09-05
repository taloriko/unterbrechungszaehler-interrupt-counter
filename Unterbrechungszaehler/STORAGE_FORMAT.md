# Persistentes Datenformat – Unterbrechungszähler 0.1.0

Die persistenten Projektdaten liegen in einer eigenen LittleFS-Partition und sind von den beiden OTA-App-Partitionen getrennt. Rohereignisse sind die **Source of Truth**; Tagesaggregate sind ausschließlich abgeleitete Statistikdaten.

## Raw Ring

Datei: `/interrupt.raw`

Kapazität: **100.000 Records × 9 Byte = 900.000 Byte**.

| Byte | Inhalt |
|---:|---|
| 0–3 | `timeValueSeconds` little-endian (`epoch` oder relative Uptime) |
| 4–6 | 24 Bit gepackt: 17 Bit Delta, 3 Bit TimeSource, 3 Bit EventSource, 1 Bit absolute-valid |
| 7 | Low-Byte der logischen Sequenz |
| 8 | CRC8 über Byte 0–7 |

Delta-Sonderwerte:

- `0..131069`: Abstand in Sekunden
- `131070`: unbekannt
- `131071`: erstes Event des lokalen Tages

Die vollständige Sequenz wird über die Ringmetadaten und Ringposition rekonstruiert. Das Low-Byte im Record dient zusätzlich der Konsistenz-/Recoveryprüfung.

## Raw-Metadaten – Format v2

Datei: `/interrupt.meta`

Zwei alternierende **44-Byte-Slots** speichern jeweils:

- Magic / Metadatenversion
- Recordgröße und Kapazität
- `writeIndex`
- `count`
- `totalSequence`
- letzten kalendarisch gültigen Eventanker (`epoch` + lokaler `dayIndex`)
- Commitcounter
- CRC32

Der persistierte Kalenderanker ist wichtig für den Abstand zum vorherigen Ereignis: Nach einem Neustart kann der nächste Event desselben lokalen Tages seinen Abstand in O(1) bestimmen, auch wenn die zuletzt gespeicherten Raw-Events nur relative Zeit hatten.

### Transaktionaler Append

1. Zielslot und nächste Sequenz werden aus den **dauerhaft bestätigten** Metadaten abgeleitet.
2. Der 9-Byte-Raw-Record wird geschrieben und geflusht.
3. Erst danach wird der nächste alternierende Metadatenslot committed.
4. Scheitert Schritt 3, werden die RAM-Metadaten vollständig auf den vorherigen Stand zurückgerollt.
5. Ist der 100.000er Ring bereits voll, wurden mit dem Zielslot gleichzeitig die ältesten 9 Byte verdrängt. Diese 9 Byte werden deshalb vor dem Überschreiben gesichert und bei einem Metadatenfehler ebenfalls zurückgeschrieben.
6. Ein Retry benutzt damit denselben Slot und dieselbe Sequenz – es entsteht kein zweites logisches Ereignis und der zurückgerollte Metastand verweist nicht auf einen bereits überschriebenen ältesten Record.

Ein bereits geschriebener, aber noch nicht committed Record ist ein Orphan. `recoverOrphan()` kann genau diesen nächsten Record anhand des Sequenz-Tags übernehmen.

### Recovery

Der normale Boot scannt den 100.000er Ring **nicht**.

Nur wenn beide Metadatenslots unbrauchbar sind, startet ein kooperativer Recoverylauf:

- Hauptscan in Batches von 128 Records
- anschließend ebenfalls kooperative Rückwärtssuche nach dem jüngsten kalendarisch gültigen Eventanker
- keine zweite große blockierende Vollschleife
- Web-/Hardwareloop kann zwischen den Batches weiterlaufen

Beim Verlust beider Raw-Metadaten kann aus den 8-Bit-Sequenztags die physische Reihenfolge, aber nicht jede frühere High-Byte-Epoche der lebenslangen Sequenz rekonstruiert werden. Ist ein gültiger Daily-Aggregate-Checkpoint vorhanden, dient dessen `lastProcessedSequence` als unabhängige dauerhafte Untergrenze: Die rekonstruierte Raw-Sequenz wird auf den ersten passenden 8-Bit-Tag **ab bzw. oberhalb dieses Checkpoints** angehoben. Dadurch werden neue Raw-Events nach Recovery nicht fälschlich als bereits aggregiert behandelt.

Ist der Raw-Ring in einem außergewöhnlichen Recoveryfall vollständig leer, der Daily-Store aber noch gültig, wird dessen Checkpoint trotzdem als logische Raw-Sequenzbasis übernommen (`count` bleibt 0). Der nächste neue Raw-Event erhält dadurch `checkpoint + 1` statt wieder bei Sequenz 1 zu beginnen. Die langfristigen Tagesaggregate können so erhalten bleiben, ohne dass neue Ereignisse wegen alter Sequenznummern übersprungen werden.

## Daily Aggregate Ring

Datei: `/daily.bin`

Kapazität: **2.300 Slots × 64 Byte = 147.200 Byte** (> 6,2 Jahre).

Ein Tagesrecord enthält:

- lokalen `dayIndex` seit 2020-01-01 in `Europe/Berlin`
- Format-/Validflags
- Tagesgesamtzahl (`uint16`)
- CRC16
- letzte bereits eingerechnete Raw-Sequenz
- 24 × `uint16` Stundenwerte

Die Heatmaps verwenden ausschließlich diese Tagesrecords. Normale Statistikabfragen müssen deshalb nicht durch 100.000 Rohereignisse laufen.

## Aggregate-Metadaten

Datei: `/daily.meta`

Zwei alternierende **40-Byte-Slots** mit CRC32 speichern:

- `writeIndex`
- `count`
- letzte verarbeitete Raw-Sequenz
- Zahl nicht kalendarisch zuordenbarer Events
- Commitcounter

`lastSequence` im Tagesrecord und `lastProcessedSequence` in den Metadaten machen Wiederholungen idempotent.

### Transaktionssicherheit bei vollem Tagesring

Wenn alle 2.300 Tagesslots belegt sind, muss für einen neuen Tag der älteste Slot überschrieben werden. Vor diesem Schreibvorgang wird der verdrängte Tagesrecord im kleinen RAM-Arbeitsobjekt gesichert. Scheitert danach der Metadatencommit, wird der verdrängte Record zurückgeschrieben und der Metastand zurückgerollt. Ein Retry kann damit denselben Ringübergang sauber erneut ausführen.

Kann ein solcher Rollback selbst nicht sicher abgeschlossen werden, wird **nicht** weiter geraten: Die Aggregate werden als reparaturbedürftig markiert und kooperativ aus dem Raw-Ring neu aufgebaut. Die Rohdaten bleiben dabei unangetastet.

## Aggregate-Rebuild

Daily Aggregates sind abgeleitet. Bei beschädigten/inkonsistenten Aggregatmetadaten:

- Raw-Ring bleibt unverändert
- Rebuild erfolgt kooperativ, maximal wenige Raw-Events je `update()`-Durchlauf
- danach wird der kleine Home-Summaryzustand aus dem reparierten Tagesrecord synchronisiert
- nicht kalendarisch zuordenbare Events werden separat gezählt

## Speicherbudget

Custom LittleFS: **1.245.184 Byte**

```text
Raw maximal        900.000 B
Daily maximal      147.200 B
Raw Meta                 88 B   (2 × 44)
Daily Meta                80 B   (2 × 40)
--------------------------------
Nutzdaten          1.047.368 B
Reserve ca.          197.816 B
```

Die ca. 198 kB Reserve müssen zusätzlich LittleFS-Verwaltungs-/Blockoverhead aufnehmen. Deshalb ist die Datenpartition nicht enger dimensioniert.

## CSV

CSV ist **kein** Primärformat. Beim Download werden die Raw-Records chronologisch – ältester noch vorhandener bis neuester – dekodiert und in kleinen HTTP-Chunks ausgegeben. Es entsteht weder eine zweite permanente CSV-Datei noch ein 100.000-Zeilen-String im RAM.

## 3.3.0 Herkunftsfilter und Löschfunktion

Der bereits im 9-Byte-RawEvent gespeicherte `eventSource` wird nun direkt für Heatmap-Filter verwendet. **Beides** kann weiterhin die kompakten Langzeit-Tagesaggregate nutzen. Ein einzelner Herkunftsfilter (z. B. GPIO oder Web) wird aus dem retained Raw-Ring berechnet; dadurch wird das bestehende Tagesformat nicht vergrößert. Ist der angefragte Zeitraum älter als die Rohdatenabdeckung, meldet die API `coverage.complete=false` statt fehlende Daten als Null zu erfinden.

Die manuelle Datenbank-Löschung entfernt zuerst die abgeleiteten Tagesaggregate und danach Rohdaten samt Metadaten. Anschließend startet der ESP32 neu und erzeugt leere, konsistente Datenstrukturen. Die Aktion wird serverseitig nur akzeptiert, wenn die Bestätigung exakt dem Projektnamen entspricht.
