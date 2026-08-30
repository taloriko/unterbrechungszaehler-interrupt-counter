#!/usr/bin/env python3
"""Validate all registered web UI language packs."""

from pathlib import Path
import json
import re
import sys

UI_DIR = Path("arduino/Unterbrechungszaehler")
BASE_FILE = UI_DIR / "WebUi.h"
BEHAVIOR_FILE = UI_DIR / "WebUiBehavior.h"

base_text = BASE_FILE.read_text(encoding="utf-8")
behavior_text = BEHAVIOR_FILE.read_text(encoding="utf-8")

used = set(re.findall(r'data-i18n="([^"]+)"', base_text))
used.update(re.findall(r"tr\('([^']+)'", base_text))
used.update(re.findall(r'tr\("([^"]+)"', base_text))

start_marker = "const I18N={de:{"
english_marker = "},en:{"
start = base_text.find(start_marker)
mid = base_text.find(english_marker, start)
end = base_text.find("}};", mid)
if start < 0 or mid < 0 or end < 0:
    print("Base language table could not be parsed")
    sys.exit(1)

key_pattern = re.compile(r"'([^']+)'\s*:")
de_block = base_text[start + len(start_marker):mid]
en_block = base_text[mid + len(english_marker):end]
de_keys = set(key_pattern.findall(de_block))
en_keys = set(key_pattern.findall(en_block))

errors = []
if de_keys != en_keys:
    errors.append("German and English base keys differ: " + ", ".join(sorted(de_keys ^ en_keys)))

missing_base = sorted(used - de_keys)
if missing_base:
    errors.append("UI keys missing from base languages: " + ", ".join(missing_base))

match = re.search(r"const LANGUAGE_PACKS=(\{.*?\});\s*const EXACT=", behavior_text, re.S)
if not match:
    errors.append("LANGUAGE_PACKS could not be parsed")
    packs = {}
else:
    try:
        packs = json.loads(match.group(1))
    except json.JSONDecodeError as exc:
        errors.append(f"LANGUAGE_PACKS is not valid JSON: {exc}")
        packs = {}

action_keys = set(re.findall(r"'(action\.[^']+)'\s*:", behavior_text))
required_keys = de_keys | action_keys

for code, pack in sorted(packs.items()):
    strings = pack.get("strings", {})
    missing = sorted(required_keys - set(strings))
    if missing:
        errors.append(f"Language {code} is incomplete: " + ", ".join(missing))
    if not pack.get("label"):
        errors.append(f"Language {code} has no label")
    if not pack.get("locale"):
        errors.append(f"Language {code} has no locale")

if errors:
    for error in errors:
        print(error)
    sys.exit(1)

print(
    f"OK: {len(de_keys)} base keys, {len(action_keys)} action keys, "
    f"{2 + len(packs)} languages complete."
)
