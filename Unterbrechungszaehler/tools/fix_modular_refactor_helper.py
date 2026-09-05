from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
path = ROOT / "tools" / "apply_modular_hardware_refactor.py"
text = path.read_text(encoding="utf-8")

old = '''old_audio = \'\'\'constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 1000;\nconstexpr uint8_t AUDIO_PROBE_MAX_ATTEMPTS = 2;\nconstexpr uint32_t AUDIO_PROBE_RETRY_DELAY_MS = 120;\nconstexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;\nconstexpr uint32_t AUDIO_INTER_COMMAND_DELAY_MS = 120;\nconstexpr uint32_t AUDIO_BOOT_GRACE_MS = 1200;\nconstexpr bool AUDIO_BOOT_TONE_ENABLED = true;\nconstexpr uint16_t AUDIO_BOOT_TONE_TRACK = 1;\nconstexpr uint32_t AUDIO_BOOT_TONE_DELAY_MS = 350;\nconstexpr uint16_t AUDIO_TEST_TRACK = 1;\n\'\'\'\n'''
new = '''old_audio = \'\'\'constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 1000;\nconstexpr uint8_t AUDIO_PROBE_MAX_ATTEMPTS = 2;\nconstexpr uint32_t AUDIO_PROBE_RETRY_DELAY_MS = 120;\nconstexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;\n// Playback itself is confirmed by the independent BUSY feedback line. This is\n// intentionally separate from UART query timeouts: a lost status reply must not\n// suppress otherwise valid audio. If BUSY never becomes active, resend the\n// track once before reporting a playback warning.\nconstexpr uint32_t AUDIO_PLAY_BUSY_CONFIRM_MS = 450;\nconstexpr uint8_t AUDIO_PLAY_MAX_ATTEMPTS = 2;\nconstexpr uint32_t AUDIO_INTER_COMMAND_DELAY_MS = 120;\nconstexpr uint32_t AUDIO_BOOT_GRACE_MS = 1200;\nconstexpr bool AUDIO_BOOT_TONE_ENABLED = true;\nconstexpr uint16_t AUDIO_BOOT_TONE_TRACK = 1;\nconstexpr uint32_t AUDIO_BOOT_TONE_DELAY_MS = 350;\nconstexpr uint16_t AUDIO_TEST_TRACK = 1;\n\'\'\'\n'''
if old not in text:
    raise SystemExit("old audio helper anchor not found")
text = text.replace(old, new, 1)

old_begin = '''  diag = Diagnostics{};\n  desiredVolumePercent = diag.desiredVolumePercent;\n  if (HardwareConfig::AUDIO_BUSY_PIN >= 0) pinMode(HardwareConfig::AUDIO_BUSY_PIN, INPUT);\n'''
new_begin = '''  // configureVolumePercent() is called before HardwareRegistry::begin(). Keep\n  // that persisted project preference across transport initialization.\n  diag = Diagnostics{};\n  diag.desiredVolumePercent = desiredVolumePercent;\n  diag.lastVolumeStep = moduleVolumeForPercent(desiredVolumePercent);\n  if (HardwareConfig::AUDIO_BUSY_PIN >= 0) pinMode(HardwareConfig::AUDIO_BUSY_PIN, INPUT);\n'''
if old_begin not in text:
    raise SystemExit("generated audio begin anchor not found")
text = text.replace(old_begin, new_begin, 1)

path.write_text(text, encoding="utf-8")
print("refactor helper corrected")
