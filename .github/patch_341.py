from pathlib import Path


def read(path):
    return Path(path).read_text(encoding="utf-8")


def write(path, text):
    Path(path).write_text(text, encoding="utf-8")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)


# Version
path = "Unterbrechungszaehler/config.h"
text = read(path)
text = replace_once(text, 'constexpr char SOFTWARE_VERSION[] = "3.4.0";', 'constexpr char SOFTWARE_VERSION[] = "3.4.1";', "config version")
write(path, text)

# Anti-spam display window: maximum equals the existing physical-button cooldown.
path = "Unterbrechungszaehler/project_config.h"
text = read(path)
text = replace_once(
    text,
    "// Suppressed-button feedback is intentionally short and deterministic.\n// No delay()/random generator is used in the physical-button fast path.\nconstexpr uint32_t DISPLAY_SPAM_FLICKER_MS = 950;\nconstexpr uint32_t DISPLAY_SPAM_FLICKER_FRAME_MS = 85;",
    "// Suppressed-button feedback remains deterministic and nonblocking. The first\n// suppressed press latches the TV effect until the current physical-button\n// cooldown expires; later suppressed presses do not restart the animation.\nconstexpr uint32_t DISPLAY_SPAM_FLICKER_MS = PHYSICAL_BUTTON_COOLDOWN_MS;\nconstexpr uint32_t DISPLAY_SPAM_FLICKER_FRAME_MS = 85;",
    "project spam flicker config",
)
write(path, text)

# Display API carries the remaining cooldown so the visual effect ends with the same window.
path = "Unterbrechungszaehler/display_views.h"
text = read(path)
text = replace_once(text, "void notifySuppressedPhysicalPress();", "void notifySuppressedPhysicalPress(uint32_t cooldownRemainingMs);", "display header spam signature")
write(path, text)

path = "Unterbrechungszaehler/display_views.cpp"
text = read(path)
text = replace_once(text, "uint32_t spamFlickerStartedAtMs = 0;", "uint32_t spamFlickerUntilMs = 0;", "display spam deadline state")
old = """void notifySuppressedPhysicalPress() {
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
}"""
new = """void notifySuppressedPhysicalPress(uint32_t cooldownRemainingMs) {
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
  // The first suppressed press owns the current visual feedback window. Further
  // suppressed presses may replay track 2 but must not restart the OLED effect.
  if (spamFlickerActive || cooldownRemainingMs == 0U) return;
  const uint32_t durationMs = std::min<uint32_t>(cooldownRemainingMs, ProjectConfig::DISPLAY_SPAM_FLICKER_MS);
  spamFlickerActive = true;
  spamFlickerUntilMs = nowMs + durationMs;
  spamFlickerNextFrameMs = nowMs;
  spamFlickerFrame = 0;
}"""
text = replace_once(text, old, new, "display spam notifier")
text = replace_once(
    text,
    "if (static_cast<uint32_t>(nowMs - spamFlickerStartedAtMs) >= ProjectConfig::DISPLAY_SPAM_FLICKER_MS) {",
    "if (due(nowMs, spamFlickerUntilMs)) {",
    "display spam end condition",
)
write(path, text)

# Compute remaining cooldown before feedback; this is only fixed-cost arithmetic.
path = "Unterbrechungszaehler/interruption_service.cpp"
text = read(path)
old = """void handleSuppressedPhysicalPress(uint32_t nowMs) {
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
}"""
new = """void handleSuppressedPhysicalPress(uint32_t nowMs) {
  const uint32_t elapsed = static_cast<uint32_t>(nowMs - physicalButtonGuard.lastAcceptedMs);
  const uint32_t remaining = elapsed < ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS
                                 ? ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS - elapsed
                                 : 0U;

  // Fast local feedback is deliberately first. Neither storage, analytics nor
  // web work is allowed in front of the acknowledgement the user can hear/see.
  if (ProjectPreferences::soundEnabled()) {
    const uint16_t count = AudioDySv17f::musicCount();
    if (count == 0U || count >= ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK) {
      AudioDySv17f::playPriorityFeedbackTrack(ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK);
    }
  }
  DisplayViews::notifySuppressedPhysicalPress(remaining);
  DisplayViews::update(currentSummary);

  SerialLog::infof("BUTTON", "suppressed | reason=anti_spam | elapsed=%lums | remaining=%lums | count=%lu",
                   static_cast<unsigned long>(elapsed), static_cast<unsigned long>(remaining),
                   static_cast<unsigned long>(physicalButtonGuard.suppressedCount));
}"""
text = replace_once(text, old, new, "service spam handler")
write(path, text)

# Web UI: ordinary checks stay short; explicit audio test follows checking state up to 120 seconds.
path = "Unterbrechungszaehler/ui-src/app.js"
text = read(path)
old = """    async hardwareAction(moduleId, actionId) {
      const query = `?id=${encodeURIComponent(moduleId)}&action=${encodeURIComponent(actionId)}`;
      try {
        const data = await this.request(`/api/hardware/action${query}`, { method: 'POST' });
        patchState({ hardware: data.hardware || { checking: false, modules: [] }, status: { ...(data.status || {}), api: 'ok' } });
        this.followHardwareCheck(0);
      } catch (error) {
        console.warn('Hardware action failed:', error);
        try { await this.refreshHardwareState(); } catch (_) {}
        alert(t('hardware.action.failed'));
      }
    },
    async followHardwareCheck(attempt) {
      await new Promise(resolve => setTimeout(resolve, attempt === 0 ? 380 : 300));
      try {
        const data = await this.refreshHardwareState();
        if (data.hardware?.checking && attempt < 4) this.followHardwareCheck(attempt + 1);
      } catch (error) {
        console.warn('Hardware check follow-up failed:', error);
      }
    },"""
new = """    async hardwareAction(moduleId, actionId) {
      const query = `?id=${encodeURIComponent(moduleId)}&action=${encodeURIComponent(actionId)}`;
      try {
        const data = await this.request(`/api/hardware/action${query}`, { method: 'POST' });
        patchState({ hardware: data.hardware || { checking: false, modules: [] }, status: { ...(data.status || {}), api: 'ok' } });
        const isLongAudioTest = moduleId === 'audio' && actionId === 'test';
        this.followHardwareCheck(0, isLongAudioTest ? 240 : 4, isLongAudioTest ? 500 : 300);
      } catch (error) {
        console.warn('Hardware action failed:', error);
        try { await this.refreshHardwareState(); } catch (_) {}
        alert(t('hardware.action.failed'));
      }
    },
    async followHardwareCheck(attempt, maxAttempts = 4, intervalMs = 300) {
      await new Promise(resolve => setTimeout(resolve, attempt === 0 ? 380 : intervalMs));
      try {
        const data = await this.refreshHardwareState();
        if (data.hardware?.checking && attempt < maxAttempts) this.followHardwareCheck(attempt + 1, maxAttempts, intervalMs);
      } catch (error) {
        console.warn('Hardware check follow-up failed:', error);
      }
    },"""
text = replace_once(text, old, new, "frontend hardware follow")
write(path, text)

# README: current version, contradictions, complete 3.x overview and 3.4.1 behavior.
path = "README.md"
text = read(path)
text = replace_once(text, "> **Aktueller Stand:** `3.3.2`", "> **Aktueller Stand:** `3.4.1`", "README current version")
hard_cut = """## 3.0.0 ist ein harter Schnitt

Die bisherigen 1.x/2.x-Stände waren Entwicklungs- und Teststände. **3.0.0 ist der neue Ausgangspunkt.** Es gibt deshalb keine zugesicherte Hardware-, Daten- oder OTA-Migration von 2.x. Wer von einem alten Testaufbau kommt, baut die Verdrahtung nach der aktuellen 3.0.0-Dokumentation neu auf.
"""
version_table = """## 3.0.0 ist ein harter Schnitt

Die bisherigen 1.x/2.x-Stände waren Entwicklungs- und Teststände. **3.0.0 ist der neue Ausgangspunkt.** Es gibt deshalb keine zugesicherte Hardware-, Daten- oder OTA-Migration von 2.x. Wer von einem alten Testaufbau kommt, baut die Verdrahtung nach der aktuellen 3.0.0-Dokumentation neu auf.

## Versionsstand 3.x

| Version | Technische Erweiterung |
|---|---|
| 3.0.0 | Neue modulare ESP32-Baseline mit GPIO-/Web-Erfassung, 100.000 Rohereignissen, 2.300 Tagesaggregaten, Heatmaps, CSV, RTC/OLED/DY-SV17F und OTA. |
| 3.0.1 | Passwortgeschützter Fallback-AP und bereinigte OTA-/AP-Statusdarstellung. |
| 3.1.0 | Heatmap-Metrik Ø Abstand, persistenter Display-Master, nicht blockierender Displaytest und dokumentiertes Soundpaket. |
| 3.2.0 | Persistente OLED-Sprache/Rotation/Helligkeit, fünf Displaymodi, DY-SV17F-Lautstärke und Rotation als Standard. |
| 3.3.0 | Vollständiger Datenbank-Reset, Herkunftsfilter für Heatmaps und konkrete Speicherfehlerdiagnose. |
| 3.3.1 | Getrennte DY-SV17F-UART-/BUSY-Diagnose mit Messzeitpunkten und manuellem End-to-End-Audiotest. |
| 3.3.2 | Audiotest bestätigt das Trackende über gezielte UART-Statusabfragen; BUSY bleibt Zusatzdiagnose. |
| 3.4.0 | 10-s-Anti-Spam für den physischen Knopf, Track 2 reserviert, normale Töne ab Track 3 und OLED-TV-Störfeedback. |
| 3.4.1 | WebUI verfolgt den manuellen Audiotest bis zum Abschluss; die OLED-TV-Störung läuft nach dem ersten verworfenen Druck bis zum Ende der aktiven 10-s-Sperre und wird durch weitere Spam-Drücke nicht neu gestartet. |
"""
text = replace_once(text, hard_cut, version_table, "README version overview")
text = replace_once(
    text,
    "5. `00001` ist **Track 1 und ausschließlich der Boot-Ton**. `00002` und höher sind die Unterbrechungstöne. Im festen Modus spielt die Firmware den ausgewählten Track ab 2; im Rotationsmodus werden die erkannten Tracks **3…N** verwendet.",
    "5. `00001` ist **Track 1 für Boot/Test**, `00002` ist **Track 2 für Anti-Spam**, `00003` und höher sind normale Unterbrechungstöne. Im festen Modus sind normale Tracks ab 3 zulässig; im Rotationsmodus werden ausschließlich die erkannten Tracks **3…N** verwendet.",
    "README track allocation",
)
text = replace_once(
    text,
    "Ab 3.2.0 ist die Lautstärke in der Weboberfläche von **0–100 %** einstellbar; bei einer frischen Konfiguration sind **100 %** voreingestellt. Der Standardmodus für Unterbrechungstöne ist **Wechseln/Rotation** über die erkannten Tracks 2…N. Bereits gespeicherte Einstellungen älterer 3.x-Stände werden nicht überschrieben.",
    "Ab 3.2.0 ist die Lautstärke in der Weboberfläche von **0–100 %** einstellbar; bei einer frischen Konfiguration sind **100 %** voreingestellt. Der Standardmodus für Unterbrechungstöne ist **Wechseln/Rotation**. Seit 3.4.0 ist Track 2 für Anti-Spam reserviert; normale feste und rotierende Unterbrechungstöne beginnen bei Track 3.",
    "README rotation paragraph",
)
text = replace_once(
    text,
    "Damit das trotzdem nicht unbemerkt bleibt, hat der Unsinn sein eigenes Feedback: **Track 2** ist fest als Anti-Spam-Ton reserviert und das OLED zeigt rund eine Sekunde eine nicht blockierende alte-TV-Störung mit „ZU SCHNELL!“. Tonkommando und erster Störframe haben im Fast-Path Priorität vor Logging und Statistikarbeit. Bei ausgeschaltetem Sound/Display wird die jeweilige Benutzerpräferenz respektiert.",
    "Ein verworfener physischer Druck löst **Track 2** als reservierten Anti-Spam-Ton aus. Das OLED startet beim ersten verworfenen Druck der laufenden Sperrphase eine nicht blockierende alte-TV-Störung mit „ZU SCHNELL!“ und hält sie bis zum Ende dieser 10-Sekunden-Sperre aktiv. Weitere verworfene Drücke können Track 2 erneut auslösen, starten die OLED-Animation jedoch nicht neu und verlängern die Sperre nicht. Tonkommando und erster Störframe haben im Fast-Path Priorität vor Logging und Statistikarbeit. Bei ausgeschaltetem Sound/Display wird die jeweilige Benutzerpräferenz respektiert.",
    "README anti-spam animation",
)
text += """

## DY-SV17F-Webdiagnose in 3.4.1

Nach einem manuellen **Ton testen** verfolgt die Weboberfläche den Hardwarestatus ausschließlich für die Dauer dieses expliziten Tests. Die Abfrage erfolgt temporär in 500-ms-Abständen und endet automatisch, sobald die Firmware den Audiotest abgeschlossen hat; als Sicherheitsgrenze gelten 120 Sekunden. Außerhalb eines gestarteten Audiotests entsteht dadurch kein zusätzlicher permanenter Polling-Timer.
"""
write(path, text)

# Changelog and release notes.
path = "CHANGELOG.md"
text = read(path)
entry = """# Changelog

## 3.4.1

- Weboberfläche verfolgt einen ausdrücklich gestarteten DY-SV17F-Audiotest bis zum Firmware-Abschluss; temporär 500 ms, maximal 120 s, kein zusätzliches permanentes Polling
- OLED-TV-Störung startet nur beim ersten verworfenen Druck einer laufenden 10-s-Sperre und endet mit dieser Sperre; weitere verworfene Drücke starten die Animation nicht neu
- Track-2-Fast-Feedback, 10-s-Cooldown und Regel „verworfen verlängert nicht“ bleiben unverändert
- README auf den vollständigen technischen 3.x-Versionsstand und die aktuelle Trackbelegung synchronisiert

"""
text = replace_once(text, "# Changelog\n\n", entry, "CHANGELOG 3.4.1")
write(path, text)

path = "Unterbrechungszaehler/RELEASE_NOTES.md"
text = read(path)
entry = """# Release 3.4.1

- temporäre 500-ms-Web-Nachführung eines manuellen DY-SV17F-Audiotests bis zum Abschluss, maximal 120 s
- keine zusätzliche permanente Frontend-Abfrage außerhalb eines gestarteten Audiotests
- OLED-TV-Störung läuft nach dem ersten verworfenen physischen Druck bis zum Ende der aktiven 10-s-Sperre und wird durch weitere Spam-Drücke nicht neu gestartet
- Track-2-Fast-Path und bestehende Anti-Spam-Zähl-/Speicherlogik bleiben unverändert
- README technisch auf den aktuellen 3.x-Versionsstand synchronisiert

"""
write(path, entry + text)

# Release checks.
path = "Unterbrechungszaehler/tools/release_check.py"
text = read(path)
text = replace_once(text, "Portable release checks for Unterbrechungszaehler 3.4.0.", "Portable release checks for Unterbrechungszaehler 3.4.1.", "release-check docstring")
text = replace_once(text, 'check(\'SOFTWARE_VERSION[] = "3.4.0"\' in config, "project version 3.4.0")', 'check(\'SOFTWARE_VERSION[] = "3.4.1"\' in config, "project version 3.4.1")', "release-check version")
text = replace_once(
    text,
    'check("DISPLAY_SPAM_FLICKER_MS = 950" in project and "renderSpamFlickerFrame" in views_cpp and "delay(" not in views_cpp.split("renderSpamFlickerFrame",1)[1].split("contrastFromPercent",1)[0], "nonblocking deterministic old-TV spam flicker")',
    'check("DISPLAY_SPAM_FLICKER_MS = PHYSICAL_BUTTON_COOLDOWN_MS" in project and "renderSpamFlickerFrame" in views_cpp and "delay(" not in views_cpp.split("renderSpamFlickerFrame",1)[1].split("contrastFromPercent",1)[0], "nonblocking deterministic old-TV spam flicker spans cooldown window")\n    check("notifySuppressedPhysicalPress(uint32_t cooldownRemainingMs)" in views_cpp and "if (spamFlickerActive || cooldownRemainingMs == 0U) return;" in views_cpp and "spamFlickerUntilMs" in views_cpp, "spam OLED effect latches once and is not retriggered")\n    check("DisplayViews::notifySuppressedPhysicalPress(remaining)" in service_cpp, "spam display duration follows remaining physical-button cooldown")\n    check("isLongAudioTest" in JS and "isLongAudioTest ? 240 : 4" in JS and "isLongAudioTest ? 500 : 300" in JS, "manual audio test gets bounded temporary web follow")\n    check("followHardwareCheck(attempt, maxAttempts = 4, intervalMs = 300)" in JS and "attempt < maxAttempts" in JS, "ordinary hardware follow remains short while audio test can run longer")',
    "release-check 3.4.1 behavior",
)
write(path, text)
