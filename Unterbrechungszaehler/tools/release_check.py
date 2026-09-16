#!/usr/bin/env python3
"""Portable release checks for Unterbrechungszaehler 3.6.1."""
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
    end = JS.find(f"    {next_language}: {{", body_start) if next_language else JS.find("\n  };", body_start)
    if end < 0:
        raise AssertionError(f"translation block end missing: {language}")
    return set(re.findall(r"'([^']+)'\s*:", JS[body_start:end]))


def generated_asset() -> tuple[bytes, bytes, str]:
    bundle = HTML.replace("/*__APP_CSS__*/", CSS).replace("/*__APP_JS__*/", JS).encode("utf-8")
    compressed = gzip.compress(bundle, compresslevel=9, mtime=0)
    return bundle, compressed, hashlib.sha256(compressed).hexdigest()[:16]


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
    sketch = (ROOT / "Unterbrechungszaehler.ino").read_text(encoding="utf-8")
    service = (ROOT / "interruption_service.cpp").read_text(encoding="utf-8")
    service_h = (ROOT / "interruption_service.h").read_text(encoding="utf-8")
    cycle = (ROOT / "work_cycle.cpp").read_text(encoding="utf-8")
    cycle_h = (ROOT / "work_cycle.h").read_text(encoding="utf-8")
    store = (ROOT / "interruption_store.cpp").read_text(encoding="utf-8")
    aggregates = (ROOT / "interruption_aggregates.cpp").read_text(encoding="utf-8")
    insights = (ROOT / "focus_insights.cpp").read_text(encoding="utf-8")
    views = (ROOT / "display_views.cpp").read_text(encoding="utf-8")
    audio = (ROOT / "audio_dy_sv17f.cpp").read_text(encoding="utf-8")
    api = (ROOT / "interruption_api.cpp").read_text(encoding="utf-8")
    server = (ROOT / "web_server.cpp").read_text(encoding="utf-8")
    partitions = (ROOT / "partitions.csv").read_text(encoding="utf-8")

    # Identity / frozen platform invariants.
    check('PROJECT_NAME[] = "Unterbrechungszähler"' in config, "project name")
    check('SOFTWARE_VERSION[] = "3.6.1"' in config, "project version 3.6.1")
    check("RAW_EVENT_CAPACITY = 100000" in project and "RAW_RECORD_SIZE = 9" in project, "100,000 x 9-byte raw ring unchanged")
    check("DAILY_AGGREGATE_CAPACITY = 2300" in project and "DAILY_RECORD_SIZE = 64" in project, "daily aggregate format unchanged")
    check("PENDING_EVENT_CAPACITY = 64" in project, "fixed 64-event persistence queue")
    check("0x2D0000, 0x130000" in partitions, "LittleFS custom partition")
    check(re.search(r'\{"di1"[^\n]*13,\s*PullMode::Up,\s*false[^\n]*25,\s*true,', hardware) is not None, "DI1 GPIO13 active-edge interrupt latch")
    check("AUDIO_RX_PIN = 18" in hardware and "AUDIO_TX_PIN = 19" in hardware and "AUDIO_BUSY_PIN = 39" in hardware, "DY-SV17F pin map")

    # 3.6.1 work-cycle contract: classification wraps the proven capture path.
    check('WORK_CYCLE_INPUT_ID[] = "di1"' in project, "work cycle owns DI1")
    check('INTERRUPTION_INPUT_ID[] = "cycle-managed"' in project, "legacy direct DI callback cannot consume DI1")
    check("WORK_CYCLE_LONG_PRESS_MS = 2000" in project, "two-second explicit cycle end")
    check("WORK_CYCLE_GOODBYE_DISPLAY_MS = 10000" in project, "ten-second goodbye display")
    check('CYCLE_JOURNAL_PATH[] = "/cycles.log"' in project, "separate cycle journal")
    check("STATE_MAGIC = 0x32435943UL" in cycle, "CYC2 ignores old 3.6.0 pending state")
    check("Preferences" in cycle and "WORK_CYCLE_PREF_NAMESPACE" in cycle, "cycle state persisted in NVS")
    check("JOURNAL_START" in cycle and "JOURNAL_END" in cycle and "appendJournal" in cycle, "start/end cycle journal markers")
    check("state.pending" not in cycle and "pendingEpochSeconds" not in cycle, "no deferred last-press candidate remains")
    check("InterruptionService::capture(InterruptionTypes::EventSource::PhysicalButton)" in cycle, "accepted short press uses normal immediate capture")
    check("captureAtEpoch" not in cycle and "captureAtEpoch" not in service and "captureAtEpoch" not in service_h, "obsolete deferred timestamp API removed")
    check("finalizeAutomaticEnd(epochSeconds)" in cycle and "local.dayIndex != state.dayIndex" in cycle, "automatic local-day fallback")
    check("never reclassified" in cycle, "day-change fallback never rewrites captured interruptions")
    check("heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS" in cycle, "long press detected on release")
    check("beginGoodbye" in cycle and "FEIERABEND" in cycle and "todayCount" in cycle, "manual goodbye includes today count")
    check("if (goodbyeActive) return;" in cycle, "goodbye ignores further physical input")
    check("exclusiveGoodbyeActive" in cycle_h and "if (!WorkCycle::exclusiveGoodbyeActive())" in sketch, "goodbye exclusively blocks normal interruption display servicing")
    check("delay(" not in cycle, "work-cycle path remains nonblocking")
    check("PhysicalButtonGuard::accept(shortPressGuard, nowMs" in cycle, "10-second guard applies to real short interruptions")
    start_block = cycle.split("void startCycle", 1)[1].split("void renderGoodbye", 1)[0]
    check("PhysicalButtonGuard::accept" not in start_block and "AudioDySv17f" not in start_block and "showSuppressed" not in start_block,
          "cycle start is silent and does not consume anti-spam window")
    capture_block = cycle.split("void captureShortPress", 1)[1].split("void handleShortPress", 1)[0]
    check(capture_block.find("PhysicalButtonGuard::accept") < capture_block.find("InterruptionService::capture"),
          "anti-spam guard precedes immediate real interruption capture")
    check("playPriorityFeedbackTrack(ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK)" in cycle, "track 2 reserved for actually suppressed presses")
    check("notifySuppressedPhysicalPress" in cycle, "suppressed short press keeps OLED feedback")

    # Compatibility: the normal interruption service itself is back to the proven model.
    check("bool capture(InterruptionTypes::EventSource source)" in service, "standard interruption capture retained")
    check("if (source == InterruptionTypes::EventSource::PhysicalButton) serviceUrgent();" in service,
          "physical interruption keeps immediate normal feedback")
    check("event.eventSource" in store and "<< 20" in store, "event source remains packed in existing raw record")
    check("eventType" not in store, "no cycle type is packed into 9-byte raw record")
    check("InterruptionAggregates::apply(event, sequence)" in service, "captured events still feed daily aggregates")
    check("scanRawAnalytics" in api and "elapsedSeconds == current.deltaSeconds" in api, "retained adjacent-event interval scan unchanged")
    check("InterruptionStore::readSequence" in insights, "Focus & Insights remains raw-interruption based")
    check("Preferences" not in insights and "LittleFS" not in insights, "Focus & Insights adds no persistence")

    # Existing anti-spam/display/audio rules.
    check("PHYSICAL_BUTTON_COOLDOWN_MS = 10000" in project, "10-second short-press cooldown")
    check("INTERRUPTION_SPAM_SOUND_TRACK = 2" in project and "INTERRUPTION_SOUND_FIRST_NORMAL_TRACK = 3" in project, "track reservation 1 boot, 2 anti-spam, 3+ normal")
    check("renderSpamFlickerFrame" in views and "DISPLAY_SPAM_FLICKER_MS = PHYSICAL_BUTTON_COOLDOWN_MS" in project, "nonblocking spam flicker retained")
    check("attachInterrupt" in audio and "busyIrqArmed" in audio and "onBusyEdge" in audio, "audio BUSY monitoring remains event driven")
    check("AUDIO_DIAGNOSTIC_STATUS_POLL_MS = 500" in hardware, "bounded manual audio-test polling")
    check("replacedNormalVerification" in audio, "priority anti-spam audio cannot strand normal verification")

    # API / reset / web invariants.
    expected_routes = (
        "/api/interruptions/event", "/api/interruptions/live", "/api/interruptions/sound",
        "/api/interruptions/preferences", "/api/interruptions/storage",
        "/api/interruptions/storage/reset", "/api/interruptions/analytics",
        "/api/interruptions/heatmap/hourly", "/api/interruptions/heatmap/month-week",
        "/api/interruptions/heatmap/year-month", "/api/interruptions/export.csv",
    )
    check(all(route in server for route in expected_routes), "project API routes retained")
    check("AppConfig::PROJECT_NAME" in server and "/api/interruptions/storage/reset" in server, "project-name protected database reset")
    check("bool eraseAll()" in store and "bool eraseAll()" in aggregates, "raw/aggregate reset paths retained")
    check("physical_button" in api and "web_button" in api and "sourceMatches" in api, "source filters retained")
    check(not re.search(r"\.(?:innerHTML|outerHTML)\s*=|insertAdjacentHTML\s*\(|document\.write\s*\(", JS), "no unsafe bulk DOM HTML writes")
    check(re.search(r"<(?:script|img|link)\b[^>]*(?:src|href)=[\"\']https?://", HTML, re.IGNORECASE) is None, "no external HTML dependencies")
    check(JS.count("setInterval(") == 1, "exactly one permanent frontend interval")

    # i18n and generated web asset integrity.
    de = translation_keys("de", "en")
    en = translation_keys("en", "swg")
    swg = translation_keys("swg", None)
    check(de == en == swg, f"base i18n key parity ({len(de)} keys/language)")
    check("I18N.it = {" in JS and "I18N.fr = {" in JS and "I18N['swg-alb'] = {" in JS and "I18N['swg-ob'] = {" in JS, "all seven UI languages bundled")

    # Existing portable host tests remain release gates.
    guard_binary = ROOT / "tools" / ".test_physical_button_guard"
    subprocess.run(["g++", "-std=c++17", "-I", str(ROOT), str(ROOT / "tools" / "test_physical_button_guard.cpp"), "-o", str(guard_binary)], check=True)
    subprocess.run([str(guard_binary)], check=True)
    guard_binary.unlink(missing_ok=True)

    insights_binary = ROOT / "tools" / ".test_focus_insights"
    subprocess.run(["g++", "-std=c++17", "-I", str(ROOT), str(ROOT / "tools" / "test_focus_insights.cpp"), "-o", str(insights_binary)], check=True)
    subprocess.run([str(insights_binary)], check=True)
    insights_binary.unlink(missing_ok=True)
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
