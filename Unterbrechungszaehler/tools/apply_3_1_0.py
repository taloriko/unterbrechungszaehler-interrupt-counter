#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent


def read(path: str) -> str:
    return (REPO / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (REPO / path).write_text(text, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    if old not in text:
        raise AssertionError(f"marker missing in {path}: {old[:100]!r}")
    if text.count(old) != 1:
        raise AssertionError(f"marker not unique in {path}: {old[:100]!r}")
    write(path, text.replace(old, new, 1))


def regex_once(path: str, pattern: str, replacement: str, flags: int = 0) -> None:
    text = read(path)
    new, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise AssertionError(f"regex count {count} in {path}: {pattern[:100]!r}")
    write(path, new)


def block_bounds(text: str, start_token: str, end_token: str) -> tuple[int, int]:
    start = text.find(start_token)
    if start < 0:
        raise AssertionError(f"translation block start missing: {start_token}")
    end = text.find(end_token, start + len(start_token))
    if end < 0:
        raise AssertionError(f"translation block end missing: {start_token}")
    return start, end


def js_escape(value: str, quote: str) -> str:
    value = value.replace("\\", "\\\\").replace("\n", "\\n")
    return value.replace(quote, "\\" + quote)


def set_translation(text: str, start_token: str, end_token: str, key: str, value: str) -> str:
    start, end = block_bounds(text, start_token, end_token)
    segment = text[start:end]
    positions = []
    for quoted in (f"'{key}'", f'"{key}"'):
        pos = segment.find(quoted)
        if pos >= 0:
            positions.append((pos, quoted))
    if len(positions) != 1:
        raise AssertionError(f"translation key {key!r} not unique in {start_token}: {len(positions)}")
    pos, quoted = positions[0]
    absolute = start + pos + len(quoted)
    colon = text.find(":", absolute, end)
    if colon < 0:
        raise AssertionError(f"translation colon missing: {key}")
    value_start = colon + 1
    while value_start < end and text[value_start].isspace():
        value_start += 1
    quote = text[value_start]
    if quote not in "'\"":
        raise AssertionError(f"translation value is not quoted: {key}")
    i = value_start + 1
    while i < end:
        if text[i] == "\\":
            i += 2
            continue
        if text[i] == quote:
            value_end = i + 1
            break
        i += 1
    else:
        raise AssertionError(f"translation value end missing: {key}")
    return text[:value_start] + quote + js_escape(value, quote) + quote + text[value_end:]


def inject_translation_keys(text: str, start_token: str, mapping: dict[str, str]) -> str:
    start = text.find(start_token)
    if start < 0:
        raise AssertionError(f"translation block missing: {start_token}")
    line_end = text.find("\n", start)
    if line_end < 0:
        raise AssertionError(f"translation block line missing: {start_token}")
    for key in mapping:
        if text.find(f"'{key}'", start, line_end + 20000) >= 0 or text.find(f'"{key}"', start, line_end + 20000) >= 0:
            raise AssertionError(f"new translation key already present: {key}")
    indent = "      " if start_token.startswith("    ") else "    "
    payload = "".join(
        f"{indent}'{key}': '{js_escape(value, chr(39))}',\n" for key, value in mapping.items()
    )
    return text[: line_end + 1] + payload + text[line_end + 1 :]


# ---------------------------------------------------------------------------
# Version and persistent settings
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/config.h",
    'constexpr char SOFTWARE_VERSION[] = "3.0.1";',
    'constexpr char SOFTWARE_VERSION[] = "3.1.0";',
)

replace_once(
    "Unterbrechungszaehler/project_config.h",
    "constexpr bool INTERRUPTION_SOUND_DEFAULT = true;\nconstexpr bool DISPLAY_FLASH_DEFAULT = true;",
    "constexpr bool INTERRUPTION_SOUND_DEFAULT = true;\nconstexpr bool DISPLAY_ENABLED_DEFAULT = true;\nconstexpr bool DISPLAY_FLASH_DEFAULT = true;",
)

replace_once(
    "Unterbrechungszaehler/hardware_config.h",
    "constexpr bool DISPLAY_BOOT_SCREEN_ENABLED = true;\n",
    "constexpr bool DISPLAY_BOOT_SCREEN_ENABLED = true;\nconstexpr uint32_t DISPLAY_BOOT_SCREEN_MIN_MS = 2000;\nconstexpr uint32_t DISPLAY_TEST_SCREEN_MS = 1500;\n",
)

replace_once(
    "Unterbrechungszaehler/project_preferences.h",
    "bool displayFlashEnabled();\nbool setDisplayFlashEnabled(bool enabled);",
    "bool displayEnabled();\nbool setDisplayEnabled(bool enabled);\nbool displayFlashEnabled();\nbool setDisplayFlashEnabled(bool enabled);",
)

replace_once(
    "Unterbrechungszaehler/project_preferences.cpp",
    "SoundMode soundModeValue = ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;\nbool flash = ProjectConfig::DISPLAY_FLASH_DEFAULT;",
    "SoundMode soundModeValue = ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT;\nbool displayEnabledValue = ProjectConfig::DISPLAY_ENABLED_DEFAULT;\nbool flash = ProjectConfig::DISPLAY_FLASH_DEFAULT;",
)
replace_once(
    "Unterbrechungszaehler/project_preferences.cpp",
    "  soundModeValue = sanitizedSoundMode(prefs.getUChar(\"sndmode\", static_cast<uint8_t>(ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT)));\n  flash = prefs.getBool(\"dispflash\", ProjectConfig::DISPLAY_FLASH_DEFAULT);",
    "  soundModeValue = sanitizedSoundMode(prefs.getUChar(\"sndmode\", static_cast<uint8_t>(ProjectConfig::INTERRUPTION_SOUND_MODE_DEFAULT)));\n  displayEnabledValue = prefs.getBool(\"dispen\", ProjectConfig::DISPLAY_ENABLED_DEFAULT);\n  flash = prefs.getBool(\"dispflash\", ProjectConfig::DISPLAY_FLASH_DEFAULT);",
)
replace_once(
    "Unterbrechungszaehler/project_preferences.cpp",
    '  SerialLog::infof("PROJECT", "Feedback settings | sound=%s | sound-mode=%s | track=%u | display-flash=%s | display-mode=%s",\n                   sound ? "ON" : "OFF", soundModeName(), static_cast<unsigned int>(track),\n                   flash ? "ON" : "OFF", displayModeName());',
    '  SerialLog::infof("PROJECT", "Feedback settings | sound=%s | sound-mode=%s | track=%u | display=%s | display-flash=%s | display-mode=%s",\n                   sound ? "ON" : "OFF", soundModeName(), static_cast<unsigned int>(track),\n                   displayEnabledValue ? "ON" : "OFF", flash ? "ON" : "OFF", displayModeName());',
)
replace_once(
    "Unterbrechungszaehler/project_preferences.cpp",
    "bool displayFlashEnabled() { return flash; }\n",
    "bool displayEnabled() { return displayEnabledValue; }\n\nbool setDisplayEnabled(bool enabled) {\n  if (displayEnabledValue == enabled) return true;\n  if (!persistBool(\"dispen\", enabled)) return false;\n  displayEnabledValue = enabled;\n  SerialLog::infof(\"PROJECT\", \"Display master switch changed | %s\", displayEnabledValue ? \"ON\" : \"OFF\");\n  return true;\n}\n\nbool displayFlashEnabled() { return flash; }\n",
)

# ---------------------------------------------------------------------------
# SH1106 boot/test visibility windows
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/display_sh1106.h",
    "bool showBootScreen();\nbool showTestScreen();",
    "bool showBootScreen();\nbool showTestScreen();\nbool bootScreenActive();\nbool manualTestActive();",
)
replace_once(
    "Unterbrechungszaehler/display_sh1106.cpp",
    'uint32_t checkedAtMs = 0;\nconst char *errorText = "";\n',
    'uint32_t checkedAtMs = 0;\nconst char *errorText = "";\nuint32_t bootScreenUntilMs = 0;\nuint32_t manualTestUntilMs = 0;\n\nbool deadlinePending(uint32_t deadlineMs) {\n  return deadlineMs != 0U && static_cast<int32_t>(millis() - deadlineMs) < 0;\n}\n',
)
replace_once(
    "Unterbrechungszaehler/display_sh1106.cpp",
    "HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::TransportAck; }\n",
    "HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::TransportAck; }\nbool bootScreenActive() { return deadlinePending(bootScreenUntilMs); }\nbool manualTestActive() { return deadlinePending(manualTestUntilMs); }\n",
)
regex_once(
    "Unterbrechungszaehler/display_sh1106.cpp",
    r"bool showBootScreen\(\) \{.*?\n\}\n\nvoid frameClear\(\)",
    '''bool showBootScreen() {
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106 || !isDetected) return false;
  if (!setPower(true)) return false;
  clearFrame();
  drawChipIcon(52, 1);
  centeredText(28, AppConfig::PROJECT_NAME);
  centeredText(40, AppConfig::SOFTWARE_VERSION);
  centeredText(52, "STARTING");
  const bool ok = flushFrame();
  if (ok) {
    bootScreenUntilMs = millis() + HardwareConfig::DISPLAY_BOOT_SCREEN_MIN_MS;
    SerialLog::successf("DISPLAY", "Boot screen shown | minimum=%lu ms",
                        static_cast<unsigned long>(HardwareConfig::DISPLAY_BOOT_SCREEN_MIN_MS));
  }
  return ok;
}

void frameClear()''',
    re.DOTALL,
)
regex_once(
    "Unterbrechungszaehler/display_sh1106.cpp",
    r"bool showTestScreen\(\) \{.*?\n\}\n\n\}  // namespace DisplaySh1106",
    '''bool showTestScreen() {
  if (!HardwareConfig::ENABLE_DISPLAY_SH1106 || !isDetected) return false;
  if (!setPower(true)) return false;
  clearFrame();
  rect(0, 0, HardwareConfig::DISPLAY_WIDTH, HardwareConfig::DISPLAY_HEIGHT);
  centeredText(9, "DISPLAY TEST");
  hLine(8, 119, 20);
  centeredText(27, "SH1106");
  centeredText(39, "128X64");
  hLine(8, 119, 50);
  for (int16_t x = 10; x <= 116; x += 8) {
    pixel(x, 56);
    pixel(x + 1, 57);
    pixel(x + 2, 58);
  }
  const bool ok = flushFrame();
  if (ok) {
    manualTestUntilMs = millis() + HardwareConfig::DISPLAY_TEST_SCREEN_MS;
    SerialLog::success("DISPLAY", "Display test screen shown");
  }
  return ok;
}

}  // namespace DisplaySh1106''',
    re.DOTALL,
)

# ---------------------------------------------------------------------------
# Project display master switch without blocking the boot screen
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/display_views.cpp",
    "bool dimmed = false;\n",
    "bool dimmed = false;\nbool displayPowerOn = true;\n",
)
regex_once(
    "Unterbrechungszaehler/display_views.cpp",
    r"void begin\(const InterruptionTypes::Summary &summary\) \{.*?\n\}\n\nvoid notifyInterruption",
    '''void begin(const InterruptionTypes::Summary &summary) {
  lastActivityMs = millis();
  lastMode = ProjectPreferences::displayMode();
  displayPowerOn = true;  // Hardware boot/init leaves the panel powered.
  renderRequested = true;
  update(summary);
}

void notifyInterruption''',
    re.DOTALL,
)
regex_once(
    "Unterbrechungszaehler/display_views.cpp",
    r"void notifyInterruption\(bool flashEnabled\) \{.*?\n\}\n\nvoid requestHomeRefresh",
    '''void notifyInterruption(bool flashEnabled) {
  const uint32_t nowMs = millis();
  lastActivityMs = nowMs;
  dimmed = false;
  renderRequested = true;
  lastContrast = -1;

  // A disabled project display never wakes for an interruption. During the
  // mandatory boot screen window the event is captured immediately, but the
  // boot image stays visible and the updated Home view appears afterwards.
  if (!ProjectPreferences::displayEnabled() || DisplaySh1106::bootScreenActive()) {
    flashRequested = false;
    return;
  }
  flashRequested = flashEnabled;
}

void requestHomeRefresh''',
    re.DOTALL,
)
regex_once(
    "Unterbrechungszaehler/display_views.cpp",
    r"void settingsChanged\(\) \{.*?\n\}\n\nvoid update\(const InterruptionTypes::Summary &summary\) \{.*?\n\}\n\n\}  // namespace DisplayViews",
    '''void settingsChanged() {
  lastActivityMs = millis();
  dimmed = false;
  lastContrast = -1;
  renderRequested = true;

  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  if (DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) return;

  if (!ProjectPreferences::displayEnabled()) {
    if (flashActive) DisplaySh1106::setInverted(false);
    flashActive = false;
    flashRequested = false;
    if (DisplaySh1106::setPower(false)) displayPowerOn = false;
    return;
  }

  if (DisplaySh1106::setPower(true)) displayPowerOn = true;
}

void update(const InterruptionTypes::Summary &summary) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  const uint32_t nowMs = millis();

  // Neither normal rendering nor a user preference may overwrite the boot
  // image before its minimum visibility time has elapsed. A manual display
  // test gets the same short ownership window and then returns to the user's
  // persistent display preference.
  if (DisplaySh1106::bootScreenActive()) {
    displayPowerOn = true;
    return;
  }
  if (DisplaySh1106::manualTestActive()) {
    displayPowerOn = true;
    return;
  }

  if (!ProjectPreferences::displayEnabled()) {
    if (flashActive) DisplaySh1106::setInverted(false);
    flashActive = false;
    flashRequested = false;
    if (displayPowerOn && DisplaySh1106::setPower(false)) displayPowerOn = false;
    return;
  }

  if (!displayPowerOn) {
    if (!DisplaySh1106::setPower(true)) return;
    displayPowerOn = true;
    lastContrast = -1;
    renderRequested = true;
  }

  updateContrast(nowMs);

  // The project loop runs every few milliseconds, but the idle OLED view does
  // not need that cadence. Immediate/transient rendering remains responsive;
  // ordinary age/Wi-Fi/time/dimmer evaluation is at most once per second.
  if (!renderRequested && !flashRequested && !flashActive &&
      static_cast<uint32_t>(nowMs - lastIdleEvaluationMs) < 1000U) return;
  if (flashActive && !due(nowMs, flashUntilMs)) return;
  lastIdleEvaluationMs = nowMs;

  if (flashActive && due(nowMs, flashUntilMs)) {
    DisplaySh1106::setInverted(false);
    flashActive = false;
    renderRequested = true;
  }

  char age[16]; ageText(summary, age);
  const bool wifi = WifiModule::stationConnected();
  const bool timeOk = TimeService::now().valid;
  const auto mode = ProjectPreferences::displayMode();
  const bool changed = summary.todayCount != lastCount || wifi != lastWifi || timeOk != lastTimeOk ||
                       strcmp(age, lastAge) != 0 || mode != lastMode;
  if (!renderRequested && !changed) return;

  if (renderHome(summary, age, wifi, timeOk)) {
    lastCount = summary.todayCount;
    lastWifi = wifi;
    lastTimeOk = timeOk;
    lastMode = mode;
    strncpy(lastAge, age, sizeof(lastAge) - 1);
    lastAge[sizeof(lastAge) - 1] = '\\0';
    renderRequested = false;
    if (flashRequested) {
      flashRequested = false;
      if (DisplaySh1106::setInverted(true)) {
        flashActive = true;
        flashUntilMs = nowMs + ProjectConfig::DISPLAY_FLASH_MS;
      }
    }
  }
}

}  // namespace DisplayViews''',
    re.DOTALL,
)

# ---------------------------------------------------------------------------
# Preferences API
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/interruption_api.cpp",
    "  fieldUInt(out, \"soundTrackCount\", AudioDySv17f::musicCount());\n  fieldBool(out, \"displayFlashEnabled\", ProjectPreferences::displayFlashEnabled());",
    "  fieldUInt(out, \"soundTrackCount\", AudioDySv17f::musicCount());\n  fieldBool(out, \"displayEnabled\", ProjectPreferences::displayEnabled());\n  fieldBool(out, \"displayFlashEnabled\", ProjectPreferences::displayFlashEnabled());",
)
replace_once(
    "Unterbrechungszaehler/web_server.cpp",
    "  const bool hasSoundMode = server.hasArg(\"soundMode\");\n  const bool hasFlash = server.hasArg(\"displayFlashEnabled\");",
    "  const bool hasSoundMode = server.hasArg(\"soundMode\");\n  const bool hasDisplayEnabled = server.hasArg(\"displayEnabled\");\n  const bool hasFlash = server.hasArg(\"displayFlashEnabled\");",
)
replace_once(
    "Unterbrechungszaehler/web_server.cpp",
    "                         static_cast<uint8_t>(hasSoundMode) + static_cast<uint8_t>(hasFlash) + static_cast<uint8_t>(hasMode) +",
    "                         static_cast<uint8_t>(hasSoundMode) + static_cast<uint8_t>(hasDisplayEnabled) +\n                         static_cast<uint8_t>(hasFlash) + static_cast<uint8_t>(hasMode) +",
)
replace_once(
    "Unterbrechungszaehler/web_server.cpp",
    "    ok = ProjectPreferences::setSoundMode(value);\n  } else if (hasFlash) {",
    "    ok = ProjectPreferences::setSoundMode(value);\n  } else if (hasDisplayEnabled) {\n    bool value = false;\n    if (!parseBoolArg(server.arg(\"displayEnabled\"), value)) {\n      server.send(400, \"application/json; charset=utf-8\", \"{\\\"ok\\\":false,\\\"error\\\":\\\"invalid_display_enabled\\\"}\");\n      return;\n    }\n    ok = ProjectPreferences::setDisplayEnabled(value);\n    displayChanged = ok;\n  } else if (hasFlash) {",
)

# ---------------------------------------------------------------------------
# Average interval analytics. The interval ending at current belongs to the
# previous raw event; requiring both retained adjacent records automatically
# excludes the final press of every day.
# ---------------------------------------------------------------------------
replace_once(
    "Unterbrechungszaehler/interruption_api.h",
    "String buildAnalyticsBundleJson(const char *hourlyMode,\n",
    "String buildAnalyticsBundleJson(const char *metric,\n                                const char *hourlyMode,\n",
)

old_struct = '''struct AnalyticsBundleContext {
  bool hourlyWeekMode = false;
  uint16_t hourlyYear = 0;
  uint8_t hourlyWeek = 0;
  uint16_t hourlyFrom = 0;
  uint16_t hourlyTo = 0;
  uint16_t monthWeekYear = 0;
  uint16_t yearMonthStart = 0;
  uint16_t yearMonthEnd = 0;
  uint32_t hourly[7U * 24U]{};
  uint32_t monthWeek[12U * 53U]{};
  uint32_t yearMonth[5U * 12U]{};
  uint16_t scanCounter = 0;
};'''
new_struct = '''struct AnalyticsBundleContext {
  bool averageInterval = false;
  bool hourlyWeekMode = false;
  uint16_t hourlyYear = 0;
  uint8_t hourlyWeek = 0;
  uint16_t hourlyFrom = 0;
  uint16_t hourlyTo = 0;
  uint16_t monthWeekYear = 0;
  uint16_t yearMonthStart = 0;
  uint16_t yearMonthEnd = 0;
  uint32_t hourly[7U * 24U]{};
  uint32_t monthWeek[12U * 53U]{};
  uint32_t yearMonth[5U * 12U]{};
  uint64_t hourlyIntervalSum[7U * 24U]{};
  uint32_t hourlyIntervalSamples[7U * 24U]{};
  uint64_t monthWeekIntervalSum[12U * 53U]{};
  uint32_t monthWeekIntervalSamples[12U * 53U]{};
  uint64_t yearMonthIntervalSum[5U * 12U]{};
  uint32_t yearMonthIntervalSamples[5U * 12U]{};
  bool oldestRawAbsoluteValid = false;
  uint16_t oldestRawDayIndex = 0;
  uint16_t newestRawDayIndex = 0;
  uint32_t oldestRawEpochSeconds = 0;
  uint32_t newestRawEpochSeconds = 0;
  uint16_t scanCounter = 0;
};'''
replace_once("Unterbrechungszaehler/interruption_api.cpp", old_struct, new_struct)

interval_helpers = r'''
void appendIntervalValues(String &out, const uint64_t *sums, const uint32_t *samples, size_t count) {
  out += '[';
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) out += ',';
    const uint32_t average = samples[i] == 0
                                 ? 0
                                 : static_cast<uint32_t>((sums[i] + samples[i] / 2U) / samples[i]);
    JsonUtils::appendUInt(out, average);
  }
  out += ']';
}

bool isoWeekStartDayIndex(uint16_t year, uint8_t week, uint16_t &dayIndexOut) {
  uint16_t jan4 = 0;
  if (!ProjectTime::dateToDayIndex(year, 1, 4, jan4)) return false;
  const int32_t weekOneMonday = static_cast<int32_t>(jan4) - ProjectTime::weekdayFromDayIndex(jan4);
  const int32_t selected = weekOneMonday + static_cast<int32_t>(week - 1U) * 7;
  if (selected < 0 || selected > 65535) return false;
  dayIndexOut = static_cast<uint16_t>(selected);
  return true;
}

bool intervalCoverageComplete(const AnalyticsBundleContext &ctx, uint16_t selectedStartDayIndex) {
  const uint32_t rawCount = InterruptionStore::count();
  const uint32_t rawCapacity = InterruptionStore::capacity();
  const bool oldestWasOverwritten = rawCapacity > 0U && rawCount >= rawCapacity && InterruptionStore::oldestSequence() > 1U;
  if (!oldestWasOverwritten) return true;
  return ctx.oldestRawAbsoluteValid && ctx.oldestRawDayIndex <= selectedStartDayIndex;
}

void appendIntervalCoverage(String &out, const AnalyticsBundleContext &ctx, bool complete) {
  out += '{';
  fieldBool(out, "complete", complete);
  fieldUInt(out, "retainedRawCount", InterruptionStore::count());
  if (ctx.oldestRawAbsoluteValid) {
    fieldUInt(out, "oldestEpochSeconds", ctx.oldestRawEpochSeconds);
    fieldUInt(out, "newestEpochSeconds", ctx.newestRawEpochSeconds, false);
  } else {
    removeTrailingComma(out);
  }
  out += '}';
}

void addIntervalSample(AnalyticsBundleContext &ctx,
                       const ProjectTime::LocalDateTime &start,
                       uint32_t elapsedSeconds) {
  bool includeHourly = false;
  if (ctx.hourlyWeekMode) {
    includeHourly = start.isoYear == ctx.hourlyYear && start.isoWeek == ctx.hourlyWeek;
  } else {
    includeHourly = start.dayIndex >= ctx.hourlyFrom && start.dayIndex <= ctx.hourlyTo;
  }
  if (includeHourly) {
    const size_t index = static_cast<size_t>(start.weekday) * 24U + start.hour;
    ctx.hourlyIntervalSum[index] += elapsedSeconds;
    ++ctx.hourlyIntervalSamples[index];
  }

  if (start.year == ctx.monthWeekYear && start.month >= 1U && start.month <= 12U &&
      start.isoWeek >= 1U && start.isoWeek <= 53U) {
    const size_t index = static_cast<size_t>(start.month - 1U) * 53U + static_cast<size_t>(start.isoWeek - 1U);
    ctx.monthWeekIntervalSum[index] += elapsedSeconds;
    ++ctx.monthWeekIntervalSamples[index];
  }

  if (start.year >= ctx.yearMonthStart && start.year <= ctx.yearMonthEnd && start.month >= 1U && start.month <= 12U) {
    const size_t index = static_cast<size_t>(start.year - ctx.yearMonthStart) * 12U + static_cast<size_t>(start.month - 1U);
    ctx.yearMonthIntervalSum[index] += elapsedSeconds;
    ++ctx.yearMonthIntervalSamples[index];
  }
}

bool scanIntervalAnalytics(AnalyticsBundleContext &ctx) {
  const uint64_t first = InterruptionStore::oldestSequence();
  const uint64_t last = InterruptionStore::newestSequence();
  if (first == 0U || last == 0U || first > last) return true;

  bool previousUsable = false;
  InterruptionTypes::RawEvent previous;
  ProjectTime::LocalDateTime previousLocal;

  for (uint64_t sequence = first; sequence <= last; ++sequence) {
    servicePhysicalInputDuringRead(ctx.scanCounter);

    InterruptionTypes::RawEvent current;
    if (!InterruptionStore::readSequence(sequence, current)) {
      previousUsable = false;  // Never bridge a missing/corrupt retained record.
      continue;
    }

    ProjectTime::LocalDateTime currentLocal;
    const bool currentUsable = current.absoluteValid &&
                               ProjectTime::fromEpochSeconds(current.timeValueSeconds, currentLocal);
    if (currentUsable) {
      if (!ctx.oldestRawAbsoluteValid) {
        ctx.oldestRawAbsoluteValid = true;
        ctx.oldestRawDayIndex = currentLocal.dayIndex;
        ctx.oldestRawEpochSeconds = current.timeValueSeconds;
      }
      ctx.newestRawDayIndex = currentLocal.dayIndex;
      ctx.newestRawEpochSeconds = current.timeValueSeconds;
    }

    if (previousUsable && currentUsable && previousLocal.dayIndex == currentLocal.dayIndex &&
        current.deltaSeconds > 0U && current.deltaSeconds < InterruptionTypes::DELTA_UNKNOWN &&
        current.timeValueSeconds > previous.timeValueSeconds) {
      const uint32_t elapsedSeconds = current.timeValueSeconds - previous.timeValueSeconds;
      // deltaSeconds is generated from the same absolute timestamps. Requiring
      // equality also proves that the current event actually follows this
      // retained predecessor instead of an older calendar anchor.
      if (elapsedSeconds == current.deltaSeconds) {
        addIntervalSample(ctx, previousLocal, elapsedSeconds);
      }
    }

    previous = current;
    previousLocal = currentLocal;
    previousUsable = currentUsable;
  }
  return true;
}

'''
replace_once(
    "Unterbrechungszaehler/interruption_api.cpp",
    "void currentLocal(ProjectTime::LocalDateTime &local) {\n",
    interval_helpers + "void currentLocal(ProjectTime::LocalDateTime &local) {\n",
)

new_bundle = r'''String buildAnalyticsBundleJson(const char *metric,
                                const char *hourlyMode,
                                uint16_t hourlyYear,
                                uint8_t hourlyWeek,
                                const char *fromDate,
                                const char *toDate,
                                uint16_t monthWeekYear,
                                bool &validRequest) {
  validRequest = false;
  if (monthWeekYear < 2020 || monthWeekYear > 2099) return String();

  const bool averageInterval = metric && strcmp(metric, "averageInterval") == 0;
  const bool countMetric = !metric || !*metric || strcmp(metric, "count") == 0;
  if (!averageInterval && !countMetric) return String();

  static AnalyticsBundleContext ctx;
  ctx = AnalyticsBundleContext{};
  ctx.averageInterval = averageInterval;
  ctx.monthWeekYear = monthWeekYear;

  uint16_t hourlyCoverageStart = 0;
  if (hourlyMode && strcmp(hourlyMode, "week") == 0) {
    if (hourlyYear < 2020 || hourlyYear > 2099 || hourlyWeek < 1 || hourlyWeek > 53 ||
        !isoWeekStartDayIndex(hourlyYear, hourlyWeek, hourlyCoverageStart)) {
      return String();
    }
    ctx.hourlyWeekMode = true;
    ctx.hourlyYear = hourlyYear;
    ctx.hourlyWeek = hourlyWeek;
  } else if (hourlyMode && strcmp(hourlyMode, "range") == 0) {
    if (!ProjectTime::parseDate(fromDate, ctx.hourlyFrom) ||
        !ProjectTime::parseDate(toDate, ctx.hourlyTo) ||
        ctx.hourlyFrom > ctx.hourlyTo) {
      return String();
    }
    hourlyCoverageStart = ctx.hourlyFrom;
  } else {
    return String();
  }

  ProjectTime::LocalDateTime local;
  currentLocal(local);
  uint16_t endYear = 0;
  if (local.valid) {
    endYear = local.year;
  } else {
    YearMonthContext probe;
    probe.onlyFindMax = true;
    InterruptionAggregates::forEach(visitYearMonth, &probe);
    endYear = probe.maxYear ? probe.maxYear : 2025;
  }
  ctx.yearMonthEnd = endYear;
  ctx.yearMonthStart = endYear >= 4 ? static_cast<uint16_t>(endYear - 4U) : endYear;

  uint16_t monthWeekCoverageStart = 0;
  uint16_t yearMonthCoverageStart = 0;
  if (!ProjectTime::dateToDayIndex(monthWeekYear, 1, 1, monthWeekCoverageStart) ||
      !ProjectTime::dateToDayIndex(ctx.yearMonthStart, 1, 1, yearMonthCoverageStart)) {
    return String();
  }

  const bool scanOk = averageInterval ? scanIntervalAnalytics(ctx)
                                      : InterruptionAggregates::forEach(visitAnalyticsBundle, &ctx);
  if (!scanOk) return String();
  validRequest = true;

  String out;
  out.reserve(18000);
  out += '{';
  fieldBool(out, "ok", true);

  JsonUtils::appendKey(out, "storage");
  appendStorageObject(out);
  out += ',';

  JsonUtils::appendKey(out, "hourly");
  out += '{';
  fieldBool(out, "ok", true);
  fieldString(out, "metric", averageInterval ? "averageInterval" : "count");
  fieldString(out, "unit", averageInterval ? "seconds" : "count");
  fieldUInt(out, "rows", 7);
  fieldUInt(out, "cols", 24);
  fieldInt(out, "currentRow", local.valid ? local.weekday : -1);
  fieldInt(out, "currentCol", local.valid ? local.hour : -1);
  JsonUtils::appendKey(out, "values");
  if (averageInterval) appendIntervalValues(out, ctx.hourlyIntervalSum, ctx.hourlyIntervalSamples, 7U * 24U);
  else appendValues(out, ctx.hourly, 7U * 24U);
  if (averageInterval) {
    out += ',';
    JsonUtils::appendKey(out, "samples");
    appendValues(out, ctx.hourlyIntervalSamples, 7U * 24U);
    out += ',';
    JsonUtils::appendKey(out, "coverage");
    appendIntervalCoverage(out, ctx, intervalCoverageComplete(ctx, hourlyCoverageStart));
  }
  out += '}';
  out += ',';

  JsonUtils::appendKey(out, "monthWeek");
  out += '{';
  fieldBool(out, "ok", true);
  fieldString(out, "metric", averageInterval ? "averageInterval" : "count");
  fieldString(out, "unit", averageInterval ? "seconds" : "count");
  fieldUInt(out, "rows", 12);
  fieldUInt(out, "cols", 53);
  fieldUInt(out, "year", monthWeekYear);
  const bool currentMonthWeekYear = local.valid && local.year == monthWeekYear;
  fieldInt(out, "currentRow", currentMonthWeekYear ? static_cast<int32_t>(local.month - 1U) : -1);
  fieldInt(out, "currentCol", currentMonthWeekYear ? static_cast<int32_t>(local.isoWeek - 1U) : -1);
  JsonUtils::appendKey(out, "values");
  if (averageInterval) appendIntervalValues(out, ctx.monthWeekIntervalSum, ctx.monthWeekIntervalSamples, 12U * 53U);
  else appendValues(out, ctx.monthWeek, 12U * 53U);
  if (averageInterval) {
    out += ',';
    JsonUtils::appendKey(out, "samples");
    appendValues(out, ctx.monthWeekIntervalSamples, 12U * 53U);
    out += ',';
    JsonUtils::appendKey(out, "coverage");
    appendIntervalCoverage(out, ctx, intervalCoverageComplete(ctx, monthWeekCoverageStart));
  }
  out += '}';
  out += ',';

  JsonUtils::appendKey(out, "yearMonth");
  out += '{';
  fieldBool(out, "ok", true);
  fieldString(out, "metric", averageInterval ? "averageInterval" : "count");
  fieldString(out, "unit", averageInterval ? "seconds" : "count");
  fieldUInt(out, "rows", 5);
  fieldUInt(out, "cols", 12);
  fieldUInt(out, "startYear", ctx.yearMonthStart);
  fieldUInt(out, "endYear", ctx.yearMonthEnd);
  fieldInt(out,
           "currentRow",
           local.valid && local.year >= ctx.yearMonthStart && local.year <= ctx.yearMonthEnd
               ? static_cast<int32_t>(local.year - ctx.yearMonthStart)
               : -1);
  fieldInt(out, "currentCol", local.valid ? static_cast<int32_t>(local.month - 1U) : -1);
  JsonUtils::appendKey(out, "values");
  if (averageInterval) appendIntervalValues(out, ctx.yearMonthIntervalSum, ctx.yearMonthIntervalSamples, 5U * 12U);
  else appendValues(out, ctx.yearMonth, 5U * 12U);
  if (averageInterval) {
    out += ',';
    JsonUtils::appendKey(out, "samples");
    appendValues(out, ctx.yearMonthIntervalSamples, 5U * 12U);
    out += ',';
    JsonUtils::appendKey(out, "coverage");
    appendIntervalCoverage(out, ctx, intervalCoverageComplete(ctx, yearMonthCoverageStart));
  }
  out += '}';

  out += '}';
  return out;
}

String buildHourlyHeatmapJson'''
regex_once(
    "Unterbrechungszaehler/interruption_api.cpp",
    r"String buildAnalyticsBundleJson\(.*?\nString buildHourlyHeatmapJson",
    new_bundle,
    re.DOTALL,
)

replace_once(
    "Unterbrechungszaehler/web_server.cpp",
    "  const String mode = server.arg(\"hourlyMode\");\n",
    "  const String metric = server.arg(\"metric\");\n  const String mode = server.arg(\"hourlyMode\");\n",
)
replace_once(
    "Unterbrechungszaehler/web_server.cpp",
    "  const String json = InterruptionApi::buildAnalyticsBundleJson(\n      mode.c_str(), hourlyYear, hourlyWeek, from.c_str(), to.c_str(), monthWeekYear, valid);",
    "  const String json = InterruptionApi::buildAnalyticsBundleJson(\n      metric.length() ? metric.c_str() : \"count\", mode.c_str(), hourlyYear, hourlyWeek,\n      from.c_str(), to.c_str(), monthWeekYear, valid);",
)

# ---------------------------------------------------------------------------
# Frontend: metric toggle, inverted heat intensity for interval duration and
# display master switch.
# ---------------------------------------------------------------------------
js_path = "Unterbrechungszaehler/ui-src/app.js"
text = read(js_path)

translations = {
    "de": {
        "project.displayEnabled": "Display",
        "analytics.metric": "Metrik",
        "analytics.metric.count": "Anzahl",
        "analytics.metric.averageInterval": "Ø Abstand",
        "analytics.intervalSamples": "{n} Abstände",
        "analytics.coveragePartial": "Ø-Abstand basiert auf den noch vorhandenen Rohereignissen.",
    },
    "en": {
        "project.displayEnabled": "Display",
        "analytics.metric": "Metric",
        "analytics.metric.count": "Count",
        "analytics.metric.averageInterval": "Average interval",
        "analytics.intervalSamples": "{n} intervals",
        "analytics.coveragePartial": "Average interval is based on the raw events that are still retained.",
    },
    "swg": {
        "project.displayEnabled": "Display",
        "analytics.metric": "Metrik",
        "analytics.metric.count": "Anzahl",
        "analytics.metric.averageInterval": "Ø Abstand",
        "analytics.intervalSamples": "{n} Abständ",
        "analytics.coveragePartial": "Dr Ø-Abstand basiert auf de Rohereignisse, wo no im Speicher send.",
    },
    "it": {
        "project.displayEnabled": "Display",
        "analytics.metric": "Metrica",
        "analytics.metric.count": "Conteggio",
        "analytics.metric.averageInterval": "Intervallo medio",
        "analytics.intervalSamples": "{n} intervalli",
        "analytics.coveragePartial": "L'intervallo medio si basa sugli eventi grezzi ancora presenti in memoria.",
    },
    "fr": {
        "project.displayEnabled": "Écran",
        "analytics.metric": "Mesure",
        "analytics.metric.count": "Nombre",
        "analytics.metric.averageInterval": "Intervalle moyen",
        "analytics.intervalSamples": "{n} intervalles",
        "analytics.coveragePartial": "L'intervalle moyen repose sur les événements bruts encore conservés.",
    },
    "swg-alb": {
        "project.displayEnabled": "Display",
        "analytics.metric": "Metrik",
        "analytics.metric.count": "Anzahl",
        "analytics.metric.averageInterval": "Ø Abstand",
        "analytics.intervalSamples": "{n} Abständ",
        "analytics.coveragePartial": "Dr Ø-Abstand kommt aus de Rohereignisse, wo no do send.",
    },
    "swg-ob": {
        "project.displayEnabled": "Display",
        "analytics.metric": "Metrik",
        "analytics.metric.count": "Anzahl",
        "analytics.metric.averageInterval": "Ø Abstand",
        "analytics.intervalSamples": "{n} Abständ",
        "analytics.coveragePartial": "Dr Ø-Abstand basiert auf de Rohereignisse, wo no gspeichert send.",
    },
}

blocks = {
    "de": ("    de: {", "    en: {"),
    "en": ("    en: {", "    swg: {"),
    "swg": ("    swg: {", "\n  };"),
    "it": ("  I18N.it = {", "\n  };"),
    "fr": ("  I18N.fr = {", "\n  };"),
    "swg-alb": ("  I18N['swg-alb'] = {", "\n  };"),
    "swg-ob": ("  I18N['swg-ob'] = {", "\n  };"),
}

for language, mapping in translations.items():
    text = inject_translation_keys(text, blocks[language][0], mapping)

updated_descriptions = {
    "de": {
        "view.analytics.desc": "Heatmaps wahlweise als Anzahl oder durchschnittlicher abgeschlossener Zeitabstand zwischen Unterbrechungen.",
        "analytics.hourly.desc": "Wochentage und Stunden – wahlweise Anzahl oder Ø Abstand bis zur nächsten Unterbrechung am selben Tag.",
        "analytics.monthWeek.desc": "Monate und ISO-Kalenderwochen – wahlweise Anzahl oder Ø Abstand.",
        "analytics.yearMonth.desc": "Letzte fünf Kalenderjahre nach Monat – wahlweise Anzahl oder Ø Abstand.",
    },
    "en": {
        "view.analytics.desc": "Heatmaps can show either interruption counts or the average completed interval between interruptions.",
        "analytics.hourly.desc": "Weekdays and hours – either count or average interval to the next interruption on the same day.",
        "analytics.monthWeek.desc": "Months and ISO calendar weeks – either count or average interval.",
        "analytics.yearMonth.desc": "The last five calendar years by month – either count or average interval.",
    },
    "swg": {
        "view.analytics.desc": "D Heatmaps zeiget entweder d Anzahl oder dr durchschnittlich abgeschlossene Abstand zwischa de Unterbrechunga.",
        "analytics.hourly.desc": "Wochentäg ond Stunda – Anzahl oder Ø Abstand bis zur nächste Unterbrechung am selba Tag.",
        "analytics.monthWeek.desc": "Monat ond ISO-Kalenderwocha – Anzahl oder Ø Abstand.",
        "analytics.yearMonth.desc": "D letzte fünf Kalenderjohr nach Monat – Anzahl oder Ø Abstand.",
    },
    "it": {
        "view.analytics.desc": "Le mappe di calore mostrano il numero di interruzioni oppure l'intervallo medio completato tra le interruzioni.",
        "analytics.hourly.desc": "Giorni e ore: conteggio oppure intervallo medio fino all'interruzione successiva dello stesso giorno.",
        "analytics.monthWeek.desc": "Mesi e settimane ISO: conteggio oppure intervallo medio.",
        "analytics.yearMonth.desc": "Ultimi cinque anni per mese: conteggio oppure intervallo medio.",
    },
    "fr": {
        "view.analytics.desc": "Les cartes thermiques affichent le nombre d'interruptions ou l'intervalle moyen terminé entre les interruptions.",
        "analytics.hourly.desc": "Jours et heures : nombre ou intervalle moyen jusqu'à l'interruption suivante du même jour.",
        "analytics.monthWeek.desc": "Mois et semaines ISO : nombre ou intervalle moyen.",
        "analytics.yearMonth.desc": "Cinq dernières années par mois : nombre ou intervalle moyen.",
    },
    "swg-alb": {
        "view.analytics.desc": "D Heatmaps zeiget Anzahl oder dr durchschnittlich fertige Abstand zwischa de Unterbrechunga.",
        "analytics.hourly.desc": "Wochentäg ond Stunda – Anzahl oder Ø Abstand bis zom nächste Druck am selba Tag.",
        "analytics.monthWeek.desc": "Monat ond Kalenderwocha – Anzahl oder Ø Abstand.",
        "analytics.yearMonth.desc": "D letzte fünf Johr nach Monat – Anzahl oder Ø Abstand.",
    },
    "swg-ob": {
        "view.analytics.desc": "D Heatmaps zeiget Anzahl oder dr durchschnittlich abgeschlossene Abstand zwischa de Unterbrechunga.",
        "analytics.hourly.desc": "Wochentäg ond Stunda – Anzahl oder Ø Abstand bis zur nächste Unterbrechung am selba Tag.",
        "analytics.monthWeek.desc": "Monat ond Kalenderwocha – Anzahl oder Ø Abstand.",
        "analytics.yearMonth.desc": "D letzte fünf Johr nach Monat – Anzahl oder Ø Abstand.",
    },
}
for language, mapping in updated_descriptions.items():
    for key, value in mapping.items():
        text = set_translation(text, blocks[language][0], blocks[language][1], key, value)

old_state = "    projectSettings: { soundEnabled: true, soundMode: 'fixed', soundTrack: 2, soundTrackCount: 0, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 50, displayDimAfterMinutes: 10, displayDimBrightness: 10 },\n    analytics: { loaded: false, loading: false, dirty: false, error: '', storage: null, hourly: null, monthWeek: null, yearMonth: null, hourlyMode: 'week' },"
new_state = "    projectSettings: { soundEnabled: true, soundMode: 'fixed', soundTrack: 2, soundTrackCount: 0, displayEnabled: true, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 50, displayDimAfterMinutes: 10, displayDimBrightness: 10 },\n    analytics: { loaded: false, loading: false, dirty: false, error: '', storage: null, hourly: null, monthWeek: null, yearMonth: null, hourlyMode: 'week', metric: 'count' },"
if old_state not in text:
    raise AssertionError("frontend state marker missing")
text = text.replace(old_state, new_state, 1)

old_switches = "    const soundHint = el('div', 'form-note project-setting-note'); grid.append(soundHint);\n    addSwitch('displayFlashEnabled', 'project.displayFlash');"
new_switches = "    const soundHint = el('div', 'form-note project-setting-note'); grid.append(soundHint);\n    addSwitch('displayEnabled', 'project.displayEnabled');\n    addSwitch('displayFlashEnabled', 'project.displayFlash');"
if old_switches not in text:
    raise AssertionError("display switch marker missing")
text = text.replace(old_switches, new_switches, 1)

new_grid = r'''  function formatIntervalSeconds(value, compact = false) {
    const total = Math.max(0, Math.round(Number(value) || 0));
    if (total < 60) return `${total}s`;
    const hours = Math.floor(total / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const seconds = total % 60;
    if (hours > 0) return `${hours}h${minutes ? ` ${minutes}m` : ''}${!compact && seconds ? ` ${seconds}s` : ''}`;
    if (compact && total >= 600) return `${Math.floor(total / 60)}m`;
    return `${Math.floor(total / 60)}m${seconds ? ` ${seconds}s` : ''}`;
  }

  function renderHeatmapGrid(root, kind, data) {
    const holder = $('.heatmap-holder', root);
    if (!holder) return;
    holder.replaceChildren();
    if (state.analytics.error) {
      const empty = el('div', 'heatmap-empty'); empty.textContent = t('analytics.loadError'); holder.append(empty); return;
    }
    if (!data || !Array.isArray(data.values)) {
      const empty = el('div', 'heatmap-empty'); empty.textContent = t('analytics.noData'); holder.append(empty); return;
    }
    const isAverage = data.metric === 'averageInterval';
    const samples = Array.isArray(data.samples) ? data.samples : [];
    const narrow = window.matchMedia('(max-width: 700px)').matches;
    const transpose = kind === 'monthWeek' ? window.matchMedia('(max-width: 1300px)').matches : narrow;
    const originalRows = Number(data.rows || 0), originalCols = Number(data.cols || 0);
    const rows = transpose ? originalCols : originalRows;
    const cols = transpose ? originalRows : originalCols;
    const labels = heatmapLabels(kind, data, transpose);
    const validAverages = isAverage
      ? data.values.map((v, i) => Number(samples[i] || 0) > 0 ? Number(v) : null).filter(v => Number.isFinite(v))
      : [];
    const minAverage = validAverages.length ? Math.min(...validAverages) : 0;
    const maxAverage = validAverages.length ? Math.max(...validAverages) : 0;
    const maxCount = isAverage ? 0 : Math.max(0, ...data.values.map(v => Number(v) || 0));
    const grid = el('div', 'heatmap-grid');
    grid.setAttribute('role', 'grid');
    grid.style.setProperty('--heat-cols', String(cols));

    const corner = el('div', 'heatmap-corner'); grid.append(corner);
    for (let col = 0; col < cols; col++) {
      const head = el('div', 'heatmap-col-head'); head.textContent = labels.cols[col] || String(col + 1);
      const originalCol = transpose ? -1 : col;
      const originalRow = transpose ? col : -1;
      if ((originalCol >= 0 && originalCol === Number(data.currentCol)) || (originalRow >= 0 && originalRow === Number(data.currentRow))) head.classList.add('is-current');
      grid.append(head);
    }
    for (let row = 0; row < rows; row++) {
      const rowHead = el('div', 'heatmap-row-head'); rowHead.textContent = labels.rows[row] || String(row + 1);
      const originalRowForHead = transpose ? -1 : row;
      const originalColForHead = transpose ? row : -1;
      if ((originalRowForHead >= 0 && originalRowForHead === Number(data.currentRow)) || (originalColForHead >= 0 && originalColForHead === Number(data.currentCol))) rowHead.classList.add('is-current');
      grid.append(rowHead);
      for (let col = 0; col < cols; col++) {
        const originalRow = transpose ? col : row;
        const originalCol = transpose ? row : col;
        const index = originalRow * originalCols + originalCol;
        const value = Number(data.values[index] || 0);
        const sampleCount = Number(samples[index] || 0);
        const hasValue = isAverage ? sampleCount > 0 : value > 0;
        const cell = el('div', 'heatmap-cell');
        const shown = isAverage ? (hasValue ? formatIntervalSeconds(value, true) : '—') : String(value);
        cell.textContent = shown;
        let heatPercent = 0;
        if (isAverage && hasValue) {
          heatPercent = maxAverage > minAverage
            ? 8 + ((maxAverage - value) / (maxAverage - minAverage)) * 88
            : 70;
        } else if (!isAverage && value > 0 && maxCount > 0) {
          heatPercent = Math.max(8, (value / maxCount) * 88);
        }
        cell.style.setProperty('--heat-pct', `${Math.round(heatPercent)}%`);
        if (!hasValue) cell.classList.add('is-zero');
        if (originalRow === Number(data.currentRow)) cell.classList.add('current-row');
        if (originalCol === Number(data.currentCol)) cell.classList.add('current-col');
        if (originalRow === Number(data.currentRow) && originalCol === Number(data.currentCol)) cell.classList.add('current-intersection');
        cell.setAttribute('role', 'gridcell');
        const rowLabel = transpose ? labels.cols[col] : labels.rows[row];
        const colLabel = transpose ? labels.rows[row] : labels.cols[col];
        const detail = isAverage
          ? (hasValue ? `${formatIntervalSeconds(value, false)} · ${t('analytics.intervalSamples').replace('{n}', String(sampleCount))}` : '—')
          : String(value);
        const accessible = `${rowLabel}, ${colLabel}: ${detail}`;
        cell.setAttribute('aria-label', accessible);
        cell.title = accessible;
        grid.append(cell);
      }
    }
    holder.append(grid);
    if (isAverage && data.coverage?.complete === false) {
      const coverage = el('div', 'form-note heatmap-coverage');
      coverage.textContent = t('analytics.coveragePartial');
      holder.append(coverage);
    }
  }

  function createFilterField'''
text, count = re.subn(
    r"  function renderHeatmapGrid\(root, kind, data\) \{.*?\n  function createFilterField",
    new_grid,
    text,
    count=1,
    flags=re.DOTALL,
)
if count != 1:
    raise AssertionError("renderHeatmapGrid replacement failed")

marker = "  function renderHeatmapHourly() {\n"
metric_helper = r'''  function createAnalyticsMetricField() {
    const wrap = el('label', 'analytics-filter-field');
    const label = el('span'); label.textContent = t('analytics.metric');
    const select = el('select'); select.dataset.analyticsMetric = '1';
    for (const [value, key] of [['count','analytics.metric.count'],['averageInterval','analytics.metric.averageInterval']]) {
      const option = el('option'); option.value = value; option.textContent = t(key); select.append(option);
    }
    select.value = state.analytics.metric || 'count';
    wrap.append(label, select);
    return { wrap, select };
  }

'''
if marker not in text:
    raise AssertionError("metric helper marker missing")
text = text.replace(marker, metric_helper + marker, 1)

new_heatmap_renderers = r'''  function renderHeatmapHourly() {
    const root = el('div', 'analytics-block'); root.dataset.heatmapKind = 'hourly';
    const controls = el('div', 'analytics-filters');
    const metricField = createAnalyticsMetricField();
    const modeWrap = el('label', 'analytics-filter-field'); const modeLabel = el('span'); modeLabel.textContent = t('analytics.mode');
    const mode = el('select'); mode.dataset.analyticsHourlyMode = '1';
    for (const [value, key] of [['week','analytics.mode.week'],['range','analytics.mode.range']]) { const option=el('option'); option.value=value; option.textContent=t(key); mode.append(option); }
    mode.value = state.analytics.hourlyMode || 'week'; modeWrap.append(modeLabel, mode);
    const current = projectCurrentCalendar();
    const yearField = createFilterField('analytics.year','number',String(current.year),'analyticsHourlyYear'); yearField.input.min='2020'; yearField.input.max='2199';
    const weekField = createFilterField('analytics.week','number',String(current.week),'analyticsHourlyWeek'); weekField.input.min='1'; weekField.input.max='53';
    const dateText = `${current.year}-${String(current.month).padStart(2,'0')}-${String(current.day).padStart(2,'0')}`;
    const fromField = createFilterField('analytics.from','date',dateText,'analyticsHourlyFrom');
    const toField = createFilterField('analytics.to','date',dateText,'analyticsHourlyTo');
    const button = el('button','button'); button.type='button'; button.dataset.analyticsAction='hourly'; button.append(icon('refresh')); const bt=el('span'); bt.textContent=t('analytics.load'); button.append(bt);
    controls.append(metricField.wrap,modeWrap,yearField.wrap,weekField.wrap,fromField.wrap,toField.wrap,button);
    const holder = el('div','heatmap-holder'); root.append(controls,holder);
    const updateMode = () => { const isWeek=mode.value==='week'; yearField.wrap.hidden=!isWeek; weekField.wrap.hidden=!isWeek; fromField.wrap.hidden=isWeek; toField.wrap.hidden=isWeek; state.analytics.hourlyMode=mode.value; };
    mode.addEventListener('change', updateMode); updateMode();
    Bindings.add(['analytics.hourly','analytics.error'],()=>renderHeatmapGrid(root,'hourly',state.analytics.hourly));
    return root;
  }

  function renderHeatmapMonthWeek() {
    const root=el('div','analytics-block'); root.dataset.heatmapKind='monthWeek';
    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); const year=createFilterField('analytics.year','number',String(projectCurrentCalendar().year),'analyticsMonthWeekYear'); year.input.min='2020'; year.input.max='2199';
    const button=el('button','button'); button.type='button'; button.dataset.analyticsAction='month-week'; button.append(icon('refresh')); const bt=el('span');bt.textContent=t('analytics.load');button.append(bt); controls.append(metricField.wrap,year.wrap,button);
    const holder=el('div','heatmap-holder'); root.append(controls,holder);
    Bindings.add(['analytics.monthWeek','analytics.error'],()=>renderHeatmapGrid(root,'monthWeek',state.analytics.monthWeek)); return root;
  }

  function renderHeatmapYearMonth() {
    const root=el('div','analytics-block'); root.dataset.heatmapKind='yearMonth';
    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); controls.append(metricField.wrap);
    const holder=el('div','heatmap-holder'); root.append(controls,holder);
    Bindings.add(['analytics.yearMonth','analytics.error'],()=>renderHeatmapGrid(root,'yearMonth',state.analytics.yearMonth)); return root;
  }

  function renderAnalyticsStorage'''
text, count = re.subn(
    r"  function renderHeatmapHourly\(\) \{.*?\n  function renderAnalyticsStorage",
    new_heatmap_renderers,
    text,
    count=1,
    flags=re.DOTALL,
)
if count != 1:
    raise AssertionError("heatmap renderer replacement failed")

old_parts = "      const parts = [`hourlyMode=${encodeURIComponent(mode)}`];"
new_parts = "      const metric = state.analytics.metric || 'count';\n      const parts = [`metric=${encodeURIComponent(metric)}`, `hourlyMode=${encodeURIComponent(mode)}`];"
if old_parts not in text:
    raise AssertionError("analytics bundle query marker missing")
text = text.replace(old_parts, new_parts, 1)

old_click = '''    if (analyticsAction) {
      const root = analyticsAction.closest('[data-heatmap-kind]');
      if (analyticsAction.dataset.analyticsAction === 'hourly') await Transport.loadHourlyHeatmap(root);
      else if (analyticsAction.dataset.analyticsAction === 'month-week') await Transport.loadMonthWeekHeatmap(root);
      return;
    }'''
new_click = '''    if (analyticsAction) {
      const root = analyticsAction.closest('[data-heatmap-kind]');
      if ((state.analytics.metric || 'count') === 'averageInterval') await Transport.loadAnalytics(true);
      else if (analyticsAction.dataset.analyticsAction === 'hourly') await Transport.loadHourlyHeatmap(root);
      else if (analyticsAction.dataset.analyticsAction === 'month-week') await Transport.loadMonthWeekHeatmap(root);
      return;
    }'''
if old_click not in text:
    raise AssertionError("analytics click marker missing")
text = text.replace(old_click, new_click, 1)

change_marker = "  document.addEventListener('change', event => {\n    const projectSetting = event.target.closest('[data-project-setting]');"
change_new = "  document.addEventListener('change', event => {\n    const metricControl = event.target.closest('[data-analytics-metric]');\n    if (metricControl) {\n      state.analytics.metric = metricControl.value === 'averageInterval' ? 'averageInterval' : 'count';\n      for (const control of document.querySelectorAll('[data-analytics-metric]')) control.value = state.analytics.metric;\n      state.analytics.dirty = true;\n      Transport.loadAnalytics(true);\n      return;\n    }\n    const projectSetting = event.target.closest('[data-project-setting]');"
if change_marker not in text:
    raise AssertionError("change handler marker missing")
text = text.replace(change_marker, change_new, 1)

write(js_path, text)

# ---------------------------------------------------------------------------
# Host-side semantics tests
# ---------------------------------------------------------------------------
test_path = "Unterbrechungszaehler/tools/test_interruption_storage.py"
replace_once(
    test_path,
    "def aggregate(events: list[Event]) -> dict[int, list[int]]:\n",
    '''def completed_intervals(events: list[Event]) -> list[tuple[Event, int]]:\n    result: list[tuple[Event, int]] = []\n    for previous, current in zip(events, events[1:]):\n        if day_index(previous.when) != day_index(current.when):\n            continue\n        elapsed = int(current.when.timestamp() - previous.when.timestamp())\n        if elapsed > 0:\n            result.append((previous, elapsed))\n    return result\n\n\ndef aggregate(events: list[Event]) -> dict[int, list[int]]:\n''',
)

average_tests = r'''

def test_average_interval_semantics() -> None:
    events = [
        Event(datetime(2026, 9, 2, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 8, 15, tzinfo=TZ)),
        Event(datetime(2026, 9, 2, 10, 0, tzinfo=TZ)),
    ]
    samples = completed_intervals(events)
    assert [seconds for _, seconds in samples] == [900, 6300]
    assert [start.when.hour for start, _ in samples] == [8, 8]
    assert round(sum(seconds for _, seconds in samples) / len(samples)) == 3600
    # The last press at 10:00 has no following same-day press and therefore no sample.
    assert all(start.when.hour != 10 for start, _ in samples)

    # Never bridge midnight.
    midnight = [
        Event(datetime(2026, 9, 2, 23, 50, tzinfo=TZ)),
        Event(datetime(2026, 9, 3, 0, 10, tzinfo=TZ)),
    ]
    assert completed_intervals(midnight) == []

    # One press on a day yields no completed interval.
    assert completed_intervals([Event(datetime(2026, 9, 4, 9, 0, tzinfo=TZ))]) == []

    # Weighted average over all samples, not an average of daily averages.
    weighted = [
        Event(datetime(2026, 9, 7, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 7, 8, 10, tzinfo=TZ)),
        Event(datetime(2026, 9, 8, 8, 0, tzinfo=TZ)),
        Event(datetime(2026, 9, 8, 8, 30, tzinfo=TZ)),
    ]
    values = [seconds for _, seconds in completed_intervals(weighted)]
    assert values == [600, 1800]
    assert sum(values) // len(values) == 1200

    # DST spring-forward still uses real elapsed epoch seconds.
    before = Event(datetime(2026, 3, 29, 1, 30, tzinfo=TZ))
    after = Event((before.when.astimezone(timezone.utc) + timedelta(hours=1)).astimezone(TZ))
    dst = completed_intervals([before, after])
    assert len(dst) == 1 and dst[0][1] == 3600


def test_average_interval_ring_coverage_rule() -> None:
    def complete(raw_count: int, capacity: int, oldest_sequence: int, oldest_day: int, selected_start: int) -> bool:
        overwritten = capacity > 0 and raw_count >= capacity and oldest_sequence > 1
        return not overwritten or oldest_day <= selected_start

    assert complete(50_000, RAW_CAPACITY, 1, 2000, 1000)
    assert complete(RAW_CAPACITY, RAW_CAPACITY, 1, 2000, 1000)
    assert not complete(RAW_CAPACITY, RAW_CAPACITY, 5001, 2500, 2400)
    assert complete(RAW_CAPACITY, RAW_CAPACITY, 5001, 2500, 2500)
'''
text = read(test_path)
marker = "\ndef test_pending_queue_overflow_keeps_live_count_visible() -> None:\n"
if marker not in text:
    raise AssertionError("test insertion marker missing")
text = text.replace(marker, average_tests + marker, 1)
text = text.replace(
    "        test_heatmap_views_from_daily_aggregates,\n        test_pending_queue_overflow_keeps_live_count_visible,",
    "        test_heatmap_views_from_daily_aggregates,\n        test_average_interval_semantics,\n        test_average_interval_ring_coverage_rule,\n        test_pending_queue_overflow_keeps_live_count_visible,",
    1,
)
write(test_path, text)

# ---------------------------------------------------------------------------
# Release gates
# ---------------------------------------------------------------------------
release_path = "Unterbrechungszaehler/tools/release_check.py"
text = read(release_path)
text = text.replace("Portable release checks for Unterbrechungszaehler 3.0.1.", "Portable release checks for Unterbrechungszaehler 3.1.0.", 1)
text = text.replace('check(\'SOFTWARE_VERSION[] = "3.0.1"\' in config, "project version 3.0.1")', 'check(\'SOFTWARE_VERSION[] = "3.1.0"\' in config, "project version 3.1.0")', 1)
marker = '    check("PENDING_EVENT_CAPACITY = 64" in project, "64-event fixed persistence queue")\n'
if marker not in text:
    raise AssertionError("release check marker missing")
text = text.replace(
    marker,
    marker
    + '    check("DISPLAY_ENABLED_DEFAULT = true" in project, "persistent display master default")\n'
    + '    check("DISPLAY_BOOT_SCREEN_MIN_MS = 2000" in hardware, "two-second nonblocking boot screen minimum")\n'
    + '    check("displayEnabled" in JS and "project.displayEnabled" in JS, "display master switch in UI")\n'
    + '    check("averageInterval" in JS and "analytics.coveragePartial" in JS, "average-interval heatmap UI")\n'
    + '    interruption_api = (ROOT / "interruption_api.cpp").read_text(encoding="utf-8")\n'
    + '    check("scanIntervalAnalytics" in interruption_api and "elapsedSeconds == current.deltaSeconds" in interruption_api, "retained adjacent-event interval scan")\n'
    + '    check("delay(2000)" not in (ROOT / "display_views.cpp").read_text(encoding="utf-8") and "delay(2000)" not in (ROOT / "display_sh1106.cpp").read_text(encoding="utf-8"), "boot screen has no blocking two-second delay")\n',
    1,
)
write(release_path, text)

print("3.1.0 source patch applied")
