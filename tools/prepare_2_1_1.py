#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from generate_webui_gzip import generate

ROOT = Path(__file__).resolve().parents[1]
SKETCH = ROOT / "arduino" / "Unterbrechungszaehler"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count == 0:
        if new in text:
            return
        raise RuntimeError(f"Muster nicht gefunden in {path}: {old[:80]!r}")
    if count != 1:
        raise RuntimeError(f"Muster {count}x gefunden in {path}, erwartet 1x")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def patch_config() -> None:
    replace_once(SKETCH / "Config.h", 'APP_VERSION[] = "2.1.0"', 'APP_VERSION[] = "2.1.1"')


def patch_web_service() -> None:
    path = SKETCH / "WebService.cpp"
    old_includes = '''#include "WebUi.h"
#include "WebUiPatch.h"
#include "WebUiFixes.h"
#include "WebUiNetwork.h"
#include "WebUiDisplay.h"
#include "WebUiV2.h"'''
    replace_once(path, old_includes, '#include "WebUiGzip.h"')

    old_route = '''  server_.on("/", HTTP_GET, [this]() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html; charset=utf-8", "");
    server_.sendContent_P(WEB_UI);
    server_.sendContent_P(WEB_UI_PATCH);
    server_.sendContent_P(WEB_UI_FIXES);
    server_.sendContent_P(WEB_UI_NETWORK);
    server_.sendContent_P(WEB_UI_DISPLAY);
    server_.sendContent_P(WEB_UI_V2);
    server_.sendContent("");
  });'''
    new_route = '''  server_.on("/", HTTP_GET, [this]() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.sendHeader("Content-Encoding", "gzip");
    server_.sendHeader("Vary", "Accept-Encoding");
    server_.send_P(200,
                   PSTR("text/html; charset=utf-8"),
                   WEB_UI_GZIP,
                   WEB_UI_GZIP_LENGTH);
  });'''
    replace_once(path, old_route, new_route)


def patch_legacy_fix() -> None:
    path = SKETCH / "WebUiFixes.h"
    old = '''// WebService sendet diesen Baustein bereits zwischen Basis-UI und den weiteren
// Erweiterungen. Die Makroverkettung haelt die bestehende 1.0.1-Reihenfolge
// unveraendert und fuegt die 2.0-UI ohne Kopie der grossen Behavior-Datei an.
#define WEB_UI_FIXES WEB_UI_BEHAVIOR); server_.sendContent_P(WEB_UI_V2'''
    new = '''// Legacy-Alias fuer den unkomprimierten Entwicklungsstand. Die produktive
// Auslieferung verwendet ab 2.1.1 die automatisch erzeugte WebUiGzip.h.
#define WEB_UI_FIXES WEB_UI_BEHAVIOR'''
    replace_once(path, old, new)


def patch_build_workflow() -> None:
    path = ROOT / ".github" / "workflows" / "build.yml"
    replace_once(
        path,
        "arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json",
        "arduino-cli core install esp32:esp32@3.3.11 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json",
    )

    old = '''      - name: Create test secrets
        run: cp arduino/Unterbrechungszaehler/Secrets.example.h arduino/Unterbrechungszaehler/Secrets.h
      - name: Compile firmware
        env:
          VERSION: ${{ steps.version.outputs.version }}
        run: |
          mkdir -p build
          arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir build arduino/Unterbrechungszaehler
          cp build/Unterbrechungszaehler.ino.bin "build/Unterbrechungszaehler-${VERSION}-OTA.bin"'''
    new = '''      - name: Create test secrets
        run: cp arduino/Unterbrechungszaehler/Secrets.example.h arduino/Unterbrechungszaehler/Secrets.h
      - name: Verify compressed web UI
        run: |
          python tools/generate_webui_gzip.py
          if ! git diff --quiet -- arduino/Unterbrechungszaehler/WebUiGzip.h; then
            echo "WebUiGzip.h ist nicht aktuell. Bitte Generator ausfuehren und Ergebnis committen." >&2
            git diff --stat -- arduino/Unterbrechungszaehler/WebUiGzip.h
            exit 1
          fi
      - name: Compile firmware
        env:
          VERSION: ${{ steps.version.outputs.version }}
        run: |
          set -o pipefail
          mkdir -p build
          arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir build arduino/Unterbrechungszaehler 2>&1 | tee build/compile.log
          size_line=$(grep -m1 "Sketch uses" build/compile.log || true)
          used=$(printf '%s\n' "$size_line" | sed -n 's/.*Sketch uses \([0-9][0-9]*\) bytes.*/\1/p')
          maximum=$(printf '%s\n' "$size_line" | sed -n 's/.*Maximum is \([0-9][0-9]*\) bytes.*/\1/p')
          if [ -z "$used" ] || [ -z "$maximum" ]; then
            echo "Firmwaregroesse konnte nicht aus dem Compiler-Log gelesen werden." >&2
            exit 1
          fi
          reserve=$((maximum - used))
          echo "Firmware: ${used}/${maximum} Bytes, Reserve: ${reserve} Bytes"
          if [ "$reserve" -lt 65536 ]; then
            echo "Zu wenig Flash-Reserve fuer einen belastbaren OTA-Release (mindestens 65536 Bytes erforderlich)." >&2
            exit 1
          fi
          cp build/Unterbrechungszaehler.ino.bin "build/Unterbrechungszaehler-${VERSION}-OTA.bin"'''
    replace_once(path, old, new)


def patch_changelog() -> None:
    path = ROOT / "CHANGELOG.md"
    text = path.read_text(encoding="utf-8")
    if "## 2.1.1" in text:
        return
    marker = "# Changelog\n\n"
    entry = '''## 2.1.1

Flash-/OTA-Fix ohne Funktionsabbau. Die Weboberflaeche bleibt inhaltlich unveraendert, wird aber nicht mehr als rund 150 kB Klartext in die Firmware gelinkt.

### Flash / OTA

- Weboberflaeche wird deterministisch mit gzip komprimiert und als `WebUiGzip.h` in PROGMEM eingebettet
- Browser erhalten die Seite mit `Content-Encoding: gzip` und entpacken sie automatisch
- doppelte Auslieferung von `WEB_UI_V2` entfernt
- editierbare `WebUi*.h`-Quelldateien bleiben erhalten; `tools/generate_webui_gzip.py` erzeugt daraus die komprimierte Fassung
- ESP32-Arduino-Core im CI auf `3.3.11` festgesetzt, damit Builds reproduzierbar bleiben
- CI bricht einen Release ab, wenn weniger als 64 KiB Programmspeicher-Reserve verbleiben
- Versionsstand auf `2.1.1` angehoben

### Kompatibilitaet

- keine Aenderung an Pinbelegung, LittleFS-Daten, Einstellungen, API oder Bedienung
- bestehende OTA-Partitionierung bleibt unveraendert
- 2.1.1 ist als direktes OTA-Update fuer bestehende kompatible 2.x-Installationen vorgesehen

'''
    if not text.startswith(marker):
        raise RuntimeError("Unerwarteter CHANGELOG-Aufbau")
    path.write_text(marker + entry + text[len(marker):], encoding="utf-8", newline="\n")


def main() -> int:
    patch_config()
    patch_web_service()
    patch_legacy_fix()
    patch_build_workflow()
    patch_changelog()
    raw_size, gzip_size, _ = generate()
    temporary_workflow = ROOT / ".github" / "workflows" / "prepare-2.1.1.yml"
    if temporary_workflow.exists():
        temporary_workflow.unlink()
    temporary_script = ROOT / "tools" / "prepare_2_1_1.py"
    if temporary_script.exists():
        temporary_script.unlink()
    print(f"2.1.1 vorbereitet; Web-UI {raw_size} -> {gzip_size} Bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
