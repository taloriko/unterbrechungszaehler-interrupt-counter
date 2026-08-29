#!/usr/bin/env python3
"""Prueft die Sprachschluessel der modularen Weboberflaeche."""

from pathlib import Path
import re
import sys

path = Path("arduino/UnterbrechungszaehlerModular/WebUi.h")
text = path.read_text(encoding="utf-8")

used = set(re.findall(r'data-i18n="([^"]+)"', text))
used.update(re.findall(r"tr\('([^']+)'\)", text))
used.update(re.findall(r'tr\("([^"]+)"\)', text))

marker = "const I18N={de:{"
start = text.find(marker)
if start < 0:
    print("I18N-Tabelle nicht gefunden")
    sys.exit(1)

english_marker = "},en:{"
mid = text.find(english_marker, start)
end = text.find("}};", mid)
if mid < 0 or end < 0:
    print("I18N-Bloecke konnten nicht gelesen werden")
    sys.exit(1)

de_block = text[start + len(marker):mid]
en_block = text[mid + len(english_marker):end]
key_pattern = re.compile(r"'([^']+)'\s*:")
de = set(key_pattern.findall(de_block))
en = set(key_pattern.findall(en_block))

missing_de = sorted(used - de)
missing_en = sorted(used - en)
extra_difference = sorted(de ^ en)

if missing_de or missing_en or extra_difference:
    if missing_de:
        print("Fehlt Deutsch:", ", ".join(missing_de))
    if missing_en:
        print("Fehlt Englisch:", ", ".join(missing_en))
    if extra_difference:
        print("Sprachbloecke unterscheiden sich:", ", ".join(extra_difference))
    sys.exit(1)

print(f"OK: {len(used)} verwendete Sprachschluessel, DE/EN vollstaendig.")
