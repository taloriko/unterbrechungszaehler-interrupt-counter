# Hardware zusammenbauen

> **Status: In Arbeit**

## Taster

Der Ereigniseingang liegt auf GPIO27.

```text
GPIO27 ---- potentialfreier Kontakt ---- GND
```

Der Eingang verwendet `INPUT_PULLUP`.

**Keine externe Spannung an GPIO27 anlegen.**

## Schalter für den Autark-Modus

Optional kann ein potentialfreier Schalter an GPIO33 angeschlossen werden.

```text
GPIO33 ---- potentialfreier Schalter ---- GND
```

- GPIO33 offen: normaler WLAN-/Netzbetrieb
- GPIO33 gegen GND: Autark-Modus **BETA**

Auch GPIO33 verwendet `INPUT_PULLUP`.

**Keine externe Spannung an GPIO33 anlegen.**

Weitere Angaben zu Gehäuse, Klemmen und mechanischem Aufbau folgen.
