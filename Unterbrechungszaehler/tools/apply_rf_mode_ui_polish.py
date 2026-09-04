from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def patch(path, old, new, label):
    p = ROOT / path
    s = p.read_text(encoding="utf-8")
    if s.count(old) != 1:
        raise SystemExit(f"{label}: anchor count={s.count(old)}")
    p.write_text(s.replace(old, new, 1), encoding="utf-8")

patch(
    "ui-src/app.js",
    "    rfTest(value) { return t(`rf433.test.${value || 'idle'}`); },\n",
    "    rfTest(value) { return t(`rf433.test.${value || 'idle'}`); },\n    rfMode(value) { return t(`rf433.mode.${value === 'somfy' ? 'somfy' : 'universal'}`); },\n",
    "RF mode formatter",
)
patch(
    "tools/release_check.py",
    '    check("rfMode" in JS and "\'rf433.mode.somfy\'" in JS and "\'rf433.mode.universal\'" in JS, "RF mode selector UI")\n',
    '    check("rfMode" in JS and "\'rf433.mode.somfy\'" in JS and "\'rf433.mode.universal\'" in JS and "rfMode(value)" in JS, "RF mode selector UI")\n',
    "RF mode UI check",
)
print("RF mode UI polish applied")
