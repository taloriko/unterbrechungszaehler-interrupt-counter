from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(path, old, new):
    text = read(path)
    if old not in text:
        raise SystemExit(f"missing anchor in {path}: {old[:120]!r}")
    write(path, text.replace(old, new, 1))


# Version
replace_once("Unterbrechungszaehler/config.h", 'SOFTWARE_VERSION[] = "3.3.1"', 'SOFTWARE_VERSION[] = "3.3.2"')
replace_once("Unterbrechungszaehler/audio_dy_sv17f.h", "// Existing playback path. 3.3.1 deliberately changes diagnostics only.", "// Existing playback path remains unchanged; 3.3.2 fixes only diagnostic test completion.")

# A deliberately slow, test-only UART status cadence. This is not a permanent poll.
replace_once(
    "Unterbrechungszaehler/hardware_config.h",
    "constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;\nconstexpr uint32_t AUDIO_DIAGNOSTIC_TEST_TIMEOUT_MS = 120000;",
    "constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;\nconstexpr uint32_t AUDIO_DIAGNOSTIC_STATUS_POLL_MS = 500; // only while the explicit manual audio test is active\nconstexpr uint32_t AUDIO_DIAGNOSTIC_TEST_TIMEOUT_MS = 120000;",
)

path = "Unterbrechungszaehler/audio_dy_sv17f.cpp"
text = read(path)

text = text.replace(
    "  TestPlay,\n  TestVerifyPlaying\n};",
    "  TestPlay,\n  TestVerifyPlaying,\n  TestCheckStopped\n};",
    1,
)

old = '''      setHealth(StatusRegistry::State::Checking);\n      SerialLog::successf("AUDIO", "AUDIO TEST | UART playback confirmed | BUSY now=%s",\n                          testBusyDuringKnown ? (testBusyDuringHigh ? "HIGH" : "LOW") : "n/a");\n      break;'''
new = '''      setHealth(StatusRegistry::State::Checking);\n      deferredAction = DeferredAction::TestCheckStopped;\n      deferredAtMs = millis() + HardwareConfig::AUDIO_DIAGNOSTIC_STATUS_POLL_MS;\n      SerialLog::successf("AUDIO", "AUDIO TEST | UART playback confirmed | BUSY now=%s | end checks every %lu ms",\n                          testBusyDuringKnown ? (testBusyDuringHigh ? "HIGH" : "LOW") : "n/a",\n                          static_cast<unsigned long>(HardwareConfig::AUDIO_DIAGNOSTIC_STATUS_POLL_MS));\n      break;'''
if old not in text:
    raise SystemExit("TestPlaying anchor missing")
text = text.replace(old, new, 1)

old = '''      if (currentPlayState == PlayState::Playing) {\n        testBusyDuringKnown = busyLevelKnown;\n        testBusyDuringHigh = currentBusyLevelHigh;\n        setHealth(StatusRegistry::State::Checking);\n        SerialLog::infof("AUDIO", "AUDIO TEST | BUSY edge checked by UART | still playing | BUSY=%s",\n                         testBusyDuringKnown ? (testBusyDuringHigh ? "HIGH" : "LOW") : "n/a");\n        return;\n      }'''
new = '''      if (currentPlayState == PlayState::Playing) {\n        testBusyDuringKnown = busyLevelKnown;\n        testBusyDuringHigh = currentBusyLevelHigh;\n        setHealth(StatusRegistry::State::Checking);\n        deferredAction = DeferredAction::TestCheckStopped;\n        deferredAtMs = millis() + HardwareConfig::AUDIO_DIAGNOSTIC_STATUS_POLL_MS;\n        SerialLog::infof("AUDIO", "AUDIO TEST | UART end check | still playing | BUSY=%s",\n                         testBusyDuringKnown ? (testBusyDuringHigh ? "HIGH" : "LOW") : "n/a");\n        return;\n      }'''
if old not in text:
    raise SystemExit("TestTransition playing anchor missing")
text = text.replace(old, new, 1)

old = '''        if (completeCycle) {\n          confirmedBusyPolarity = testBusyDuringHigh ? BusyPolarity::ActiveHigh : BusyPolarity::ActiveLow;\n          finishAudioTest(AudioTestState::Ok, StatusRegistry::State::Ok, "");\n        } else {\n          confirmedBusyPolarity = BusyPolarity::Unconfirmed;\n          finishAudioTest(AudioTestState::Partial, StatusRegistry::State::Warning,\n                          "UART playback works; BUSY polarity not confirmed");\n        }\n        return;'''
new = '''        if (completeCycle) {\n          confirmedBusyPolarity = testBusyDuringHigh ? BusyPolarity::ActiveHigh : BusyPolarity::ActiveLow;\n          SerialLog::successf("AUDIO", "AUDIO TEST | UART stopped confirmed | BUSY polarity=%s", busyPolarityName());\n        } else {\n          confirmedBusyPolarity = BusyPolarity::Unconfirmed;\n          SerialLog::warning("AUDIO", "AUDIO TEST | UART start/end confirmed | BUSY polarity remains unconfirmed");\n        }\n        // UART is the protocol truth for test completion. BUSY is additional\n        // diagnostic information and must never keep a working module in Checking.\n        finishAudioTest(AudioTestState::Ok, StatusRegistry::State::Ok, "");\n        return;'''
if old not in text:
    raise SystemExit("TestTransition stopped anchor missing")
text = text.replace(old, new, 1)

old = '''  if (testUartPlaying && waitingFor == WaitKind::None && deferredAction == DeferredAction::None) {\n    sendQuery(WaitKind::TestTransition, 0x01);\n  }'''
new = '''  if (testUartPlaying && waitingFor == WaitKind::None) {\n    // A BUSY edge may accelerate the next UART end check, but it is not\n    // required for completion. Cancel only our own scheduled end check.\n    if (deferredAction == DeferredAction::TestCheckStopped) deferredAction = DeferredAction::None;\n    if (deferredAction == DeferredAction::None) sendQuery(WaitKind::TestTransition, 0x01);\n  }'''
if old not in text:
    raise SystemExit("serviceBusyEdge anchor missing")
text = text.replace(old, new, 1)

old = '''  if (manualTestActive && due(now, manualTestStartedAt + HardwareConfig::AUDIO_DIAGNOSTIC_TEST_TIMEOUT_MS) &&\n      waitingFor == WaitKind::None && deferredAction == DeferredAction::None) {\n    finishAudioTest(testUartPlaying ? AudioTestState::Partial : AudioTestState::Warning,\n                    StatusRegistry::State::Warning,\n                    "audio test timeout; track end was not confirmed");\n  }'''
new = '''  if (manualTestActive && due(now, manualTestStartedAt + HardwareConfig::AUDIO_DIAGNOSTIC_TEST_TIMEOUT_MS)) {\n    // Hard safety stop even if an end-check query/deferred action is active.\n    finishAudioTest(testUartPlaying ? AudioTestState::Partial : AudioTestState::Warning,\n                    StatusRegistry::State::Warning,\n                    "audio test timeout; UART track end was not confirmed");\n  }'''
if old not in text:
    raise SystemExit("manual test timeout anchor missing")
text = text.replace(old, new, 1)

old = '''    } else if (action == DeferredAction::TestVerifyPlaying) {\n      if (manualTestActive && waitingFor == WaitKind::None) sendQuery(WaitKind::TestPlaying, 0x01);\n    }\n  }'''
new = '''    } else if (action == DeferredAction::TestVerifyPlaying) {\n      if (manualTestActive && waitingFor == WaitKind::None) sendQuery(WaitKind::TestPlaying, 0x01);\n    } else if (action == DeferredAction::TestCheckStopped) {\n      if (manualTestActive && testUartPlaying && waitingFor == WaitKind::None) {\n        SerialLog::info("AUDIO", "AUDIO TEST | scheduled UART end check");\n        sendQuery(WaitKind::TestTransition, 0x01);\n      }\n    }\n  }'''
if old not in text:
    raise SystemExit("deferred action anchor missing")
text = text.replace(old, new, 1)

old = '''         deferredAction == DeferredAction::VerifyPlay ||\n         deferredAction == DeferredAction::TestPlay ||\n         deferredAction == DeferredAction::TestVerifyPlaying;'''
new = '''         deferredAction == DeferredAction::VerifyPlay ||\n         deferredAction == DeferredAction::TestPlay ||\n         deferredAction == DeferredAction::TestVerifyPlaying ||\n         deferredAction == DeferredAction::TestCheckStopped;'''
if old not in text:
    raise SystemExit("checking() anchor missing")
text = text.replace(old, new, 1)

write(path, text)

# Release checks
path = "Unterbrechungszaehler/tools/release_check.py"
text = read(path)
text = text.replace("Unterbrechungszaehler 3.3.1", "Unterbrechungszaehler 3.3.2", 1)
text = text.replace('SOFTWARE_VERSION[] = "3.3.1"', 'SOFTWARE_VERSION[] = "3.3.2"', 1)
text = text.replace('"project version 3.3.1"', '"project version 3.3.2"', 1)
text = text.replace('f"3.3.1 UI additions present for {language}"', 'f"3.3.2 UI additions present for {language}"', 1)
anchor = '    check("AudioTestState::Partial" in audio_cpp and "audioTestUartPlayingConfirmed" in audio_h, "manual audio-test result is explicit")\n'
addition = '''    check("AUDIO_DIAGNOSTIC_STATUS_POLL_MS = 500" in hardware, "manual audio test has bounded UART end-check cadence")\n    check("DeferredAction::TestCheckStopped" in audio_cpp and "scheduled UART end check" in audio_cpp, "UART end checks run only inside explicit audio test")\n    check("UART is the protocol truth for test completion" in audio_cpp and "finishAudioTest(AudioTestState::Ok, StatusRegistry::State::Ok" in audio_cpp, "UART stopped confirmation completes test even when BUSY is unconfirmed")\n    check("audio test timeout; UART track end was not confirmed" in audio_cpp, "manual audio test has hard safety timeout")\n'''
if anchor not in text:
    raise SystemExit("release check audio anchor missing")
text = text.replace(anchor, anchor + addition, 1)
write(path, text)

# Changelog
path = "CHANGELOG.md"
text = read(path)
entry = '''## 3.3.2\n\n- DY-SV17F-Audiotest beendet sich jetzt zuverlässig über den aktiv abgefragten UART-Wiedergabestatus statt auf eine weitere BUSY-Flanke angewiesen zu sein\n- Während eines ausdrücklich gestarteten Audiotests wird der Status mit sparsamen 500-ms-Abständen abgefragt; außerhalb des Tests gibt es weiterhin kein zyklisches UART- oder BUSY-Polling\n- BUSY-Flanken beschleunigen eine Statusprüfung weiterhin, sind aber nur Zusatzdiagnose und keine Voraussetzung für einen erfolgreichen Test\n- Wenn UART sowohl `Spielt` als auch anschließend `Gestoppt` bestätigt, endet der Audiotest mit OK; eine nicht bestätigte BUSY-Polarität wird separat angezeigt\n- Der 120-s-Sicherheits-Timeout beendet den Test nun auch dann sicher, wenn gerade eine Statusabfrage geplant oder aktiv ist\n\n'''
if "## 3.3.2" not in text:
    text = text.replace("# Changelog\n\n", "# Changelog\n\n" + entry, 1)
write(path, text)

# Release notes: current release content only needs the patch summary at the top.
path = "Unterbrechungszaehler/RELEASE_NOTES.md"
text = read(path)
if not text.startswith("# Release 3.3.2"):
    text = '''# Release 3.3.2\n\n- manueller DY-SV17F-Audiotest beendet sich anhand gezielter UART-Statusabfragen sicher, auch wenn keine weitere BUSY-Flanke kommt\n- UART wird nur während des ausdrücklich gestarteten Audiotests in 500-ms-Abständen auf das Trackende geprüft; außerhalb des Tests bleibt die Diagnose ereignisgesteuert\n- BUSY bleibt zusätzliche Hardwarediagnose: eine unbestätigte Polarität verhindert weder funktionierende Wiedergabe noch einen erfolgreichen UART-End-to-End-Test\n- funktionierender PLAY-Pfad, Boot-Ton, Rotation, Lautstärke und normale Unterbrechungstöne bleiben unverändert\n\n''' + text
write(path, text)

# Current-version labels in README variants only; keep historical release sections untouched.
for path in ("README.md", "docs/de/README.md", "docs/en/README.md", "docs/swg/README.md"):
    text = read(path)
    if "`3.3.1`" in text:
        text = text.replace("`3.3.1`", "`3.3.2`", 1)
    write(path, text)

print("3.3.2 patch applied")
