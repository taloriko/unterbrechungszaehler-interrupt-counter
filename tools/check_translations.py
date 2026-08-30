#!/usr/bin/env python3
"""Validate all registered web UI languages and language-independent rendering."""

from pathlib import Path
import re
import sys

UI_DIR = Path("arduino/Unterbrechungszaehler")
BASE_FILE = UI_DIR / "WebUi.h"
BEHAVIOR_FILE = UI_DIR / "WebUiBehavior.h"
UI_FILES = sorted(UI_DIR.glob("WebUi*.h"))

base_text = BASE_FILE.read_text(encoding="utf-8")
behavior_text = BEHAVIOR_FILE.read_text(encoding="utf-8")
key_pattern = re.compile(r"['\"]([^'\"]+)['\"]\s*:")


def matching_brace(text: str, opening: int) -> int:
    depth = 0
    quote = None
    escaped = False
    for index in range(opening, len(text)):
        char = text[index]
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in ("'", '"'):
            quote = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError("unclosed object")


def extract_language_blocks(text: str, variable: str) -> dict[str, set[str]]:
    marker = re.search(rf"const\s+{re.escape(variable)}\s*=\s*\{{", text)
    if not marker:
        raise ValueError(f"{variable} could not be parsed")
    outer_open = text.find("{", marker.start())
    outer_close = matching_brace(text, outer_open)
    body = text[outer_open + 1 : outer_close]

    starts = list(re.finditer(r"(?:^|,)\s*([A-Za-z][A-Za-z0-9_-]*)\s*:\s*\{", body))
    if not starts:
        raise ValueError(f"{variable} has no languages")

    blocks: dict[str, set[str]] = {}
    for match in starts:
        code = match.group(1)
        opening = body.find("{", match.start())
        closing = matching_brace(body, opening)
        blocks[code] = set(key_pattern.findall(body[opening + 1 : closing]))
    return blocks


def compare_language_keys(name: str, blocks: dict[str, set[str]], errors: list[str]):
    if not blocks:
        return
    reference_code = "de" if "de" in blocks else next(iter(blocks))
    reference = blocks[reference_code]
    for language, keys in sorted(blocks.items()):
        difference = sorted(reference ^ keys)
        if difference:
            errors.append(
                f"{name}: {reference_code} and {language} keys differ: "
                + ", ".join(difference)
            )


errors: list[str] = []

try:
    base_keys = extract_language_blocks(base_text, "I18N")
except ValueError as exc:
    errors.append(str(exc))
    base_keys = {}

try:
    extra_keys = extract_language_blocks(behavior_text, "EXTRA_I18N")
except ValueError as exc:
    errors.append(str(exc))
    extra_keys = {}

if set(base_keys) != set(extra_keys):
    errors.append(
        "Core and extension language sets differ: "
        + ", ".join(sorted(set(base_keys) ^ set(extra_keys)))
    )

compare_language_keys("Core translations", base_keys, errors)
compare_language_keys("Extension translations", extra_keys, errors)

reference_language = "de" if "de" in base_keys else next(iter(base_keys), "")
if reference_language:
    used_core = set(re.findall(r'data-i18n="([^"]+)"', base_text))
    used_core.update(re.findall(r"\btr\('([^']+)'", base_text))
    used_core.update(re.findall(r'\btr\("([^"]+)"', base_text))
    missing_core = sorted(used_core - base_keys[reference_language])
    if missing_core:
        errors.append("UI keys missing from core translations: " + ", ".join(missing_core))

combined_extensions = "\n".join(
    path.read_text(encoding="utf-8") for path in UI_FILES if path != BASE_FILE
)
if extra_keys:
    used_extra = set(re.findall(r"uiText\('([^']+)'", combined_extensions))
    used_extra.update(re.findall(r"uicTr\('([^']+)'", combined_extensions))
    used_extra.update(re.findall(r"\btr\('([^']+)'", combined_extensions))
    used_extra.update(re.findall(r"\bxtr\('([^']+)'", base_text))
    reference_extra = extra_keys.get("de", next(iter(extra_keys.values())))
    missing_extra = sorted(used_extra - reference_extra)
    if missing_extra:
        errors.append("UI keys missing from extension translations: " + ", ".join(missing_extra))

meta_match = re.search(r"const\s+META\s*=\s*\{(.*?)\};", behavior_text, re.S)
if not meta_match:
    errors.append("META could not be parsed")
else:
    meta_codes = set(re.findall(r"(?:^|,)\s*([A-Za-z][A-Za-z0-9_-]*)\s*:\s*\{", meta_match.group(1)))
    if meta_codes != set(extra_keys):
        errors.append(
            "Language metadata differs from translation languages: "
            + ", ".join(sorted(meta_codes ^ set(extra_keys)))
        )

for path in UI_FILES:
    text = path.read_text(encoding="utf-8")
    if "translateDynamicText" in text:
        errors.append(f"{path.name}: text-to-text translation is not allowed")
    if re.search(r"applySwabian|scheduleSwabian", text, re.I):
        errors.append(f"{path.name}: language-specific render helper is not allowed")
    if re.search(
        r"(?:\blang\b|document\.documentElement\.lang)\s*(?:===|!==)\s*['\"][A-Za-z][A-Za-z0-9_-]*['\"]",
        text,
    ):
        errors.append(f"{path.name}: direct language comparison in render code is not allowed")
    if re.search(r"const\s+(?:text|txt)\s*=\s*\(de\s*,\s*en\)", text):
        errors.append(f"{path.name}: two-language text helper is not allowed")

if errors:
    for error in errors:
        print(error)
    sys.exit(1)

languages = sorted(base_keys)
reference = "de" if "de" in base_keys else languages[0]
print(
    f"OK: {len(base_keys[reference])} core keys, "
    f"{len(extra_keys[reference])} extension keys, "
    f"{len(languages)} languages ({', '.join(languages)}) complete and structurally identical."
)
