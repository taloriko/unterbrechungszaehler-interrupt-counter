# Software-Architektur – modularer Reboot

> **Status: In Arbeit / Testzweig**

Der neue Softwareaufbau liegt getrennt von der bisherigen Firmware unter:

```text
arduino/UnterbrechungszaehlerModular/
```

Der bestehende Programmstand bleibt waehrend der Erprobung unveraendert.

## Ziel

Die Firmware soll auch bei weiteren Funktionen gut lesbar, erweiterbar und wartbar bleiben. Ein Fehler in einer optionalen Funktion soll den eigentlichen Unterbrechungszaehler moeglichst nicht beeinflussen.

Die Hauptdatei enthaelt deshalb nur noch den Programmablauf. Fachlogik und Hardwarezugriffe sind in eigene Module getrennt.

## Module

| Modul | Aufgabe |
| --- | --- |
| `Config` | zentrale Pins, Grenzwerte, Dateinamen und Konstanten |
| `StorageService` | LittleFS, normaler Ringspeicher, Langzeitarchiv und Autark-Daten |
| `TimeService` | Systemzeit, NTP, Browserzeit und Zeitquelle |
| `RtcService` | optionale DS3231-Echtzeituhr |
| `DisplayService` | optionales SH1106-Display |
| `NetworkService` | WLAN, Fallback-Access-Point und mDNS |
| `CounterService` | normales Erfassen und Loeschen von Unterbrechungen |
| `AutarkService` | Autark-Sessions und relative Ereigniszeiten |
| `InputService` | Taster und Autark-Schalter inklusive Entprellung |
| `LedService` | LED-Rueckmeldungen ohne blockierende Wartezeiten |
| `AnalyticsService` | Heatmap- und Langzeitauswertungen |
| `WebService` | HTTP-API, CSV-Export und Weboberflaeche |
| `WebUi` | HTML, CSS, JavaScript und zentrale Sprachtexte |

## Grundregeln fuer Erweiterungen

1. Die Hauptdatei bleibt ein Orchestrator. Neue Fachlogik gehoert in ein eigenes Modul.
2. Ein Modul soll nur die Abhaengigkeiten kennen, die es wirklich benoetigt.
3. Hardware, die nicht zwingend erforderlich ist, muss optional bleiben.
4. Der normale Ereignisspeicher hat Vorrang vor Auswertung und Komfortfunktionen.
5. Langzeitarchiv und Heatmaps duerfen bei einem Fehler den 10.000-Ereignis-Speicher nicht blockieren.
6. Beschaedigte Speicherdateien werden nicht ungefragt geloescht. Sie werden als `.invalid` gesichert und neu angelegt.
7. Sichtbare Texte in der Weboberflaeche werden ueber zentrale Sprachschluessel gepflegt.
8. Deutsch und Englisch muessen fuer jeden verwendeten Sprachschluessel vorhanden sein.
9. Zeitkritische Schleifen sollen nicht mit langen `delay()`-Aufrufen blockiert werden.
10. Im Autarkmodus werden WLAN und Webserver abgeschaltet, die CPU wird reduziert und im Leerlauf Light-Sleep verwendet.

## Sprachumschaltung

Die Sprachtexte liegen zentral im Objekt `I18N` in `WebUi.h`.

HTML-Elemente verwenden Sprachschluessel wie:

```html
<span data-i18n="device.storage"></span>
```

Im JavaScript wird der Text ausschliesslich ueber `tr("device.storage")` aufgeloest.

Wenn eine neue Anzeige hinzukommt, muss der Schluessel sowohl in `de` als auch in `en` ergaenzt werden.

## Speicherstrategie

Der bestehende binäre 10.000-Ereignis-Ringspeicher bleibt kompatibel. Zusaetzlich wird das Langzeitarchiv mit bis zu 100.000 Zeitstempeln weitergefuehrt.

Beim Speichern eines normalen Ereignisses ist der kleine Ringspeicher die fuehrende Datenquelle. Schlaegt nur das Langzeitarchiv fehl, bleibt die Erfassung im normalen Ringspeicher funktionsfaehig und der Archivstatus wird als nicht synchron markiert.

## Zeitstrategie

Prioritaet der Zeitquellen:

1. NTP
2. RTC
3. Browserzeit
4. keine absolute Zeit

Eine gueltige RTC kann direkt beim Start die Systemzeit setzen. Nach einer erfolgreichen NTP-Synchronisation wird die RTC auf die NTP-Zeit nachgefuehrt.

## Autarkbetrieb

Beim Umschalten auf Autarkbetrieb:

- neue Session anlegen
- WLAN und Webserver abschalten
- CPU-Takt reduzieren
- RTC-Zeit verwenden, wenn vorhanden
- OLED-Startstatus 15 Sekunden anzeigen, wenn Display vorhanden
- danach Display ausschalten
- im Leerlauf Light-Sleep verwenden

Beim Verlassen wird eine Ende-Markierung gespeichert. Falls zu diesem Zeitpunkt noch keine absolute Zeit vorhanden ist, wird der Zeitanker nach der naechsten gueltigen Zeitsynchronisation nachgetragen.

## Teststrategie vor Zusammenfuehrung

Der neue Zweig soll erst nach einem Vergleich mit der bisherigen Firmware zusammengefuehrt werden.

Zu pruefen sind mindestens:

- Boot mit und ohne WLAN
- Boot mit und ohne RTC
- Boot mit und ohne OLED
- kurzer Tastendruck
- langer Tastendruck
- virtueller Taster in der Weboberflaeche
- 10.000er Ringspeicher
- Langzeitarchiv
- Neustart mit vorhandenen Daten
- CSV-Export
- NTP-Server aendern
- Browserzeit ohne NTP und RTC
- Umschalten Deutsch / Englisch auf jedem Reiter
- Light-/Dark-/System-Darstellung
- Heatmaps und Jahreswechsel
- Autarkbetrieb mit RTC
- Autarkbetrieb ohne RTC
- Rueckkehr aus Autarkbetrieb
- Verhalten bei defekter oder ungueltiger Speicherdatei

Erst wenn diese Punkte bestanden sind, wird der Reboot fuer die Zusammenfuehrung mit dem Hauptzweig vorbereitet.
