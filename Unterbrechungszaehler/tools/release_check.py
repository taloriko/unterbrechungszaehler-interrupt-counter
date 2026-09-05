#!/usr/bin/env python3
"""Portable release checks for Unterbrechungszaehler 3.3.0-dev433."""
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
    check('SOFTWARE_VERSION[] = "3.3.0-dev433"' in config, "prototype version 3.3.0-dev433")
    check(
        'AVAILABLE_LANGUAGES_JSON[] = "[\\\"de\\\",\\\"en\\\",\\\"it\\\",\\\"fr\\\",\\\"swg\\\",\\\"swg-alb\\\",\\\"swg-ob\\\"]"' in config,
        "declared UI languages",
    )
    check('FALLBACK_AP_PASSWORD[] = "Unterbrechungszähler"' in config, "password-protected fallback AP")
    check("ota.apWarning" not in JS and "ota.unavailable" not in JS, "obsolete OTA/AP notices removed")
    check("path: 'ota.usedPercent'" in JS and "type: 'meter'" in JS, "OTA storage utilisation meter")
    check("RAW_EVENT_CAPACITY = 100000" in project, "100,000 raw-event capacity")
    check("RAW_RECORD_SIZE = 9" in project, "9-byte raw record")
    check("DAILY_AGGREGATE_CAPACITY = 2300" in project, "daily aggregate retention")
    check("PENDING_EVENT_CAPACITY = 64" in project, "64-event fixed persistence queue")
    source_registry = (ROOT / "source_registry.h").read_text(encoding="utf-8")
    raw_store = (ROOT / "interruption_store.cpp").read_text(encoding="utf-8")
    check("SOURCE_ID_RADIO_FIRST = 6" in source_registry and "SOURCE_ID_RADIO_LAST = 15" in source_registry, "exactly ten RF logical source ids")
    check("V3_HEADER_ENCODE[64]" in raw_store and "V3_HEADER_DECODE[128]" in raw_store, "self-describing mixed v2/v3 raw codec")
    check("out.sourceId" in raw_store and "RAW_RECORD_SIZE = 9" in project, "source id stored without growing raw records")
    check("RF433_SCK_PIN = 14" in hardware and "RF433_MISO_PIN = 32" in hardware and "RF433_MOSI_PIN = 23" in hardware and "RF433_CS_PIN = 25" in hardware and "RF433_GDO0_PIN = 26" in hardware and "RF433_GDO2_PIN = 27" in hardware, "CC1101 pin map")
    check("RF433_SOMFY_FREQUENCY_HZ = 433420000UL" in hardware, "Somfy RTS 433.42 MHz frequency")
    rf_driver = (ROOT / "rf433_cc1101.cpp").read_text(encoding="utf-8")
    source_registry_cpp = (ROOT / "source_registry.cpp").read_text(encoding="utf-8")
    preferences_cpp = (ROOT / "project_preferences.cpp").read_text(encoding="utf-8")
    check("RF_MODE_DEFAULT = ProjectPreferences::RadioMode::Universal433" in project and '"rfmode"' in preferences_cpp, "persistent exclusive RF operating mode")
    check("Protocol::SomfyRts" in rf_driver and "decodeSomfyPayload" in rf_driver and "setOperatingProtocol" in rf_driver, "Somfy RTS normal receive path")
    check("RadioProtocol::SomfyRts" in source_registry_cpp and "uint8_t protocol = 0" in source_registry_cpp and "sizeof(StoredEntry) == 32" in source_registry_cpp, "RF protocol reuses reserved registry byte without growth")
    check("verifyConfiguration" in rf_driver and "configVerified" in (ROOT / "rf433_cc1101.h").read_text(encoding="utf-8"), "CC1101 selected-mode register readback verification")
    check((ROOT / "rf433_cc1101.cpp").exists() and (ROOT / "source_registry.cpp").exists(), "RF receiver and source registry modules")
    check("DISPLAY_ENABLED_DEFAULT = true" in project, "persistent display master default")
    check("DISPLAY_BOOT_SCREEN_MIN_MS = 4000" in hardware, "four-second nonblocking boot screen minimum")
    check("displayEnabled" in JS and "project.displayEnabled" in JS, "display master switch in UI")
    check("DISPLAY_BRIGHTNESS_DEFAULT_PERCENT = 65" in project and "DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT = 5" in project, "display brightness defaults 65/5 percent")
    check("SOUND_VOLUME_DEFAULT_PERCENT = 100" in project, "sound volume default 100 percent")
    check("INTERRUPTION_SOUND_MODE_DEFAULT = ProjectPreferences::SoundMode::Rotate" in project, "rotating sound is the fresh default")
    check("DISPLAY_ROTATION_180_DEFAULT = false" in project and "displayRotation180" in JS, "persistent 180-degree display option")
    check("day-progress" in JS and "project.displayMode.focus" in JS, "five OLED display modes exposed")
    check("normalizeDisplayText" in (ROOT / "display_sh1106.cpp").read_text(encoding="utf-8") and 'append("AE")' in (ROOT / "display_sh1106.cpp").read_text(encoding="utf-8"), "OLED UTF-8 transliteration fallback")
    check("ProjectPreferences::language()" in (ROOT / "display_views.cpp").read_text(encoding="utf-8") and 'prefs.putString(key' in (ROOT / "project_preferences.cpp").read_text(encoding="utf-8"), "OLED language follows persistent UI language")
    check("soundVolume" in JS and "setVolumePercent" in (ROOT / "audio_dy_sv17f.cpp").read_text(encoding="utf-8"), "DY-SV17F volume control")
    check("ota.hint" not in JS and "Export Compiled Binary" not in JS, "obsolete Arduino sketch BIN hint removed")
    check("averageInterval" in JS and "analytics.coveragePartial" in JS, "average-interval heatmap UI")
    interruption_api = (ROOT / "interruption_api.cpp").read_text(encoding="utf-8")
    check("scanIntervalAnalytics" in interruption_api and "elapsedSeconds == current.deltaSeconds" in interruption_api, "retained adjacent-event interval scan")
    check("delay(4000)" not in (ROOT / "display_views.cpp").read_text(encoding="utf-8") and "delay(4000)" not in (ROOT / "display_sh1106.cpp").read_text(encoding="utf-8"), "boot screen has no blocking four-second delay")
    check(re.search(r'\{"di1"[^\n]*13,\s*PullMode::Up,\s*false[^\n]*25,\s*true,', hardware) is not None, "DI1 GPIO13 active-edge interrupt latch")
    check("AUDIO_RX_PIN = 18" in hardware and "AUDIO_TX_PIN = 19" in hardware and "AUDIO_BUSY_PIN = 39" in hardware, "DY-SV17F pin map")
    audio_driver = (ROOT / "audio_dy_sv17f.cpp").read_text(encoding="utf-8")
    hardware_registry = (ROOT / "hardware_registry.cpp").read_text(encoding="utf-8")
    check("AUDIO_MIN_COMMAND_GAP_MS = 120" in hardware and "AUDIO_VOLUME_SEND_REPEATS = 2" in hardware and "HardwareTypes::FeedbackType::ExternalFeedback" in audio_driver, "DY-SV17F paced command path with external BUSY feedback")
    check("case QueryKind::CurrentDevice: return 0x0A;" in audio_driver and "case QueryKind::CurrentTrack: return 0x0D;" in audio_driver, "DY-SV17F full low-priority diagnostics")
    check('#include <driver/rmt_rx.h>' in rf_driver and 'attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN)' not in rf_driver, "CC1101 raw timing uses ESP32 RMT instead of per-edge ISR")
    check("Rf433Cc1101::update();" in hardware_registry and "Rf433Cc1101::update();" not in (ROOT / "rf433_service.cpp").read_text(encoding="utf-8"), "HardwareRegistry exclusively services RF driver")
    check("if (!Rf433Cc1101::begin())" not in (ROOT / "rf433_service.cpp").read_text(encoding="utf-8"), "project RF service does not initialize hardware driver")
    check("HARDWARE_DIAG_LABELS" in JS, "expanded hardware diagnostics translated")
    check(hardware_registry.find("Rf433Cc1101::begin();") < hardware_registry.find("AudioDySv17f::begin();"), "CC1101 SPI initialized before final UART2 pin routing")
    check("rfMode" in JS and "'rf433.mode.somfy'" in JS and "'rf433.mode.universal'" in JS and "rfMode(value)" in JS, "RF mode selector UI")
    check("0x2D0000, 0x130000" in partitions, "LittleFS custom partition")

    de = translation_keys("de", "en")
    en = translation_keys("en", "swg")
    swg = translation_keys("swg", None)
    check(de == en == swg, f"base i18n key parity ({len(de)} keys/language)")
    check(
        "I18N.it = {" in JS
        and "I18N.fr = {" in JS
        and "I18N['swg-alb'] = {" in JS
        and "I18N['swg-ob'] = {" in JS,
        "additional bundled UI languages",
    )
    for language in ("de", "en", "it", "fr", "swg", "swg-alb", "swg-ob"):
        token = f"Object.assign(I18N{'.' + language if '-' not in language else '[' + repr(language) + ']'}, {{"
        check(token in JS, f"3.2.0 UI additions present for {language}")

    positions = [JS.find(f"{{ id: '{name}'") for name in ("device", "wifi", "memory", "time", "hardware", "ota")]
    check(all(position >= 0 for position in positions) and positions == sorted(positions), "device card order")
    check(JS.count("setInterval(") == 1, "exactly one permanent frontend interval")
    check("const weeks = Array.from({ length: 53 }, (_, i) => String(i + 1))" in JS, "calendar-week heatmap headers use numbers only")
    check("Bindings.notify('analytics.monthWeek')" in JS and "Bindings.notify('analytics.hourly')" in JS, "manual heatmap filters trigger targeted rerender")
    check("projectSettings: renderProjectSettings" in JS, "Home project settings card")
    check("rfLearn: renderRfLearn" in JS and "rfSources: renderRfSourceList" in JS, "RF learning on Home and source manager on Device")
    check("rf433.test.somfy_received" in JS and "attempt < 40" in JS, "Somfy test UI and bounded 10-second hardware follow-up")
    check("Rf433Cc1101::startReceiveTest" in (ROOT / "hardware_registry.cpp").read_text(encoding="utf-8") and '"rf433"' in (ROOT / "hardware_registry.cpp").read_text(encoding="utf-8"), "RF receiver integrated into HardwareRegistry with test action")
    check("WiFi.setHostname(AppConfig::FIRMWARE_NAME)" in (ROOT / "wifi_module.cpp").read_text(encoding="utf-8"), "project Wi-Fi hostname")
    check("includeRetainedCounts" in (ROOT / "rf433_api.cpp").read_text(encoding="utf-8") and "compact=1" in JS, "compact RF configuration API avoids raw-ring scans")
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
        "/api/interruptions/sources",
        "/api/interruptions/rf/learn",
        "/api/interruptions/rf/cancel",
        "/api/interruptions/sources/rename",
        "/api/interruptions/sources/unbind",
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
