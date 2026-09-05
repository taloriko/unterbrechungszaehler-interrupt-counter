from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
path = ROOT / "tools" / "apply_modular_hardware_refactor.py"
text = path.read_text(encoding="utf-8")

start_token = '''# ---------------------------------------------------------------------------\n# Release checks: guard the architectural regression we are fixing.\n# ---------------------------------------------------------------------------\n'''
end_token = 'print("Modular hardware refactor applied")\n'
start = text.find(start_token)
end = text.find(end_token, start)
if start < 0 or end < 0:
    raise SystemExit("release-check helper section not found")

replacement = r'''# ---------------------------------------------------------------------------
# Release checks: replace the old audio-specific regression assertion with
# invariants for the modular RMT/BUSY architecture.
# ---------------------------------------------------------------------------
check = read("tools/release_check.py")
old_audio_check = '''    check("AUDIO_PROBE_MAX_ATTEMPTS = 2" in hardware and "RetryProbePlay" in audio_driver and "BUSY confirms active playback" in audio_driver, "DY-SV17F nonblocking query retry and BUSY fallback")
'''
new_arch_checks = '''    check("AUDIO_MIN_COMMAND_GAP_MS = 120" in hardware and "AUDIO_VOLUME_SEND_REPEATS = 2" in hardware and "HardwareTypes::FeedbackType::ExternalFeedback" in audio_driver, "DY-SV17F paced command path with external BUSY feedback")
    check("case QueryKind::CurrentDevice: return 0x0A;" in audio_driver and "case QueryKind::CurrentTrack: return 0x0D;" in audio_driver, "DY-SV17F full low-priority diagnostics")
    check('#include <driver/rmt_rx.h>' in rf_driver and 'attachInterrupt(digitalPinToInterrupt(HardwareConfig::RF433_GDO0_PIN)' not in rf_driver, "CC1101 raw timing uses ESP32 RMT instead of per-edge ISR")
    check("Rf433Cc1101::update();" in hardware_registry and "Rf433Cc1101::update();" not in (ROOT / "rf433_service.cpp").read_text(encoding="utf-8"), "HardwareRegistry exclusively services RF driver")
    check("if (!Rf433Cc1101::begin())" not in (ROOT / "rf433_service.cpp").read_text(encoding="utf-8"), "project RF service does not initialize hardware driver")
    check("HARDWARE_DIAG_LABELS" in JS, "expanded hardware diagnostics translated")
'''
if old_audio_check not in check:
    raise SystemExit("old audio release-check assertion not found")
check = check.replace(old_audio_check, new_arch_checks, 1)
write("tools/release_check.py", check)

'''
text = text[:start] + replacement + end_token + text[end + len(end_token):]
path.write_text(text, encoding="utf-8")
print("release-check helper corrected")
