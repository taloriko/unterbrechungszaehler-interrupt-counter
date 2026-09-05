from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
audio_path = ROOT / "Unterbrechungszaehler" / "audio_dy_sv17f.cpp"
config_path = ROOT / "Unterbrechungszaehler" / "hardware_config.h"

def replace_once(text: str, old: str, new: str, label: str) -> str:
    if text.count(old) != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {text.count(old)}")
    return text.replace(old, new, 1)

config = config_path.read_text(encoding="utf-8")
config = replace_once(config,
    "constexpr uint32_t AUDIO_PLAY_BUSY_CONFIRM_MS = 450;\nconstexpr uint8_t AUDIO_PLAY_MAX_ATTEMPTS = 2;",
    "// BUSY is diagnostic feedback only. Never resend a play command merely because\n// the decoder/amplifier needs longer to assert BUSY; duplicate play commands can\n// themselves restart or delay a track.\nconstexpr uint32_t AUDIO_PLAY_BUSY_CONFIRM_MS = 1200;\nconstexpr uint8_t AUDIO_PLAY_MAX_ATTEMPTS = 1;",
    "audio confirmation policy")
config = replace_once(config,
    "constexpr uint32_t AUDIO_AUTO_PROBE_DELAY_MS = 5000;",
    "// Keep the startup path quiet. UART diagnostics are deliberately delayed and\n// remain lower priority than every real playback request.\nconstexpr uint32_t AUDIO_AUTO_PROBE_DELAY_MS = 15000;",
    "audio auto probe delay")
config_path.write_text(config, encoding="utf-8")

audio = audio_path.read_text(encoding="utf-8")
audio = replace_once(audio,
    "uint8_t volumeRepeatsPending = 0;\nuint32_t volumeNotBeforeMs = 0;\nuint16_t queuedPlayTrack = 0;",
    "uint8_t volumeRepeatsPending = 0;\nuint32_t volumeNotBeforeMs = 0;\nbool volumePrimed = false;  // current desired volume was sent at least once\nuint16_t queuedPlayTrack = 0;",
    "volume primed state")

audio = replace_once(audio,
    "  diag.desiredVolumePercent = desiredVolumePercent;\n  diag.lastVolumeStep = step;\n  ++diag.volumeCommands;",
    "  diag.desiredVolumePercent = desiredVolumePercent;\n  diag.lastVolumeStep = step;\n  volumePrimed = true;\n  ++diag.volumeCommands;",
    "mark volume primed")

audio = replace_once(audio,
    "void serviceQueuedPlay() {\n  if (queuedPlayTrack == 0U || volumeRepeatsPending != 0U || playbackConfirmActive) return;",
    "void serviceQueuedPlay() {\n  // A real playback request has priority over the redundant second volume send.\n  // Only the first volume command for the current setting must precede playback.\n  if (queuedPlayTrack == 0U || !volumePrimed || playbackConfirmActive) return;",
    "queued play priority")

audio = replace_once(audio,
    "  if (bootTonePending && due(now, bootToneEarliestMs) && volumeRepeatsPending == 0U &&\n      !probeActive && !playbackConfirmActive && txReady(now)) {",
    "  if (bootTonePending && due(now, bootToneEarliestMs) && volumePrimed &&\n      queuedPlayTrack == 0U && !probeActive && !playbackConfirmActive && txReady(now)) {",
    "boot tone scheduling")

audio = replace_once(audio,
    "  volumeNotBeforeMs = nextTxAllowedMs;\n  volumeRepeatsPending = HardwareConfig::AUDIO_VOLUME_SEND_REPEATS;",
    "  volumeNotBeforeMs = nextTxAllowedMs;\n  volumePrimed = false;\n  volumeRepeatsPending = HardwareConfig::AUDIO_VOLUME_SEND_REPEATS;",
    "boot volume priming")

audio = replace_once(audio,
    "  // Volume is a command-only setting and is safe while a track is playing. It\n  // is serviced before playback retries so a newly requested 0 % actually\n  // reaches the module instead of waiting for BUSY to become idle.\n  serviceVolume();\n  handlePlaybackConfirmation();\n  serviceQueuedPlay();\n\n  if (probeActive && volumeRepeatsPending == 0U && queuedPlayTrack == 0U) sendCurrentProbeQuery();",
    "  // Priority: first apply the current volume once, then real playback, then\n  // redundant volume refresh and diagnostics. No user sound waits for the second\n  // volume transmission and BUSY never causes a duplicate play command.\n  if (!volumePrimed) serviceVolume();\n  handlePlaybackConfirmation();\n  serviceQueuedPlay();\n  if (queuedPlayTrack == 0U) serviceVolume();\n\n  if (probeActive && volumeRepeatsPending == 0U && queuedPlayTrack == 0U) sendCurrentProbeQuery();",
    "audio update priority")

audio = replace_once(audio,
    "  desiredVolumePercent = percent > 100U ? 100U : percent;\n  diag.desiredVolumePercent = desiredVolumePercent;",
    "  desiredVolumePercent = percent > 100U ? 100U : percent;\n  diag.desiredVolumePercent = desiredVolumePercent;\n  volumePrimed = false;",
    "volume change reprime")

audio = replace_once(audio,
    "bool playTrack(uint16_t trackNumber) {\n  if (!enabled() || !uartReady || trackNumber == 0U) return false;\n  if (probeActive || waitingQuery != QueryKind::None) cancelProbeForPriorityCommand(\"playback\");\n  queuedPlayTrack = trackNumber;\n  return true;\n}",
    "bool playTrack(uint16_t trackNumber) {\n  if (!enabled() || !uartReady || trackNumber == 0U) return false;\n  if (probeActive || waitingQuery != QueryKind::None) cancelProbeForPriorityCommand(\"playback\");\n  // Any explicit project/test playback supersedes the cosmetic boot chime. This\n  // prevents a boot tone from appearing seconds later after an early button press.\n  bootTonePending = false;\n  queuedPlayTrack = trackNumber;\n  return true;\n}",
    "explicit playback suppresses boot tone")

# serviceBootAndBackgroundProbe calls playTrack() for the boot chime. Since the
# public function now suppresses bootTonePending, its existing post-call clear is
# harmless and keeps the intent explicit.

audio_path.write_text(audio, encoding="utf-8")
print("audio startup priority patch applied")
