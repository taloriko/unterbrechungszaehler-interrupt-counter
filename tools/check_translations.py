#!/usr/bin/env python3
"""Validate the complete DE/EN/SWG web UI translation tables."""

from pathlib import Path
import re
import sys

UI_DIR = Path("arduino/Unterbrechungszaehler")
BASE_FILE = UI_DIR / "WebUi.h"
BEHAVIOR_FILE = UI_DIR / "WebUiBehavior.h"
LANGUAGES = ("de", "en", "swg")

base_text = BASE_FILE.read_text(encoding="utf-8")
behavior_text = BEHAVIOR_FILE.read_text(encoding="utf-8")
key_pattern = re.compile(r"['\"]([^'\"]+)['\"]\s*:")


def extract_three_language_blocks(text: str, variable: str):
    pattern = (
        rf"const\s+{re.escape(variable)}\s*=\s*\{{\s*"
        r"de\s*:\s*\{(.*?)\}\s*,\s*"
        r"en\s*:\s*\{(.*?)\}\s*,\s*"
        r"swg\s*:\s*\{(.*?)\}\s*\}\s*;"
    )
    match = re.search(pattern, text, re.S)
    if not match:
        raise ValueError(f"{variable} could not be parsed")
    return {
        "de": set(key_pattern.findall(match.group(1))),
        "en": set(key_pattern.findall(match.group(2))),
        "swg": set(key_pattern.findall(match.group(3))),
    }


def compare_language_keys(name: str, blocks: dict[str, set[str]], errors: list[str]):
    reference = blocks["de"]
    for language in LANGUAGES[1:]:
        difference = sorted(reference ^ blocks[language])
        if difference:
            errors.append(
                f"{name}: German and {language} keys differ: " + ", ".join(difference)
            )


errors = []

try:
    base_keys = extract_three_language_blocks(base_text, "I18N")
except ValueError as exc:
    errors.append(str(exc))
    base_keys = {language: set() for language in LANGUAGES}

try:
    extra_keys = extract_three_language_blocks(behavior_text, "EXTRA_I18N")
except ValueError as exc:
    errors.append(str(exc))
    extra_keys = {language: set() for language in LANGUAGES}

compare_language_keys("Core translations", base_keys, errors)
compare_language_keys("Extension translations", extra_keys, errors)

used = set(re.findall(r'data-i18n="([^"]+)"', base_text))
used.update(re.findall(r"tr\('([^']+)'", base_text))
used.update(re.findall(r'tr\("([^"]+)"', base_text))
missing_base = sorted(used - base_keys["de"])
if missing_base:
    errors.append("UI keys missing from core translations: " + ", ".join(missing_base))

meta_match = re.search(r"const\s+META\s*=\s*\{(.*?)\};", behavior_text, re.S)
if not meta_match:
    errors.append("META could not be parsed")
else:
    meta = meta_match.group(1)
    for language in LANGUAGES:
        if not re.search(rf"\b{language}\s*:\s*\{{", meta):
            errors.append(f"Language {language} has no metadata")

if errors:
    for error in errors:
        print(error)
    sys.exit(1)

print(
    f"OK: {len(base_keys['de'])} core keys, "
    f"{len(extra_keys['de'])} extension keys, "
    f"{len(LANGUAGES)} languages complete and structurally identical."
)
