#!/usr/bin/env python3
"""Portable release checks for Unterbrechungszaehler 3.0.0."""
from __future__ import annotations

import gzip
import hashlib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JS = (ROOT / "ui-src" / "app.js").read_text(encoding="utf-8")
HTML = (ROOT / "ui-src" / "index.html").read_text(encoding="utf-8")
CSS = (ROOT / "ui-src" / "app.css").read_text(encoding="utf-8")


def check(condition: bool, label: str) -> None:
    if not condition:
        raise AssertionError(label)
    print(f"PASS {label}")


def translation_keys(language: str, next_language: str | None) -> set[str]:
    start_token = f"    {language}: {{"
    start = JS.find(start_token)
    if start < 0:
        raise AssertionError(f"translation block missing: {language}")
    body_start = start + len(start_token)
    if next_language:
        end = JS.find(f"    {next_language}: {{", body_start)
    else:
        end = JS.find("\n  };", body_start)
    if end < 0:
        raise AssertionError(f"translation block end missing: {language}")
    body = JS[body_start:end]
    return set(re.findall(r"'([^']+)'\s*:", body))


def generated_asset() -> tuple[bytes, bytes, str]:
    bundle = HTML.replace("/*__APP_CSS__*/", CSS).replace("/*__APP_JS__*/", JS).encode("utf-8")
    compressed = gzip.compress(bundle, compresslevel=9, mtime=0)
    etag = hashlib.sha256(compressed).hexdigest()[:16]
    return bundle, compressed, etag


def header_bytes() -> tuple[bytes, str]:
    text = (ROOT / "web_assets.h").read_text(encoding="utf-8")
    values = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", text))
    match = re.search(r'WEB_ASSET_ETAG\[\]\s*=\s*"\\"([0-9a-f]+)\\""', text)
    if not match:
        raise AssertionError("web asset ETag missing")
    return values, match.group(1)


def main() -> None:
    config = (ROOT / "config.h").read_text(encoding="utf-8")
    project = (ROOT / "project_config.h").read_text(encoding="utf-8")
    hardware = (ROOT / "hardware_config.h").read_text(encoding="utf-8")
    partitions = (ROOT / "partitions.csv").read_text(encoding="utf-8")

    check('PROJECT_NAME[] = "Unterbrechungszähler"' in config, "project name")
    check('SOFTWARE_VERSION[] = "3.0.0"' in config, "project version 3.0.0")
    check('AVAILABLE_LANGUAGES_JSON[] = "[\\\"de\\\",\\\"en\\\",\\\"swg\\\"]"' in config, "declared UI languages")
    check("RAW_EVENT_CAPACITY = 100000" in project, "100,000 raw-event capacity")
    check("RAW_RECORD_SIZE = 9" in project, "9-byte raw record")
    check("DAILY_AGGREGATE_CAPACITY = 2300" in project, "daily aggregate retention")
    check("PENDING_EVENT_CAPACITY = 64" in project, "64-event fixed persistence queue")
    check(re.search(r'\{"di1"[^\n]*13,\s*PullMode::Up,\s*false[^\n]*25,\s*true,', hardware) is not None, "DI1 GPIO13 active-edge interrupt latch")
    check("AUDIO_RX_PIN = 18" in hardware and "AUDIO_TX_PIN = 19" in hardware and "AUDIO_BUSY_PIN = 39" in hardware, "DY-SV17F pin map")
    check("0x2D0000, 0x130000" in partitions, "LittleFS custom partition")

    de = translation_keys("de", "en")
    en = translation_keys("en", "swg")
    swg = translation_keys("swg", None)
    check(de == en == swg, f"i18n key parity ({len(de)} keys/language)")

    positions = [JS.find(f"{{ id: '{name}'") for name in ("device", "wifi", "memory", "time", "hardware", "ota")]
    check(all(position >= 0 for position in positions) and positions == sorted(positions), "device card order")
    check(JS.count("setInterval(") == 1, "exactly one permanent frontend interval")
    check("const weeks = Array.from({ length: 53 }, (_, i) => String(i + 1))" in JS, "calendar-week heatmap headers use numbers only")
    check("Bindings.notify('analytics.monthWeek')" in JS and "Bindings.notify('analytics.hourly')" in JS, "manual heatmap filters trigger targeted rerender")
    check("projectSettings: renderProjectSettings" in JS, "Home project settings card")
    check("SoundMode::Rotate" in (ROOT / "project_preferences.cpp").read_text(encoding="utf-8"), "rotating interruption sound mode")
    check("Track 1 belongs exclusively to the boot sound" in (ROOT / "interruption_service.cpp").read_text(encoding="utf-8"), "boot track excluded from rotating interruption sound")
    check(not re.search(r"\.(?:innerHTML|outerHTML)\s*=|insertAdjacentHTML\s*\(|document\.write\s*\(", JS), "no unsafe bulk DOM HTML writes")
    external = re.search(r"<(?:script|img|link)\b[^>]*(?:src|href)=[\"\']https?://", HTML, re.IGNORECASE)
    check(external is None, "no external HTML dependencies")

    expected_routes = (
        "/api/interruptions/event",
        "/api/interruptions/live",
        "/api/interruptions/sound",
        "/api/interruptions/preferences",
        "/api/interruptions/storage",
        "/api/interruptions/analytics",
        "/api/interruptions/heatmap/hourly",
        "/api/interruptions/heatmap/month-week",
        "/api/interruptions/heatmap/year-month",
        "/api/interruptions/export.csv",
    )
    web_server = (ROOT / "web_server.cpp").read_text(encoding="utf-8")
    check(all(route in web_server for route in expected_routes), "project API routes")

    subprocess.run([sys.executable, str(ROOT / "tools" / "test_interruption_storage.py")], check=True)
    subprocess.run([sys.executable, "-m", "py_compile", str(ROOT / "tools" / "build_web.py"), str(ROOT / "tools" / "test_interruption_storage.py"), str(ROOT / "tools" / "release_check.py")], check=True)
    if subprocess.run(["node", "--check", str(ROOT / "ui-src" / "app.js")], check=False).returncode != 0:
        raise AssertionError("JavaScript syntax")
    print("PASS JavaScript syntax")

    bundle, compressed, etag = generated_asset()
    stored, stored_etag = header_bytes()
    check(stored == compressed, "web_assets.h matches readable UI sources")
    check(stored_etag == etag, "web asset ETag")
    check(gzip.decompress(stored) == bundle, "gzip roundtrip")
    print(f"INFO web bundle={len(bundle)} bytes gzip={len(compressed)} bytes etag={etag}")
    print("PASS portable release checks")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, subprocess.CalledProcessError) as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        raise SystemExit(1)
