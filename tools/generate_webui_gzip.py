#!/usr/bin/env python3
from __future__ import annotations

import gzip
import hashlib
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKETCH = ROOT / "arduino" / "Unterbrechungszaehler"
OUTPUT = SKETCH / "WebUiGzip.h"
COMPONENTS = (
    "WebUi.h",
    "WebUiPatch.h",
    "WebUiBehavior.h",
    "WebUiV2.h",
    "WebUiNetwork.h",
    "WebUiDisplay.h",
)
RAW_RE = re.compile(r'R"HTML\((.*)\)HTML";', re.DOTALL)


def extract_component(path: Path) -> str:
    text = path.read_text(encoding="utf-8")
    match = RAW_RE.search(text)
    if not match:
        raise RuntimeError(f"Kein R\\\"HTML-Block in {path}")
    return match.group(1)


def build_payload() -> tuple[bytes, bytes]:
    html = "".join(extract_component(SKETCH / name) for name in COMPONENTS).encode("utf-8")
    packed = gzip.compress(html, compresslevel=9, mtime=0)
    return html, packed


def render_header(html: bytes, packed: bytes) -> str:
    lines = []
    for offset in range(0, len(packed), 48):
        chunk = packed[offset : offset + 48]
        lines.append('"' + ''.join(f"\\{byte:03o}" for byte in chunk) + '"')

    return "\n".join(
        [
            "#pragma once",
            "",
            "#include <Arduino.h>",
            "",
            "// Automatisch erzeugt mit tools/generate_webui_gzip.py.",
            "// Quelldateien bleiben die editierbaren WebUi*.h-Dateien; diese Datei nicht von Hand bearbeiten.",
            f"// Unkomprimiert: {len(html)} Bytes | gzip: {len(packed)} Bytes | SHA256: {hashlib.sha256(html).hexdigest()}",
            "static const char WEB_UI_GZIP[] PROGMEM =",
            *lines,
            ";",
            "",
            "static constexpr size_t WEB_UI_GZIP_LENGTH = sizeof(WEB_UI_GZIP) - 1;",
            "",
        ]
    )


def generate() -> tuple[int, int, bool]:
    html, packed = build_payload()
    rendered = render_header(html, packed)
    previous = OUTPUT.read_text(encoding="utf-8") if OUTPUT.exists() else None
    changed = previous != rendered
    if changed:
        OUTPUT.write_text(rendered, encoding="utf-8", newline="\n")
    return len(html), len(packed), changed


def main() -> int:
    raw_size, gzip_size, changed = generate()
    saving = raw_size - gzip_size
    print(f"Web-UI: {raw_size} -> {gzip_size} Bytes gzip, Ersparnis {saving} Bytes")
    print("WebUiGzip.h aktualisiert" if changed else "WebUiGzip.h ist aktuell")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
