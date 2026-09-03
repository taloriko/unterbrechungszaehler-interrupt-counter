#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def patch(name: str, old: str, new: str, count: int = 1):
    path = ROOT / name
    text = path.read_text(encoding="utf-8")
    if text.count(old) != count:
        raise SystemExit(f"marker mismatch {name}: {old[:80]!r} count={text.count(old)}")
    path.write_text(text.replace(old, new, count), encoding="utf-8")

# Forward-declare the nonblocking volume application before finishProbe() uses it.
path = ROOT / "audio_dy_sv17f.cpp"
text = path.read_text(encoding="utf-8")
late = "void updateBusyPin();\nbool applyDesiredVolume();\n\nvoid handleFrame"
if late not in text:
    raise SystemExit("late audio declaration marker missing")
text = text.replace(late, "void updateBusyPin();\n\nvoid handleFrame", 1)
early = "void sendQuery(WaitKind kind, uint8_t command) {\n  sendFrame(command);\n  startWait(kind, command);\n}\n"
if early not in text:
    raise SystemExit("audio declaration insertion marker missing")
text = text.replace(early, early + "\nbool applyDesiredVolume();\n", 1)
path.write_text(text, encoding="utf-8")

# If absolute calendar time disappears, don't leave a stale day-average next to
# a reset today counter.
patch(
    "interruption_service.cpp",
    "      currentDayValid = false;\n      currentSummary.todayCount = 0;\n      bumpRevision();",
    "      currentDayValid = false;\n      currentSummary.todayCount = 0;\n      currentSummary.todayIntervalSumSeconds = 0;\n      currentSummary.todayIntervalSamples = 0;\n      bumpRevision();",
)

# Be explicit about strcmp/strncmp used for the boot language selection.
patch("Unterbrechungszaehler.ino", "#include <Arduino.h>\n", "#include <Arduino.h>\n#include <cstring>\n")

print("3.2.0 C++ polish applied")
