# Assembly

> **Status: Work in progress**

## Event input

```text
GPIO27 ---- dry contact ---- GND
```

GPIO27 uses `INPUT_PULLUP`.

**Do not apply external voltage to GPIO27.**

## Standalone mode switch

```text
GPIO33 ---- dry-contact switch ---- GND
```

- GPIO33 open: normal Wi-Fi/network mode
- GPIO33 connected to GND: standalone mode **BETA**

GPIO33 also uses `INPUT_PULLUP`.

**Do not apply external voltage to GPIO33.**

More details about enclosure, terminals and mechanical assembly will follow.
