#!/usr/bin/env python3
from pathlib import Path
import re

path = Path(__file__).resolve().parents[1] / "ui-src" / "app.js"
text = path.read_text(encoding="utf-8")
# Translation entries may share a source line with ota.file, so remove the
# property itself rather than assuming one key per line.
text = re.sub(r"\s*['\"]ota\.hint['\"]\s*:\s*(['\"])(?:(?!\1).)*?\1\s*,?", "", text)
text = text.replace(", hintKey: 'ota.hint'", "")
text = text.replace("hintKey: 'ota.hint', ", "")
if "ota.hint" in text or "Export Compiled Binary" in text:
    raise SystemExit("obsolete OTA hint still present")
path.write_text(text, encoding="utf-8")
print("OTA sketch-BIN hint fully removed")
