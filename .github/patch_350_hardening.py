#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "Unterbrechungszaehler"
APP = FW / "ui-src" / "app.js"
RC = FW / "tools" / "release_check.py"


def replace_once(path, old, new):
    text = path.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise RuntimeError(f"anchor count for {path}: {text.count(old)}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")

# Add a localized label for the actual per-result coverage count in all seven 3.5 overlays.
replacements = {
    "'patterns.days': '{n} ausgewertete Tage', 'patterns.insufficient':": "'patterns.days': '{n} ausgewertete Tage', 'patterns.coveredDays': '{n} abgedeckte Tage', 'patterns.insufficient':",
    "'patterns.days': '{n} evaluated days', 'patterns.insufficient':": "'patterns.days': '{n} evaluated days', 'patterns.coveredDays': '{n} covered days', 'patterns.insufficient':",
    "'patterns.days': '{n} giorni analizzati', 'patterns.insufficient':": "'patterns.days': '{n} giorni analizzati', 'patterns.coveredDays': '{n} giorni coperti', 'patterns.insufficient':",
    "'patterns.days': '{n} jours évalués', 'patterns.insufficient':": "'patterns.days': '{n} jours évalués', 'patterns.coveredDays': '{n} jours couverts', 'patterns.insufficient':",
    "'patterns.days': '{n} ausgwertete Däg', 'patterns.insufficient':": "'patterns.days': '{n} ausgwertete Däg', 'patterns.coveredDays': '{n} abdeckte Däg', 'patterns.insufficient':",
}
text = APP.read_text(encoding="utf-8")
for old, new in list(replacements.items())[:4]:
    if text.count(old) != 1:
        raise RuntimeError(f"translation anchor count {old!r}: {text.count(old)}")
    text = text.replace(old, new, 1)
swg_old = "'patterns.days': '{n} ausgwertete Däg', 'patterns.insufficient':"
swg_new = "'patterns.days': '{n} ausgwertete Däg', 'patterns.coveredDays': '{n} abdeckte Däg', 'patterns.insufficient':"
if text.count(swg_old) != 3:
    raise RuntimeError(f"swg coverage anchors: {text.count(swg_old)}")
text = text.replace(swg_old, swg_new)
APP.write_text(text, encoding="utf-8")

replace_once(
    APP,
    "    const quietValue = el('strong', 'focus-insight-value'); quiet.append(quietLabel, quietValue);",
    "    const quietValue = el('strong', 'focus-insight-value');\n"
    "    const quietMeta = el('small', 'focus-insight-meta'); quiet.append(quietLabel, quietValue, quietMeta);"
)
replace_once(
    APP,
    "    const peakValue = el('strong', 'focus-insight-value'); peak.append(peakLabel, peakValue);",
    "    const peakValue = el('strong', 'focus-insight-value');\n"
    "    const peakMeta = el('small', 'focus-insight-meta'); peak.append(peakLabel, peakValue, peakMeta);"
)
replace_once(
    APP,
    "      quietValue.textContent = patterns.quietSufficient ? patternRange(patterns.quietStartHour, patterns.quietEndHour) : t('patterns.insufficient');\n"
    "      peakValue.textContent = patterns.peakSufficient ? patternRange(patterns.peakStartHour, patterns.peakEndHour) : t('patterns.insufficient');\n"
    "      basisValue.textContent = t('patterns.days').replace('{n}', String(Number(patterns.evaluatedDays || 0)));",
    "      quietValue.textContent = patterns.quietSufficient ? patternRange(patterns.quietStartHour, patterns.quietEndHour) : t('patterns.insufficient');\n"
    "      quietMeta.textContent = patterns.quietSufficient ? t('patterns.coveredDays').replace('{n}', String(Number(patterns.quietCoveredDays || 0))) : '';\n"
    "      peakValue.textContent = patterns.peakSufficient ? patternRange(patterns.peakStartHour, patterns.peakEndHour) : t('patterns.insufficient');\n"
    "      peakMeta.textContent = patterns.peakSufficient ? t('patterns.coveredDays').replace('{n}', String(Number(patterns.peakCoveredDays || 0))) : '';\n"
    "      basisValue.textContent = t('patterns.days').replace('{n}', String(Number(patterns.evaluatedDays || 0)));"
)
replace_once(
    APP,
    "'interruptions.patterns.quietEndHour','interruptions.patterns.peakSufficient','interruptions.patterns.peakStartHour','interruptions.patterns.peakEndHour'",
    "'interruptions.patterns.quietEndHour','interruptions.patterns.quietCoveredDays','interruptions.patterns.peakSufficient','interruptions.patterns.peakStartHour','interruptions.patterns.peakEndHour','interruptions.patterns.peakCoveredDays'"
)

css = FW / "ui-src" / "app.css"
css_text = css.read_text(encoding="utf-8")
if '.focus-insight-meta {' not in css_text:
    css_text += "\n.focus-insight-meta { display: block; margin-top: 5px; color: var(--muted); font-size: .72rem; }\n"
css.write_text(css_text, encoding="utf-8")

rc = RC.read_text(encoding="utf-8")
anchor = '    check("trendPrevious60" in JS and "trendLast60" in JS and "focus.explain" in JS, "explained 120-minute trend on Home")\n'
extra = (
    '    check("quietCoveredDays" in JS and "peakCoveredDays" in JS and "patterns.coveredDays" in JS, "work-pattern results expose actual covered-day basis")\n'
    '    check("if (scanning) return stableDuringScan" in insights_cpp and "timeValidityChanged" in insights_cpp, "Focus scan is reentrancy-safe and reacts to time validity changes")\n'
    '    check("Preferences" not in insights_cpp and "LittleFS" not in insights_cpp and "appendRaw" not in insights_cpp and "writeSequence" not in insights_cpp, "Focus & Insights adds no persistent storage writes")\n'
)
if rc.count(anchor) != 1:
    raise RuntimeError('release-check insertion anchor missing')
rc = rc.replace(anchor, anchor + extra, 1)
RC.write_text(rc, encoding="utf-8")

print('final 3.5.0 hardening applied')
