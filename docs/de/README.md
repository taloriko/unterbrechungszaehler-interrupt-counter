# Unterbrechungszähler

> [!WARNING]
> **KI-Hinweis:** Dieses Projekt wurde maßgeblich mit Unterstützung von KI erstellt, anschließend aber praktisch getestet, überarbeitet und weiterentwickelt.  
> Wem KI-generierter Code grundsätzlich nicht ins Weltbild passt, kann an dieser Stelle aufhören. Spart uns beiden Zeit. ;-)

> **Stand dieser README:** `2.1.0`

[English](../en/README.md) · [Schwäbisch](../swg/README.md) · [Projektstart](../../README.md)

---

## Für was brauche ich das Ding?

Du sitzt konzentriert an einer Aufgabe.

Dann kommt ein Kollege.  
Dann klingelt das Telefon.  
Dann braucht jemand „nur ganz kurz“ etwas.  
Dann kommt der Chef.  
Und irgendwann fragst du dich, was du eigentlich vor zwei Stunden machen wolltest.

Genau dafür gibt es den **Unterbrechungszähler**.

**Knopf drücken → Zeitpunkt speichern → später im Browser auswerten**

Damit ist es nicht mehr nur ein Gefühl wie:

> „Heute bin ich irgendwie zu nichts gekommen.“

Sondern du kannst tatsächlich sehen, **wie oft und wann du unterbrochen wurdest**.

Ob das deinen Chef anschließend interessiert, ist natürlich eine völlig andere wissenschaftliche Fragestellung. ;-)

---

## Kann das Ding noch mehr?

Ja.

Der Unterbrechungszähler ist technisch gesehen nicht auf einen Taster beschränkt.

Anstelle des Tasters kannst du praktisch jeden geeigneten **potentialfreien Kontakt** verwenden.

Zum Beispiel:

- Störmeldung einer Maschine über einen Relaiskontakt
- Unterbrechung einer Lichtschranke
- Tür- oder Fensterkontakt
- Schaltkontakt einer Anlage
- Betriebs- oder Fehlermeldungen
- Kontakt eines externen Tasters
- oder irgendeinen anderen Kontakt, bei dem du später wissen möchtest: **Wann ist das eigentlich passiert?**

Kurz gesagt:

**Kontakt schaltet → Ereignis wird gespeichert → Daten werden visualisiert.**

Sei kreativ.

Wenn jemand das Projekt irgendwann benutzt, um die Öffnungen des Kühlschranks zu zählen, möchte ich davon allerdings erfahren.

---

## Warum habe ich das gemacht?

Über die Jahre hat sich bei uns im Unternehmen einiges verändert.

Wir bekamen mehr Mitarbeiter.  
Dann mehr Aufgaben.  
Dann noch mehr Mitarbeiter.  
Dann noch mehr Aufgaben.

Und wenn die wirtschaftliche Lage schwieriger wird, kennt vermutlich jeder das bewährte Managementkonzept:

**Noch mehr Aufgaben.**

Irgendwann hatte ich das Gefühl, keinen klaren Gedanken mehr fassen zu können – geschweige denn, eine Aufgabe von vielleicht 60 Minuten einmal konzentriert und ohne Unterbrechung fertigzustellen.

Das Problem:

In einem größeren Unternehmen reicht

> „So kann ich nicht vernünftig arbeiten.“

als Argument irgendwann nicht mehr unbedingt aus.

Also entstand die Idee, etwas zurückzugeben, das große Unternehmen besonders lieben:

**Daten, Statistiken, Dokumentation und Auswertungen.**

Oder anders gesagt:

Ich setze die Bürokratie- und Dokumentationswut einfach gegen das System ein. ;-)

Da mein Arbeitstag allerdings ohnehin schon genug Chaos enthält, durfte daraus natürlich keine zusätzliche Verwaltungsaufgabe entstehen.

Die wichtigste Anforderung war deshalb von Anfang an:

**Ein Knopfdruck. Fertig.**

Keine App öffnen.  
Kein Formular ausfüllen.  
Keine Kategorie auswählen.  
Keine Excel-Liste pflegen.

Einfach drücken und weiterarbeiten.

Den Rest macht das Gerät.

---

## Was kann das Gerät?

- **Ereignisse per Taster oder potentialfreiem Kontakt erfassen**  
  Einfach, schnell und ohne jedes Mal einen Verwaltungsakt daraus zu machen.

- **Lokale Weboberfläche ohne Cloud**  
  Sehr wichtig. Nicht alles muss erst einmal über drei Rechenzentren geschickt werden, nur damit man einen Knopf zählen kann. ;-)

- **Tagesansicht, Verlauf, Details und Heatmaps**  
  Damit man schnell sieht, wann besonders viel los war – oder wann offenbar alle anderen Mittagspause hatten.

- **CSV-Export und Langzeit-Ringspeicher**  
  Falls aus „Ich werde ständig unterbrochen“ irgendwann „Zeig mir die Daten“ wird.

- **Optionale DS3231-RTC**  
  Damit das Gerät auch ohne WLAN weiß, wie spät es ist. Revolutionäre Technik.

- **Optionales SH1106-OLED mit 128 × 64 Pixeln**  
  Technisch nicht zwingend notwendig, sieht aber sofort mindestens 37 % professioneller aus.

- **Fallback-WLAN für lokalen Zugriff**  
  Wer keine Cloud möchte, sollte das Gerät schließlich trotzdem noch erreichen können.

- **Autarkbetrieb über Akku oder Powerbank**  
  Falls am gewünschten Einsatzort keine Steckdose vorhanden ist – oder du tatsächlich irgendwann auf einer einsamen Insel Unterbrechungen dokumentieren musst.

- **Deutsch, Englisch und Schwäbisch in der Oberfläche**  
  Internationalisierung muss schließlich irgendwo anfangen.

- **MagSafe-Ring für Akku oder Halterungen**  
  Weil Klettband zwar funktioniert, aber Magnete einfach mehr nach Zukunft aussehen.

---

## Schnellstart

1. [Hardware beschaffen](HARDWARE-BESCHAFFEN.md)
2. [Hardware zusammenbauen](HARDWARE-ZUSAMMENBAU.md)
3. [Software konfigurieren](SOFTWARE.md)
4. [Firmware flashen](FLASHEN.md)
5. [Gerät benutzen](NUTZUNG-NORMAL.md)

Danach musst du eigentlich nur noch dafür sorgen, dass dich jemand unterbricht.

Das sollte in den meisten Büros kein größeres Problem darstellen.

---

## Weitere Dokumentation

- [Autarkbetrieb](NUTZUNG-AUTARK.md)
- [Software-Architektur](SOFTWARE-ARCHITEKTUR.md)
- [Änderungen / Changelog](../../CHANGELOG.md)

---

## Warum Schwäbisch?

Weil technische Projekte nicht immer komplett ernst sein müssen.

Software darf funktionieren **und** trotzdem ein bisschen Persönlichkeit haben.

Ich mag Schwäbisch und wollte außerdem irgendwo ein kleines Easter Egg einbauen.

Also gibt es die Oberfläche auch auf Schwäbisch.

Ob das die internationale Verbreitung des Projekts beschleunigt oder massiv behindert, wird die Zukunft zeigen.

---

## Lizenz

Dieses Projekt steht unter der **MIT-Lizenz**.

Kurz gesagt: benutzen, verändern, erweitern und daraus etwas Eigenes bauen ist ausdrücklich erlaubt.

Wenn daraus irgendwann ein millionenschweres Produkt entsteht, freue ich mich natürlich über eine Postkarte.
