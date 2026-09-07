from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def patch(rel, old, new):
    path = ROOT / rel
    text = path.read_text(encoding="utf-8")
    if old not in text:
        raise SystemExit(f"missing anchor: {rel}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")

patch(
    "Unterbrechungszaehler/audio_dy_sv17f.cpp",
    '''  if (waitingFor == WaitKind::VerifyPlay) {
    waitingFor = WaitKind::None;
    verifyExpectation = VerifyExpectation::Any;
  }
  if (deferredAction == DeferredAction::VerifyPlay) deferredAction = DeferredAction::None;
  sendPlayCommand(trackNumber);
  return true;
}''',
    '''  bool replacedNormalVerification = false;
  if (waitingFor == WaitKind::VerifyPlay) {
    waitingFor = WaitKind::None;
    verifyExpectation = VerifyExpectation::Any;
    replacedNormalVerification = true;
  }
  if (deferredAction == DeferredAction::VerifyPlay) {
    deferredAction = DeferredAction::None;
    replacedNormalVerification = true;
  }
  sendPlayCommand(trackNumber);
  // scheduleVerify() marks health as Checking. A deliberately superseded normal
  // verification must not leave that diagnostic state stuck forever.
  if (replacedNormalVerification && isDetected) setHealth(StatusRegistry::State::Ok);
  return true;
}''',
)

patch(
    "Unterbrechungszaehler/interruption_service.cpp",
    '''  if (ProjectPreferences::soundMode() != ProjectPreferences::SoundMode::Rotate) {
    return ProjectPreferences::soundTrack() >= firstNormal ? ProjectPreferences::soundTrack() : firstNormal;
  }

  const uint16_t count = AudioDySv17f::musicCount();''',
    '''  const uint16_t count = AudioDySv17f::musicCount();
  if (ProjectPreferences::soundMode() != ProjectPreferences::SoundMode::Rotate) {
    if (count > 0U && count < firstNormal) return 0U;
    return ProjectPreferences::soundTrack() >= firstNormal ? ProjectPreferences::soundTrack() : firstNormal;
  }

''',
)

# Strengthen release assertions for both edge cases.
path = ROOT / "Unterbrechungszaehler/tools/release_check.py"
text = path.read_text(encoding="utf-8")
anchor = '    check("playPriorityFeedbackTrack(ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK)" in service_cpp, "track 2 fast feedback on suppressed press")\n'
extra = ('    check("replacedNormalVerification" in audio_cpp and "setHealth(StatusRegistry::State::Ok)" in audio_cpp, "priority spam track cannot leave normal verification stuck in Checking")\n'
         '    check("count > 0U && count < firstNormal" in service_cpp and "return 0U" in service_cpp, "known modules with only reserved tracks do not receive nonexistent normal track 3")\n')
if anchor not in text:
    raise SystemExit("release-check anchor missing")
text = text.replace(anchor, anchor + extra, 1)
path.write_text(text, encoding="utf-8")

print("3.4.0 hardening applied")
