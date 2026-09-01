#!/usr/bin/env python3
"""Validate all registered web UI languages and language-independent rendering."""

from pathlib import Path
import re
import sys

UI_DIR = Path("arduino/Unterbrechungszaehler")
BASE_FILE = UI_DIR / "WebUi.h"
BEHAVIOR_FILE = UI_DIR / "WebUiBehavior.h"
V2_FILE = UI_DIR / "WebUiV2.h"
UI_FILES = sorted(UI_DIR.glob("WebUi*.h"))

base_text = BASE_FILE.read_text(encoding="utf-8")
behavior_text = BEHAVIOR_FILE.read_text(encoding="utf-8")
v2_text = V2_FILE.read_text(encoding="utf-8") if V2_FILE.exists() else ""
quoted_key_pattern = re.compile(r"['\"]([^'\"]+)['\"]\s*:")
bare_entry_pattern = re.compile(
    r"(?:^|,)\s*(?:['\"]([^'\"]+)['\"]|([A-Za-z_][A-Za-z0-9_]*))\s*:"
)


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


def extract_language_blocks(
    text: str, variable: str, allow_bare_keys: bool = False
) -> dict[str, set[str]]:
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
        language_body = body[opening + 1 : closing]
        if allow_bare_keys:
            matches = bare_entry_pattern.findall(language_body)
            blocks[code] = {quoted or bare for quoted, bare in matches}
        else:
            blocks[code] = set(quoted_key_pattern.findall(language_body))
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

v2_keys: dict[str, set[str]] = {}
if v2_text:
    try:
        v2_keys = extract_language_blocks(v2_text, "T", allow_bare_keys=True)
    except ValueError as exc:
        errors.append(str(exc))

if set(base_keys) != set(extra_keys):
    errors.append(
        "Core and extension language sets differ: "
        + ", ".join(sorted(set(base_keys) ^ set(extra_keys)))
    )
if v2_keys and set(base_keys) != set(v2_keys):
    errors.append(
        "Core and 2.0 language sets differ: "
        + ", ".join(sorted(set(base_keys) ^ set(v2_keys)))
    )

compare_language_keys("Core translations", base_keys, errors)
compare_language_keys("Extension translations", extra_keys, errors)
compare_language_keys("2.0 translations", v2_keys, errors)

reference_language = "de" if "de" in base_keys else next(iter(base_keys), "")
if reference_language:
    used_core = set(re.findall(r'data-i18n="([^"]+)"', base_text))
    used_core.update(re.findall(r"\btr\('([^']+)'", base_text))
    used_core.update(re.findall(r'\btr\("([^"]+)"', base_text))
    missing_core = sorted(used_core - base_keys[reference_language])
    if missing_core:
        errors.append("UI keys missing from core translations: " + ", ".join(missing_core))

combined_extensions = "\n".join(
    path.read_text(encoding="utf-8")
    for path in UI_FILES
    if path not in (BASE_FILE, V2_FILE)
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

if v2_keys:
    used_v2 = set(re.findall(r'data-v2="([^"]+)"', v2_text))
    used_v2.update(re.findall(r"\btr\('([^']+)'", v2_text))
    reference_v2 = v2_keys.get("de", next(iter(v2_keys.values())))
    missing_v2 = sorted(used_v2 - reference_v2)
    if missing_v2:
        errors.append("UI keys missing from 2.0 translations: " + ", ".join(missing_v2))

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
message = (
    f"OK: {len(base_keys[reference])} core keys, "
    f"{len(extra_keys[reference])} extension keys"
)
if v2_keys:
    message += f", {len(v2_keys[reference])} 2.0 keys"
message += f", {len(languages)} languages ({', '.join(languages)}) complete and structurally identical."
print(message)
