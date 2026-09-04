# 433-MHz-/CC1101-Prototyp testen

> **Entwicklungsstand `3.3.0-dev433` – kein Release.** Der Code liegt nur im Draft-PR #12. Die OTA aus GitHub Actions ist ausschließlich für Hardwaretests gedacht.

## Ziel

Der ESP32 bleibt Master. Der lokale DI1-Taster auf GPIO13 funktioniert weiter. Ein CC1101/RF1100SE empfängt zusätzlich 433,92-MHz-OOK-Festcode-Taster. Jeder angelernte Sender wird einer **stabilen Source-ID** zugeordnet; der frei vergebene Name steht nur einmal in der SourceRegistry und wird nicht in jedem Event dupliziert.

## Verdrahtung CC1101 / RF1100SE

| CC1101 | ESP32 |
|---|---:|
| VCC | **3,3 V** |
| GND | GND |
| SCK | GPIO14 |
| MISO / SO | GPIO32 |
| MOSI / SI | GPIO23 |
| CSN / SS | GPIO25 |
| GDO0 | GPIO26 |
| GDO2 | GPIO27 |

**CC1101 nur mit 3,3 V versorgen.** Eine passende 433-MHz-Antenne verbessert den Test erheblich.

## Welche Taster unterstützt der erste Prototyp?

Bewusst eng: gängige **433,92-MHz-ASK/OOK-Festcode-Sender** mit etwa 20–32 Bit und kurzen/langen Pulspaaren. Das ist noch kein universeller 433-MHz-Decoder. Rolling Code, Keeloq und unbekannte/proprietäre Protokolle sind nicht zugesichert.

Die Firmware verlangt zwei übereinstimmende empfangene Frames, bevor ein Tastendruck akzeptiert wird, und unterdrückt die Wiederholungen eines einzelnen Funk-Tastendrucks anschließend kurz. Damit soll ein typischer Sender mit mehreren identischen Wiederholtelegrammen genau eine Unterbrechung erzeugen.

## Test-OTA

Die OTA wird nicht als GitHub Release veröffentlicht. Sie entsteht ausschließlich als Artefakt des erfolgreichen PR-Builds. Nach jeder weiteren Codeänderung im Draft-PR gilt immer das Artefakt des neuesten grünen `Firmware build`.

ZIP des erfolgreichen Builds herunterladen, entpacken und die enthaltene `Unterbrechungszaehler-3.3.0-dev433-OTA.bin` über die bestehende OTA-Seite hochladen.

## Anlernen

1. `3.3.0-dev433` OTA installieren.
2. Weboberfläche → **Home → Funkbutton anlernen**. Dort stehen absichtlich nur Anlernen und die Anzahl der aktuell gebundenen Taster.
3. Namen eingeben, z. B. `Anna`.
4. **Neuen Button anlernen** wählen.
5. Gewünschten Funkbutton innerhalb von 30 Sekunden mehrfach drücken.
6. Der Anlerndruck wird absichtlich **nicht** als Unterbrechung gezählt.
7. Danach normal drücken. Der Event läuft über denselben `InterruptionService` wie Master/Web und bekommt seine stabile Source-ID.

Es stehen im ersten Prototyp genau **10 Funk-IDs (6–15)** zur Verfügung. IDs 0–5 bleiben für Unknown/Master/Web/Software/API/Technik reserviert.

## Hardwarestatus und Empfangstest

Unter **Gerät → Hardware** erscheint der CC1101 wie RTC, Display und Sound als normales Hardwaremodul. Dort werden Status, SPI-/Chipdaten, Pins sowie Frame-Zähler angezeigt. **Hardware prüfen** fragt den CC1101 über SPI erneut ab. **Empfang testen (5 s)** wartet auf ein gültiges Funktelegramm; ein dabei empfangenes Testtelegramm wird absichtlich nicht als Unterbrechung gespeichert.

Die vollständige Liste der angelernten Funkbuttons befindet sich ebenfalls unter **Gerät → Hardware**. Dort werden Namen geändert, Sender gelöst oder ersetzt.

## Sender ersetzen ohne Historie umzubenennen

Bei einem defekten Sender in derselben Quellenzeile **Sender ersetzen** wählen und den neuen Sender drücken. Die Source-ID und damit die Zuordnung aller historischen Events bleibt gleich; nur die physische RF-Bindung wird aktualisiert.

**Sender lösen** entfernt nur die Funkbindung. Die Source-ID und der Name werden absichtlich nicht automatisch freigegeben oder wiederverwendet.

## Speicherung

Das Raw-Event bleibt **9 Byte** groß, der Ring bleibt bei **100.000 Events**. Neue Records tragen eine 4-Bit-Source-ID in einem selbstidentifizierenden v3-Bitlayout. Alte v2-Records werden unverändert weitergelesen. Namen/RF-Codes liegen in einer kleinen CRC-geschützten NVS-Registry.

CSV enthält die numerische `source_id`; Namen werden nicht pro Event gespeichert. Die Weboberfläche löst Namen über die SourceRegistry auf.

## Was beim Hardwaretest beobachten?

- Bootlog: `CC1101 ready ...`
- Headerstatus `433 MHz` sollte OK sein.
- Unter Gerät / Hardware muss der CC1101 mit denselben Status-/Prüfmechanismen wie die übrige Hardware erscheinen.
- Der 5-s-Empfangstest muss bei einem passenden Tastendruck ein Test-Frame melden, ohne den Unterbrechungszähler zu erhöhen.
- Im Heimnetz meldet sich die WLAN-Station als `Unterbrechungszaehler` statt als generischer ESP32-Hostname.
- Beim Anlernen sollte die neue Quelle erscheinen.
- Ein einzelner menschlicher Tastendruck darf trotz RF-Wiederholtelegrammen nur **ein** Event ergeben.
- Master-DI1 und Webbutton müssen parallel weiter funktionieren.
- Nach Neustart müssen Name, Source-ID und Senderbindung erhalten bleiben.
- Sender ersetzen muss dieselbe Source-ID behalten.

Wenn ein Sender nicht erkannt wird, sind `lastCode`, `lastBits`, `rejectedFrames` und `overflowFrames` über `/api/interruptions/sources` als Diagnosewerte verfügbar.
