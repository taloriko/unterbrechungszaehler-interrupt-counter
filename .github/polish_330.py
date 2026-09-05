from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
app = ROOT / 'Unterbrechungszaehler/ui-src/app.js'
s = app.read_text(encoding='utf-8')
marker = "  const LANGUAGE_LABELS = {"
addition = r'''  const STORAGE_STATUS_LABELS = {
    de: { 'status.ready': 'Bereit', 'status.unavailable': 'Nicht verfügbar' },
    en: { 'status.ready': 'Ready', 'status.unavailable': 'Unavailable' },
    it: { 'status.ready': 'Pronto', 'status.unavailable': 'Non disponibile' },
    fr: { 'status.ready': 'Prêt', 'status.unavailable': 'Indisponible' },
    swg: { 'status.ready': 'Bereit', 'status.unavailable': 'Net verfügbar' },
    'swg-alb': { 'status.ready': 'Bereit', 'status.unavailable': 'Net verfügbar' },
    'swg-ob': { 'status.ready': 'Bereit', 'status.unavailable': 'It verfügbar' }
  };
  Object.entries(STORAGE_STATUS_LABELS).forEach(([code, labels]) => Object.assign(I18N[code], labels));

'''
if 'const STORAGE_STATUS_LABELS' not in s:
    pos = s.find(marker)
    if pos < 0:
        raise SystemExit('language marker missing')
    s = s[:pos] + addition + s[pos:]
app.write_text(s, encoding='utf-8')

rc = ROOT / 'Unterbrechungszaehler/tools/release_check.py'
r = rc.read_text(encoding='utf-8')
anchor = '    check("databaseDeletePassword" in JS and "eraseDatabase" in JS, "password-confirmed database erase UI")\n'
extra = '    check("status.ready" in JS and "status.unavailable" in JS, "translated storage health states")\n'
if extra not in r:
    if anchor not in r:
        raise SystemExit('release-check polish anchor missing')
    r = r.replace(anchor, anchor + extra, 1)
rc.write_text(r, encoding='utf-8')
print('3.3.0 polish applied')
