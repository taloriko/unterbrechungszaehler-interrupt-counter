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


def extract_json_assignment(text: str, name: str, next_name: str):
    pattern = rf"const {re.escape(name)}=(\{{.*?\}});\s*const {re.escape(next_name)}="
    match = re.search(pattern, text, re.S)
    if not match:
        raise ValueError(f"{name} could not be parsed")
    return json.loads(match.group(1))


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

try:
    extra_base = extract_json_assignment(behavior_text, "EXTRA_BASE", "LANGUAGE_PACKS")
except (ValueError, json.JSONDecodeError) as exc:
    errors.append(str(exc))
    extra_base = {"de": {}, "en": {}}

try:
    packs = extract_json_assignment(behavior_text, "LANGUAGE_PACKS", "META")
except (ValueError, json.JSONDecodeError) as exc:
    errors.append(str(exc))
    packs = {}

extra_de = set(extra_base.get("de", {}))
extra_en = set(extra_base.get("en", {}))
if extra_de != extra_en:
    errors.append("German and English dynamic keys differ: " + ", ".join(sorted(extra_de ^ extra_en)))

required_keys = de_keys | extra_de

for code, pack in sorted(packs.items()):
    strings = pack.get("strings", {})
    missing = sorted(required_keys - set(strings))
    extra = sorted(set(strings) - required_keys)
    if missing:
        errors.append(f"Language {code} is incomplete: " + ", ".join(missing))
    if extra:
        errors.append(f"Language {code} has unknown keys: " + ", ".join(extra))
    if not pack.get("label"):
        errors.append(f"Language {code} has no label")
    if not pack.get("locale"):
        errors.append(f"Language {code} has no locale")

if errors:
    for error in errors:
        print(error)
    sys.exit(1)

print(
    f"OK: {len(de_keys)} static keys, {len(extra_de)} dynamic keys, "
    f"{2 + len(packs)} languages complete."
)
