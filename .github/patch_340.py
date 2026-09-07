from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Unterbrechungszaehler"


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    (ROOT / rel).write_text(text, encoding="utf-8")


def replace_once(rel, old, new):
    text = read(rel)
    if old not in text:
        raise SystemExit(f"missing anchor in {rel}: {old[:120]!r}")
    write(rel, text.replace(old, new, 1))


def insert_before(rel, marker, addition):
    text = read(rel)
    if addition.strip() in text:
        return
    pos = text.find(marker)
    if pos < 0:
        raise SystemExit(f"missing marker in {rel}: {marker!r}")
    write(rel, text[:pos] + addition + text[pos:])


# ---------------------------------------------------------------------------
# Version + central project policy
# ---------------------------------------------------------------------------
replace_once("Unterbrechungszaehler/config.h", 'SOFTWARE_VERSION[] = "3.3.2"', 'SOFTWARE_VERSION[] = "3.4.0"')
replace_once(
    "Unterbrechungszaehler/project_config.h",
    "constexpr uint16_t INTERRUPTION_SOUND_TRACK_DEFAULT = 2;",
    "constexpr uint32_t PHYSICAL_BUTTON_COOLDOWN_MS = 10000;\n"
    "constexpr uint16_t INTERRUPTION_SPAM_SOUND_TRACK = 2;\n"
    "constexpr uint16_t INTERRUPTION_SOUND_FIRST_NORMAL_TRACK = 3;\n"
    "constexpr uint16_t INTERRUPTION_SOUND_TRACK_DEFAULT = INTERRUPTION_SOUND_FIRST_NORMAL_TRACK;",
)
insert_before(
    "Unterbrechungszaehler/project_config.h",
    "// OLED project preferences are persisted in NVS.",
    "// Suppressed-button feedback is intentionally short and deterministic.\n"
    "// No delay()/random generator is used in the physical-button fast path.\n"
    "constexpr uint32_t DISPLAY_SPAM_FLICKER_MS = 950;\n"
    "constexpr uint32_t DISPLAY_SPAM_FLICKER_FRAME_MS = 85;\n\n",
)

# ---------------------------------------------------------------------------
# Tiny pure guard so the timing rule is host-testable, including millis wrap.
# ---------------------------------------------------------------------------
write(
    "Unterbrechungszaehler/physical_button_guard.h",
    r'''#pragma once

#include <stdint.h>

namespace PhysicalButtonGuard {

struct State {
  bool hasAccepted = false;
  uint32_t lastAcceptedMs = 0;
  bool hasSuppressed = false;
  uint32_t lastSuppressedMs = 0;
  uint32_t suppressedCount = 0;
};

inline bool accept(State &state, uint32_t nowMs, uint32_t cooldownMs) {
  if (!state.hasAccepted || static_cast<uint32_t>(nowMs - state.lastAcceptedMs) >= cooldownMs) {
    state.hasAccepted = true;
    state.lastAcceptedMs = nowMs;
    return true;
  }
  state.hasSuppressed = true;
  state.lastSuppressedMs = nowMs;
  if (state.suppressedCount != UINT32_MAX) ++state.suppressedCount;
  return false;
}

}  // namespace PhysicalButtonGuard
''',
)
write(
    "Unterbrechungszaehler/tools/test_physical_button_guard.cpp",
    r'''#include <cassert>
#include <cstdint>
#include <iostream>

#include "physical_button_guard.h"

int main() {
  constexpr uint32_t cooldown = 10000U;

  PhysicalButtonGuard::State a;
  assert(PhysicalButtonGuard::accept(a, 0U, cooldown));
  assert(!PhysicalButtonGuard::accept(a, 1000U, cooldown));
  assert(!PhysicalButtonGuard::accept(a, 2000U, cooldown));
  assert(!PhysicalButtonGuard::accept(a, 9999U, cooldown));
  assert(a.suppressedCount == 3U);
  assert(PhysicalButtonGuard::accept(a, 10000U, cooldown));

  // A rejected press must not extend the window.
  PhysicalButtonGuard::State b;
  assert(PhysicalButtonGuard::accept(b, 100U, cooldown));
  assert(!PhysicalButtonGuard::accept(b, 9100U, cooldown));
  assert(PhysicalButtonGuard::accept(b, 10100U, cooldown));

  // First press after boot is always accepted, even before 10 seconds uptime.
  PhysicalButtonGuard::State c;
  assert(PhysicalButtonGuard::accept(c, 7U, cooldown));

  // Unsigned subtraction keeps the comparison correct across millis() wrap.
  PhysicalButtonGuard::State d;
  const uint32_t start = 0xFFFFF000U;
  assert(PhysicalButtonGuard::accept(d, start, cooldown));
  assert(!PhysicalButtonGuard::accept(d, static_cast<uint32_t>(start + cooldown - 1U), cooldown));
  assert(PhysicalButtonGuard::accept(d, static_cast<uint32_t>(start + cooldown), cooldown));

  std::cout << "PASS physical button anti-spam guard\n";
  return 0;
}
''',
)

# ---------------------------------------------------------------------------
# Audio: reserve track 2 and add a deliberately tiny priority-feedback path.
# It may supersede only a normal play verification; diagnostics/probes are not
# interrupted. This is what keeps repeated physical-button feedback immediate.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/audio_dy_sv17f.h",
    "bool playTrack(uint16_t trackNumber);",
    "bool playTrack(uint16_t trackNumber);\n"
    "// Immediate local-feedback command. It may replace only a pending normal\n"
    "// play verification; probes/manual diagnostics keep exclusive ownership.\n"
    "bool playPriorityFeedbackTrack(uint16_t trackNumber);",
)
insert_before(
    "Unterbrechungszaehler/audio_dy_sv17f.cpp",
    "bool playTestTone() {",
    r'''bool playPriorityFeedbackTrack(uint16_t trackNumber) {
  if (!HardwareConfig::ENABLE_AUDIO_DY_SV17F || !uartReady || trackNumber == 0U) return false;
  if (probeActive || manualTestActive) return false;
  if (waitingFor != WaitKind::None && waitingFor != WaitKind::VerifyPlay) return false;
  if (deferredAction != DeferredAction::None && deferredAction != DeferredAction::VerifyPlay) return false;

  // A repeated physical press should be audible immediately. Replacing a
  // pending normal play-state verification is safe: its eventual UART answer
  // is ignored while no query is waiting. Diagnostic/probe traffic is never
  // preempted by this fast feedback path.
  if (waitingFor == WaitKind::VerifyPlay) {
    waitingFor = WaitKind::None;
    verifyExpectation = VerifyExpectation::Any;
  }
  if (deferredAction == DeferredAction::VerifyPlay) deferredAction = DeferredAction::None;
  sendPlayCommand(trackNumber);
  return true;
}

''',
)

# ---------------------------------------------------------------------------
# Display: nonblocking deterministic old-TV flicker with optional short text.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/display_views.h",
    "void notifyInterruption(bool flashEnabled);",
    "void notifyInterruption(bool flashEnabled);\nvoid notifySuppressedPhysicalPress();",
)
views_path = "Unterbrechungszaehler/display_views.cpp"
views = read(views_path)
views = views.replace(
    "bool flashRequested = false;\nbool flashActive = false;\nuint32_t flashUntilMs = 0;",
    "bool flashRequested = false;\nbool flashActive = false;\nuint32_t flashUntilMs = 0;\n"
    "bool spamFlickerActive = false;\nuint32_t spamFlickerStartedAtMs = 0;\n"
    "uint32_t spamFlickerNextFrameMs = 0;\nuint8_t spamFlickerFrame = 0;",
    1,
)
views = views.replace(
    "  const char *average;\n};",
    "  const char *average;\n  const char *tooFast;\n};",
    1,
)
views = views.replace(
    'static const Labels de{"HEUTE", "LETZTE", "JETZT", "FOKUS", "SCHNITT"};',
    'static const Labels de{"HEUTE", "LETZTE", "JETZT", "FOKUS", "SCHNITT", "ZU SCHNELL!"};',
    1,
)
views = views.replace(
    'static const Labels en{"TODAY", "LAST", "NOW", "FOCUS", "AVG"};',
    'static const Labels en{"TODAY", "LAST", "NOW", "FOCUS", "AVG", "TOO FAST!"};',
    1,
)
views = views.replace(
    'static const Labels fr{"JOUR", "DERNIER", "MAINT", "FOCUS", "MOY"};',
    'static const Labels fr{"JOUR", "DERNIER", "MAINT", "FOCUS", "MOY", "TROP VITE!"};',
    1,
)
views = views.replace(
    'static const Labels it{"OGGI", "ULTIMA", "ORA", "FOCUS", "MEDIA"};',
    'static const Labels it{"OGGI", "ULTIMA", "ORA", "FOCUS", "MEDIA", "TROPPO PRESTO!"};',
    1,
)
views = views.replace(
    'static const Labels swg{"HEIT", "LETSCHTE", "JETZT", "FOKUS", "SCHNITT"};',
    'static const Labels swg{"HEIT", "LETSCHTE", "JETZT", "FOKUS", "SCHNITT", "NET SO HEKTISCH!"};',
    1,
)
if "tooFast" not in views:
    raise SystemExit("display label patch failed")

render_flicker = r'''bool renderSpamFlickerFrame(uint8_t frame) {
  const uint8_t phase = static_cast<uint8_t>(frame % 8U);
  DisplaySh1106::frameClear();

  // Deterministic scan-line/static frames: old-TV character without RNG,
  // allocations or a large bitmap table.
  for (uint8_t i = 0; i < 6U; ++i) {
    const int16_t y = static_cast<int16_t>((phase * 7U + i * 11U) % 64U);
    const int16_t inset = static_cast<int16_t>((phase + i * 3U) % 15U);
    DisplaySh1106::drawHLine(inset, 127 - inset, y);
  }
  if (phase == 1U || phase == 5U) {
    DisplaySh1106::drawRect(3, 18, 122, 20);
    for (int16_t x = 8; x < 124; x += 13) DisplaySh1106::drawVLine(x, 20, 35);
  }
  if ((phase & 1U) == 0U) DisplaySh1106::drawCenteredText(27, labels().tooFast);

  // Controller inversion adds a short vertical-sync-like flash without
  // rebuilding the framebuffer. Always restored when the effect ends.
  DisplaySh1106::setInverted(phase == 2U || phase == 6U);
  return DisplaySh1106::present();
}

'''
marker = "uint8_t contrastFromPercent(uint8_t percent) {"
pos = views.find(marker)
if pos < 0:
    raise SystemExit("display flicker insert marker missing")
views = views[:pos] + render_flicker + views[pos:]

notify_spam = r'''void notifySuppressedPhysicalPress() {
  const uint32_t nowMs = millis();
  lastActivityMs = nowMs;
  dimmed = false;
  lastContrast = -1;
  flashRequested = false;
  if (flashActive) {
    DisplaySh1106::setInverted(false);
    flashActive = false;
  }
  if (!ProjectPreferences::displayEnabled() || DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) {
    spamFlickerActive = false;
    return;
  }
  spamFlickerActive = true;
  spamFlickerStartedAtMs = nowMs;
  spamFlickerNextFrameMs = nowMs;
  spamFlickerFrame = 0;
}

'''
views = views.replace("void requestHomeRefresh() { renderRequested = true; }", notify_spam + "void requestHomeRefresh() { renderRequested = true; }", 1)
views = views.replace(
    "    flashActive = false;\n    flashRequested = false;\n    if (displayPowerOn && DisplaySh1106::setPower(false)) displayPowerOn = false;",
    "    flashActive = false;\n    flashRequested = false;\n    spamFlickerActive = false;\n    if (displayPowerOn && DisplaySh1106::setPower(false)) displayPowerOn = false;",
    1,
)
update_anchor = "  updateContrast(nowMs);\n\n  if (!renderRequested && !flashRequested"
update_block = r'''  updateContrast(nowMs);

  if (spamFlickerActive) {
    if (static_cast<uint32_t>(nowMs - spamFlickerStartedAtMs) >= ProjectConfig::DISPLAY_SPAM_FLICKER_MS) {
      DisplaySh1106::setInverted(false);
      spamFlickerActive = false;
      renderRequested = true;
    } else {
      if (due(nowMs, spamFlickerNextFrameMs)) {
        renderSpamFlickerFrame(spamFlickerFrame++);
        spamFlickerNextFrameMs = nowMs + ProjectConfig::DISPLAY_SPAM_FLICKER_FRAME_MS;
      }
      return;
    }
  }

  if (!renderRequested && !flashRequested'''
if update_anchor not in views:
    raise SystemExit("display update anchor missing")
views = views.replace(update_anchor, update_block, 1)
write(views_path, views)

# ---------------------------------------------------------------------------
# Interruption service: gate only physical input, before capture. Spam feedback
# is immediate and never touches event/queue/statistics state.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/interruption_service.cpp",
    '#include "interruption_aggregates.h"',
    '#include "interruption_aggregates.h"\n#include "physical_button_guard.h"',
)
service_path = "Unterbrechungszaehler/interruption_service.cpp"
service = read(service_path)
service = service.replace("uint16_t lastRotatingTrack = 1;", "uint16_t lastRotatingTrack = 2;", 1)
service = service.replace(
    "uint32_t nextAggregateRetryMs = 0;",
    "uint32_t nextAggregateRetryMs = 0;\nPhysicalButtonGuard::State physicalButtonGuard;\nbool missingNormalTracksLogged = false;",
    1,
)
old_gpio = '''void onGpioChanged(const char *channelId, bool logicalState) {
  if (!logicalState || !channelId || strcmp(channelId, ProjectConfig::INTERRUPTION_INPUT_ID) != 0) return;
  capture(InterruptionTypes::EventSource::PhysicalButton);
}
'''
new_gpio = r'''void handleSuppressedPhysicalPress(uint32_t nowMs) {
  // Fast local feedback is deliberately first. Neither storage, analytics nor
  // web work is allowed in front of the acknowledgement the user can hear/see.
  if (ProjectPreferences::soundEnabled()) {
    const uint16_t count = AudioDySv17f::musicCount();
    if (count == 0U || count >= ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK) {
      AudioDySv17f::playPriorityFeedbackTrack(ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK);
    }
  }
  DisplayViews::notifySuppressedPhysicalPress();
  DisplayViews::update(currentSummary);

  const uint32_t elapsed = static_cast<uint32_t>(nowMs - physicalButtonGuard.lastAcceptedMs);
  const uint32_t remaining = elapsed < ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS
                                 ? ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS - elapsed
                                 : 0U;
  SerialLog::infof("BUTTON", "suppressed | reason=anti_spam | elapsed=%lums | remaining=%lums | count=%lu",
                   static_cast<unsigned long>(elapsed), static_cast<unsigned long>(remaining),
                   static_cast<unsigned long>(physicalButtonGuard.suppressedCount));
}

void onGpioChanged(const char *channelId, bool logicalState) {
  if (!logicalState || !channelId || strcmp(channelId, ProjectConfig::INTERRUPTION_INPUT_ID) != 0) return;
  const uint32_t nowMs = millis();
  if (!PhysicalButtonGuard::accept(physicalButtonGuard, nowMs, ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS)) {
    handleSuppressedPhysicalPress(nowMs);
    return;
  }
  capture(InterruptionTypes::EventSource::PhysicalButton);
}
'''
if old_gpio not in service:
    raise SystemExit("physical GPIO handler anchor missing")
service = service.replace(old_gpio, new_gpio, 1)

old_sound = '''uint16_t interruptionSoundTrack() {
  if (ProjectPreferences::soundMode() != ProjectPreferences::SoundMode::Rotate) {
    return ProjectPreferences::soundTrack();
  }

  const uint16_t count = AudioDySv17f::musicCount();
  if (count >= 2) {
    // Track 1 belongs exclusively to the boot sound. Rotate deterministically
    // through 2..N so consecutive interruptions never repeat a track when at
    // least two interruption tracks exist. No RNG state or heap is required.
    lastRotatingTrack = (lastRotatingTrack < 2 || lastRotatingTrack >= count)
                          ? 2
                          : static_cast<uint16_t>(lastRotatingTrack + 1U);
    rotateFallbackLogged = false;
    return lastRotatingTrack;
  }

  // The optional track-count query can be unavailable on some modules. Keep
  // feedback functional with the configured fixed fallback rather than
  // blocking/retrying in the interruption path.
  if (!rotateFallbackLogged) {
    SerialLog::warning("INTERRUPT", "Rotating sound requested but track count is unavailable/<2; using configured fixed track");
    rotateFallbackLogged = true;
  }
  return ProjectPreferences::soundTrack();
}
'''
new_sound = r'''uint16_t interruptionSoundTrack() {
  const uint16_t firstNormal = ProjectConfig::INTERRUPTION_SOUND_FIRST_NORMAL_TRACK;
  if (ProjectPreferences::soundMode() != ProjectPreferences::SoundMode::Rotate) {
    return ProjectPreferences::soundTrack() >= firstNormal ? ProjectPreferences::soundTrack() : firstNormal;
  }

  const uint16_t count = AudioDySv17f::musicCount();
  if (count >= firstNormal) {
    // Track 1 = boot/test, track 2 = anti-spam. Normal interruption rotation
    // starts at track 3 and remains deterministic/no-heap.
    lastRotatingTrack = (lastRotatingTrack < firstNormal || lastRotatingTrack >= count)
                          ? firstNormal
                          : static_cast<uint16_t>(lastRotatingTrack + 1U);
    rotateFallbackLogged = false;
    missingNormalTracksLogged = false;
    return lastRotatingTrack;
  }

  if (count > 0U) {
    if (!missingNormalTracksLogged) {
      SerialLog::warning("INTERRUPT", "Only reserved audio tracks detected; normal interruption sound needs track 3 or higher");
      missingNormalTracksLogged = true;
    }
    return 0U;
  }

  // Some modules do not answer the optional count query. Keep the configured
  // >=3 fallback rather than blocking the physical feedback path.
  if (!rotateFallbackLogged) {
    SerialLog::warning("INTERRUPT", "Rotating sound requested but track count is unavailable; using configured track >=3 as fallback");
    rotateFallbackLogged = true;
  }
  return ProjectPreferences::soundTrack() >= firstNormal ? ProjectPreferences::soundTrack() : firstNormal;
}
'''
if old_sound not in service:
    raise SystemExit("rotation block anchor missing")
service = service.replace(old_sound, new_sound, 1)
service = service.replace(
    "    const uint16_t track = interruptionSoundTrack();\n    if (AudioDySv17f::playTrack(track)) --audioPending;",
    "    const uint16_t track = interruptionSoundTrack();\n"
    "    if (track == 0U) --audioPending;\n"
    "    else if (AudioDySv17f::playTrack(track)) --audioPending;",
    1,
)
write(service_path, service)

replace_once(
    "Unterbrechungszaehler/interruption_service.h",
    "bool soundEnabled();",
    "bool soundEnabled();\n"
    "uint32_t physicalButtonCooldownMs();\n"
    "uint32_t suppressedPhysicalPressCount();\n"
    "bool hasSuppressedPhysicalPress();\n"
    "uint32_t lastSuppressedPhysicalPressMs();",
)
service = read(service_path)
service = service.replace(
    "bool soundEnabled() { return ProjectPreferences::soundEnabled(); }\n\n}  // namespace InterruptionService",
    "bool soundEnabled() { return ProjectPreferences::soundEnabled(); }\n"
    "uint32_t physicalButtonCooldownMs() { return ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS; }\n"
    "uint32_t suppressedPhysicalPressCount() { return physicalButtonGuard.suppressedCount; }\n"
    "bool hasSuppressedPhysicalPress() { return physicalButtonGuard.hasSuppressed; }\n"
    "uint32_t lastSuppressedPhysicalPressMs() { return physicalButtonGuard.lastSuppressedMs; }\n\n"
    "}  // namespace InterruptionService",
    1,
)
write(service_path, service)

# ---------------------------------------------------------------------------
# Persisted sound-track migration: fixed normal tracks are >=3 from 3.4.0.
# ---------------------------------------------------------------------------
pref_path = "Unterbrechungszaehler/project_preferences.cpp"
prefs = read(pref_path)
prefs = prefs.replace(
    "  track = prefs.getUShort(\"sndtrack\", ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT);\n  if (track < 2) track = ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT;",
    "  track = prefs.getUShort(\"sndtrack\", ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT);\n"
    "  if (track < ProjectConfig::INTERRUPTION_SOUND_FIRST_NORMAL_TRACK) {\n"
    "    track = ProjectConfig::INTERRUPTION_SOUND_TRACK_DEFAULT;\n"
    "    prefs.putUShort(\"sndtrack\", track);\n"
    "    SerialLog::infof(\"PROJECT\", \"Reserved track migration | normal interruption track=%u\", static_cast<unsigned int>(track));\n"
    "  }",
    1,
)
prefs = prefs.replace(
    "  if (value < 2) return false;  // Track 1 is reserved exclusively for boot.",
    "  if (value < ProjectConfig::INTERRUPTION_SOUND_FIRST_NORMAL_TRACK) return false;  // 1=boot/test, 2=anti-spam.",
    1,
)
write(pref_path, prefs)
replace_once(
    "Unterbrechungszaehler/web_server.cpp",
    'parseUnsignedArg(server.arg("soundTrack"), 2, 65535, value)',
    'parseUnsignedArg(server.arg("soundTrack"), ProjectConfig::INTERRUPTION_SOUND_FIRST_NORMAL_TRACK, 65535, value)',
)

# ---------------------------------------------------------------------------
# GPIO hardware diagnostics for the boot-local suppression counter.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/hardware_registry.cpp",
    '#include "json_utils.h"',
    '#include "json_utils.h"\n#include "interruption_service.h"',
)
hw_path = "Unterbrechungszaehler/hardware_registry.cpp"
hw = read(hw_path)
hw = hw.replace(
    '    appendInfoString(out, first, "hardware.info.outputs", pinList(HardwareConfig::GpioDirection::Output));\n    endModule(out);',
    '    appendInfoString(out, first, "hardware.info.outputs", pinList(HardwareConfig::GpioDirection::Output));\n'
    '    appendInfoString(out, first, "hardware.info.buttonCooldown", String(InterruptionService::physicalButtonCooldownMs() / 1000U) + " s");\n'
    '    appendInfoUInt(out, first, "hardware.info.suppressedPresses", InterruptionService::suppressedPhysicalPressCount());\n'
    '    if (InterruptionService::hasSuppressedPhysicalPress()) {\n'
    '      appendInfoUInt(out, first, "hardware.info.lastSuppressedPress", InterruptionService::lastSuppressedPhysicalPressMs(), "checkTime");\n'
    '    }\n'
    '    endModule(out);',
    1,
)
write(hw_path, hw)

# ---------------------------------------------------------------------------
# Web UI: normal fixed track starts at 3; explain reservations and diagnostics.
# ---------------------------------------------------------------------------
app_path = "Unterbrechungszaehler/ui-src/app.js"
js = read(app_path)
js = js.replace("soundTrack: 2, soundTrackCount", "soundTrack: 3, soundTrackCount", 1)
js = js.replace("addNumber(soundGrid, 'soundTrack', 'project.soundTrack', 2, 65535);", "addNumber(soundGrid, 'soundTrack', 'project.soundTrack', 3, 65535);", 1)

translations = r'''
  const I18N_340 = {
    de: {
      'project.soundTrackHint': 'Track 1 = Boot/Test, Track 2 = Anti-Spam. Normale Unterbrechungstöne beginnen bei Track 3.',
      'project.soundTracksAvailable': 'Verfügbare Tracks: {n}. Wechselnd verwendet nur Track 3 bis {n}.',
      'project.soundTracksUnknown': 'Trackanzahl nicht bekannt; im Wechselmodus dient der feste Track ab 3 als Fallback.',
      'hardware.info.buttonCooldown': 'Anti-Spam', 'hardware.info.suppressedPresses': 'Seit Boot verworfen', 'hardware.info.lastSuppressedPress': 'Letzter verworfener Druck'
    },
    en: {
      'project.soundTrackHint': 'Track 1 = boot/test, track 2 = anti-spam. Normal interruption sounds start at track 3.',
      'project.soundTracksAvailable': 'Available tracks: {n}. Rotate mode uses only tracks 3 through {n}.',
      'project.soundTracksUnknown': 'Track count unknown; rotate mode uses the configured fixed track >=3 as fallback.',
      'hardware.info.buttonCooldown': 'Anti-spam', 'hardware.info.suppressedPresses': 'Suppressed since boot', 'hardware.info.lastSuppressedPress': 'Last suppressed press'
    },
    it: {
      'project.soundTrackHint': 'Traccia 1 = avvio/test, traccia 2 = anti-spam. I normali suoni di interruzione iniziano dalla traccia 3.',
      'project.soundTracksAvailable': 'Tracce disponibili: {n}. La rotazione usa solo le tracce da 3 a {n}.',
      'project.soundTracksUnknown': 'Numero tracce sconosciuto; la rotazione usa come ripiego la traccia fissa >=3.',
      'hardware.info.buttonCooldown': 'Anti-spam', 'hardware.info.suppressedPresses': 'Scartati dal riavvio', 'hardware.info.lastSuppressedPress': 'Ultima pressione scartata'
    },
    fr: {
      'project.soundTrackHint': 'Piste 1 = démarrage/test, piste 2 = anti-spam. Les sons normaux commencent à la piste 3.',
      'project.soundTracksAvailable': 'Pistes disponibles : {n}. La rotation utilise uniquement les pistes 3 à {n}.',
      'project.soundTracksUnknown': 'Nombre de pistes inconnu ; la rotation utilise la piste fixe >=3 comme repli.',
      'hardware.info.buttonCooldown': 'Anti-spam', 'hardware.info.suppressedPresses': 'Rejetés depuis le démarrage', 'hardware.info.lastSuppressedPress': 'Dernier appui rejeté'
    },
    swg: {
      'project.soundTrackHint': 'Track 1 = Boot/Test, Track 2 = Anti-Spam. Normale Unterbrechungstön fanget bei Track 3 a.',
      'project.soundTracksAvailable': 'Verfügbare Tracks: {n}. Wechselnd nimmt bloß Track 3 bis {n}.',
      'project.soundTracksUnknown': 'Trackanzahl net bekannt; wechselnd nimmt dr feste Track ab 3 als Fallback.',
      'hardware.info.buttonCooldown': 'Anti-Spam', 'hardware.info.suppressedPresses': 'Seit Boot verworfa', 'hardware.info.lastSuppressedPress': 'Letschter verworfener Druck'
    },
    'swg-alb': {
      'project.soundTrackHint': 'Track 1 = Boot/Test, Track 2 = Anti-Spam. Normale Tön fanget bei Track 3 a.',
      'project.soundTracksAvailable': 'Tracks do: {n}. Wechselnd nimmt Track 3 bis {n}.',
      'project.soundTracksUnknown': 'Trackanzahl net bekannt; dr feste Track ab 3 isch dr Fallback.',
      'hardware.info.buttonCooldown': 'Anti-Spam', 'hardware.info.suppressedPresses': 'Seit Boot verworfa', 'hardware.info.lastSuppressedPress': 'Letschter verworfener Druck'
    },
    'swg-ob': {
      'project.soundTrackHint': 'Track 1 = Boot/Test, Track 2 = Anti-Spam. Normale Tön fanget bei Track 3 a.',
      'project.soundTracksAvailable': 'Verfügbare Tracks: {n}. Wechselnd nimmt Track 3 bis {n}.',
      'project.soundTracksUnknown': 'Trackanzahl it bekannt; dr feste Track ab 3 isch dr Fallback.',
      'hardware.info.buttonCooldown': 'Anti-Spam', 'hardware.info.suppressedPresses': 'Seit Boot verworfa', 'hardware.info.lastSuppressedPress': 'Letschter verworfener Druck'
    }
  };
  Object.entries(I18N_340).forEach(([code, labels]) => Object.assign(I18N[code], labels));

'''
marker = "  const STORAGE_STATUS_LABELS = {"
pos = js.find(marker)
if pos < 0:
    raise SystemExit("I18N insertion marker missing")
js = js[:pos] + translations + js[pos:]
write(app_path, js)

# ---------------------------------------------------------------------------
# Sound package + concise documentation.
# ---------------------------------------------------------------------------
map_path = "docs/sounds/DATEIZUORDNUNG.txt"
map_text = read(map_path)
map_text = map_text.replace("00002.mp3 - Pling - erwischt", "00002.mp3 - Anti-Spam / zu schnell erneut gedrückt", 1)
write(map_path, map_text)

for rel in ["README.md", "docs/de/README.md", "docs/en/README.md", "docs/swg/README.md", "docs/de/HARDWARE.md", "docs/en/HARDWARE.md"]:
    text = read(rel)
    text = text.replace("Tracks **2…N**", "Tracks **3…N**")
    text = text.replace("Track **2…N**", "Track **3…N**")
    text = text.replace("tracks **2…N**", "tracks **3…N**")
    text = text.replace("tracks **2...N**", "tracks **3...N**")
    text = text.replace("Track 2 bis", "Track 3 bis")
    write(rel, text)

sections = {
    "README.md": r'''
## Anti-Spam am echten Knopf (3.4.0)

Der physische DI1/GPIO-Knopf besitzt ab 3.4.0 eine feste **10-Sekunden-Sperre**: Der erste Druck zählt, weitere physische Drücke innerhalb von weniger als 10 Sekunden werden vollständig aus Rohdaten, Tagesstatistik, CSV, Heatmaps und Ø-Abständen verworfen. Web-Ereignisse bleiben unabhängig. Ein verworfener Druck verlängert die Sperre nicht.

Damit das trotzdem nicht unbemerkt bleibt, hat der Unsinn sein eigenes Feedback: **Track 2** ist fest als Anti-Spam-Ton reserviert und das OLED zeigt rund eine Sekunde eine nicht blockierende alte-TV-Störung mit „ZU SCHNELL!“. Tonkommando und erster Störframe haben im Fast-Path Priorität vor Logging und Statistikarbeit. Bei ausgeschaltetem Sound/Display wird die jeweilige Benutzerpräferenz respektiert.

Trackbelegung: `00001` = Boot/Test, `00002` = Anti-Spam, `00003` und höher = normale Unterbrechungstöne. Der Wechselmodus rotiert entsprechend nur über **3…N**.
''',
    "docs/de/README.md": r'''
## Anti-Spam am physischen Knopf (3.4.0)

Ein gültiger DI1/GPIO-Druck startet eine feste 10-Sekunden-Sperre. Weitere physische Drücke innerhalb dieser Zeit werden nicht gespeichert oder gezählt und beeinflussen weder CSV noch Heatmaps oder Ø-Abstände. Sie lösen ausschließlich das schnelle lokale Anti-Spam-Feedback aus: Track 2 plus etwa eine Sekunde OLED-TV-Flimmern. Web-Ereignisse sind von dieser Sperre unabhängig. Track 1 bleibt Boot/Test, Track 2 ist reserviert, normale Töne beginnen bei Track 3.
''',
    "docs/en/README.md": r'''
## Physical-button anti-spam (3.4.0)

An accepted DI1/GPIO press starts a fixed 10-second window. Further physical presses inside that window are neither stored nor counted and do not affect CSV, heatmaps, or average intervals. They only trigger immediate local anti-spam feedback: track 2 plus roughly one second of non-blocking old-TV OLED flicker. Web events remain independent. Track 1 stays boot/test, track 2 is reserved, and normal interruption sounds start at track 3.
''',
    "docs/swg/README.md": r'''
## Anti-Spam am echte Knopf (3.4.0)

A gültiger DI1/GPIO-Druck macht 10 Sekunda Sperrzeit. Weitere echte Knopfdrück in der Zeit werdet weder gspeichert no gezählt ond mache CSV, Heatmaps ond Ø-Abständ net kaputt. Dafür kommt sofort Track 2 ond s OLED flimmert kurz wia a alter Fernseher mit „NET SO HEKTISCH!“. Web-Klicks send unabhängig. Track 1 bleibt Boot/Test, Track 2 isch Anti-Spam, normale Tön fanget bei Track 3 a.
''',
}
for rel, section in sections.items():
    text = read(rel)
    if "Anti-Spam" not in text or "3.4.0" not in text:
        text += section
    write(rel, text)

# Hardware docs get explicit track allocation.
for rel, section in {
    "docs/de/HARDWARE.md": "\n### Trackbelegung ab 3.4.0\n\n- `00001` – Boot/Test\n- `00002` – Anti-Spam bei erneutem physischen Druck innerhalb 10 Sekunden\n- `00003` und höher – normale Unterbrechungstöne; Rotation nur über 3…N\n",
    "docs/en/HARDWARE.md": "\n### Track allocation from 3.4.0\n\n- `00001` – boot/test\n- `00002` – anti-spam for another physical press within 10 seconds\n- `00003` and above – normal interruption sounds; rotation uses only 3…N\n",
}.items():
    text = read(rel)
    if "Trackbelegung ab 3.4.0" not in text and "Track allocation from 3.4.0" not in text:
        text += section
    write(rel, text)

# Changelog / release notes
chg = read("CHANGELOG.md")
entry = r'''## 3.4.0

- 10-Sekunden-Anti-Spam ausschließlich für den physischen DI1/GPIO-Knopf; verworfene Drücke erreichen weder Raw-Ring noch Tagesaggregate, CSV, Heatmaps oder Ø-Abstände
- verworfene Drücke verlängern die Sperrzeit nicht; Web-Ereignisse bleiben unabhängig
- Track 2 ist fest als Anti-Spam-Ton reserviert, normale Unterbrechungstöne und Rotation beginnen bei Track 3
- schneller Feedbackpfad: Anti-Spam-Tonkommando und erster OLED-Störframe laufen vor Logging/Persistenzarbeit
- nicht blockierendes, deterministisches OLED-Flimmern im Stil eines alten Fernsehers mit kurzem sprachabhängigem Hinweis
- GPIO-Diagnose zeigt 10-s-Sperre, seit Boot verworfene Drücke und den letzten verworfenen Druck

'''
if "## 3.4.0" not in chg:
    chg = chg.replace("# Changelog\n\n", "# Changelog\n\n" + entry, 1)
write("CHANGELOG.md", chg)

rn = read("Unterbrechungszaehler/RELEASE_NOTES.md")
if not rn.startswith("# Release 3.4.0"):
    rn = "# Release 3.4.0\n\n- 10-s-Anti-Spam für DI1/GPIO ohne Einfluss auf gespeicherte Unterbrechungsdaten\n- Track 2 reserviert für Spam-Feedback; normale Rotation ab Track 3\n- sofortiger Track-2-Fast-Path und nicht blockierendes OLED-TV-Flimmern\n- boot-lokaler Diagnosezähler für verworfene physische Drücke\n\n" + rn
write("Unterbrechungszaehler/RELEASE_NOTES.md", rn)

# ---------------------------------------------------------------------------
# Release checks: assert architecture plus execute pure wrap-safe guard tests.
# ---------------------------------------------------------------------------
rc_path = "Unterbrechungszaehler/tools/release_check.py"
rc = read(rc_path)
rc = rc.replace("Unterbrechungszaehler 3.3.2", "Unterbrechungszaehler 3.4.0", 1)
rc = rc.replace('SOFTWARE_VERSION[] = "3.3.2"', 'SOFTWARE_VERSION[] = "3.4.0"', 1)
rc = rc.replace('"project version 3.3.2"', '"project version 3.4.0"', 1)
rc = rc.replace('f"3.3.2 UI additions present for {language}"', 'f"3.4.0 UI additions present for {language}"', 1)
rc = rc.replace(
    'check("Track 1 belongs exclusively to the boot sound" in (ROOT / "interruption_service.cpp").read_text(encoding="utf-8"), "boot track excluded from rotating interruption sound")',
    'service_cpp = (ROOT / "interruption_service.cpp").read_text(encoding="utf-8")\n'
    '    check("Track 1 = boot/test, track 2 = anti-spam" in service_cpp, "tracks 1/2 reserved from normal rotation")\n'
    '    check("INTERRUPTION_SOUND_FIRST_NORMAL_TRACK = 3" in project and "INTERRUPTION_SPAM_SOUND_TRACK = 2" in project, "sound track reservation 1 boot, 2 anti-spam, 3+ normal")\n'
    '    check("PHYSICAL_BUTTON_COOLDOWN_MS = 10000" in project, "10-second physical-button cooldown")\n'
    '    check("PhysicalButtonGuard::accept" in service_cpp and "handleSuppressedPhysicalPress" in service_cpp, "physical button anti-spam guard before capture")\n'
    '    guard_pos = service_cpp.find("PhysicalButtonGuard::accept")\n'
    '    capture_pos = service_cpp.find("capture(InterruptionTypes::EventSource::PhysicalButton)", guard_pos)\n'
    '    check(guard_pos >= 0 and capture_pos > guard_pos and "return;" in service_cpp[guard_pos:capture_pos], "suppressed physical press exits before capture/store path")\n'
    '    check("playPriorityFeedbackTrack(ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK)" in service_cpp, "track 2 fast feedback on suppressed press")\n'
    '    check("notifySuppressedPhysicalPress" in service_cpp and "DisplayViews::update(currentSummary)" in service_cpp, "first spam display frame serviced immediately")\n'
    '    views_cpp = (ROOT / "display_views.cpp").read_text(encoding="utf-8")\n'
    '    check("DISPLAY_SPAM_FLICKER_MS = 950" in project and "renderSpamFlickerFrame" in views_cpp and "delay(" not in views_cpp.split("renderSpamFlickerFrame",1)[1].split("contrastFromPercent",1)[0], "nonblocking deterministic old-TV spam flicker")\n'
    '    prefs_cpp = (ROOT / "project_preferences.cpp").read_text(encoding="utf-8")\n'
    '    check("value < ProjectConfig::INTERRUPTION_SOUND_FIRST_NORMAL_TRACK" in prefs_cpp and "prefs.putUShort(\\\"sndtrack\\\", track)" in prefs_cpp, "legacy fixed track 2 migrates to normal track 3")\n'
    '    check("addNumber(soundGrid, \'soundTrack\', \'project.soundTrack\', 3, 65535)" in JS, "fixed-track UI starts at track 3")\n'
    '    check("hardware.info.suppressedPresses" in JS and "suppressedPhysicalPressCount" in (ROOT / "hardware_registry.cpp").read_text(encoding="utf-8"), "boot-local suppression diagnostics")',
    1,
)
# Add host compilation/execution before Python storage tests.
anchor = '    subprocess.run([sys.executable, str(ROOT / "tools" / "test_interruption_storage.py")], check=True)\n'
extra = '''    guard_binary = ROOT / "tools" / ".test_physical_button_guard"\n    subprocess.run(["g++", "-std=c++17", "-I", str(ROOT), str(ROOT / "tools" / "test_physical_button_guard.cpp"), "-o", str(guard_binary)], check=True)\n    subprocess.run([str(guard_binary)], check=True)\n    guard_binary.unlink(missing_ok=True)\n'''
if anchor not in rc:
    raise SystemExit("release check test anchor missing")
rc = rc.replace(anchor, extra + anchor, 1)
write(rc_path, rc)

print("3.4.0 anti-spam implementation applied")
