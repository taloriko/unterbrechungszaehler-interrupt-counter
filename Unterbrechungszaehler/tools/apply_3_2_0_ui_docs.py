#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def replace_once(path: Path, old: str, new: str) -> None:
    text = read(path)
    if text.count(old) != 1:
        raise SystemExit(f"marker mismatch in {path}: {old[:100]!r} count={text.count(old)}")
    write(path, text.replace(old, new, 1))


APP = ROOT / "ui-src" / "app.js"
CSS = ROOT / "ui-src" / "app.css"
js = read(APP)

# Remove the Arduino IDE sketch-BIN instruction from every language and from
# the upload component itself. The release provides the OTA BIN directly.
js = re.sub(r"^\s*['\"]ota\.hint['\"]\s*:\s*.*?\n", "", js, flags=re.M)
js = js.replace(", hintKey: 'ota.hint'", "")
js = js.replace(
    "    const hint = el('div', 'form-note');\n    hint.textContent = t(def.hintKey);\n\n",
    "",
    1,
)
js = js.replace("    root.append(label, input, hint, selected, actions, progressWrap, status);", "    root.append(label, input, selected, actions, progressWrap, status);", 1)

# Device-side additions use Object.assign so the existing large base translation
# objects remain readable. Every supported UI variant gets explicit values.
translation_patch = r'''

  Object.assign(I18N.de, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Feedback',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärke',
    'project.displayRotation180': 'Display um 180° drehen',
    'project.displayMode.dayProgress': 'Tagesfortschritt – Heute + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit letzter Unterbrechung',
    'project.settings.title': 'Hardware-Feedback',
    'project.settings.desc': 'Display, Display-Feedback und Ton sind getrennt gruppiert. Änderungen gelten sofort und bleiben im ESP32 gespeichert.',
    'card.language.desc': 'Die Sprache gilt für Weboberfläche und OLED und wird auf dem ESP32 gespeichert.'
  });
  Object.assign(I18N.en, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display feedback',
    'project.section.sound': 'Sound / DY-SV17F',
    'project.soundVolume': 'Volume',
    'project.displayRotation180': 'Rotate display 180°',
    'project.displayMode.dayProgress': 'Day progress – today + average interval',
    'project.displayMode.focus': 'Focus – time since last interruption',
    'project.settings.title': 'Hardware feedback',
    'project.settings.desc': 'Display, display feedback and sound are grouped separately. Changes apply immediately and remain stored on the ESP32.',
    'card.language.desc': 'The language applies to both the web interface and OLED and is stored on the ESP32.'
  });
  Object.assign(I18N.it, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Feedback display',
    'project.section.sound': 'Audio / DY-SV17F',
    'project.soundVolume': 'Volume',
    'project.displayRotation180': 'Ruota display di 180°',
    'project.displayMode.dayProgress': 'Progresso giornaliero – oggi + intervallo medio',
    'project.displayMode.focus': 'Focus – tempo dall’ultima interruzione',
    'project.settings.title': 'Feedback hardware',
    'project.settings.desc': 'Display, feedback del display e audio sono raggruppati separatamente. Le modifiche vengono applicate subito e salvate nell’ESP32.',
    'card.language.desc': 'La lingua vale per interfaccia web e OLED e viene salvata nell’ESP32.'
  });
  Object.assign(I18N.fr, {
    'project.section.display': 'Affichage',
    'project.section.displayFeedback': 'Retour d’affichage',
    'project.section.sound': 'Son / DY-SV17F',
    'project.soundVolume': 'Volume',
    'project.displayRotation180': 'Tourner l’affichage de 180°',
    'project.displayMode.dayProgress': 'Progression du jour – aujourd’hui + intervalle moyen',
    'project.displayMode.focus': 'Focus – temps depuis la dernière interruption',
    'project.settings.title': 'Retour matériel',
    'project.settings.desc': 'Affichage, retour visuel et son sont regroupés séparément. Les modifications sont immédiates et enregistrées sur l’ESP32.',
    'card.language.desc': 'La langue s’applique à l’interface web et à l’OLED et est enregistrée sur l’ESP32.'
  });
  Object.assign(I18N.swg, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Rückmeldung',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärk',
    'project.displayRotation180': 'Display um 180° dreha',
    'project.displayMode.dayProgress': 'Tagesverlauf – heit + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit dr letschta Unterbrechung',
    'project.settings.title': 'Hardware-Rückmeldung',
    'project.settings.desc': 'Display, Display-Rückmeldung ond Ton send sauber trennt. Ändrunga geltet glei ond bleibet em ESP32 gspeichert.',
    'card.language.desc': 'D Sproch gilt fürs Web ond fürs OLED ond wird em ESP32 gspeichert.'
  });
  Object.assign(I18N['swg-alb'], {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Rückmeldung',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärk',
    'project.displayRotation180': 'Display um 180° dreha',
    'project.displayMode.dayProgress': 'Tagesverlauf – heit + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit dr letschta Unterbrechung',
    'project.settings.title': 'Hardware-Rückmeldung',
    'project.settings.desc': 'Display, Display-Rückmeldung ond Ton send extra gruppiert. Ändrunga geltet glei ond bleibet gspeichert.',
    'card.language.desc': 'D Sproch gilt fürs Web ond OLED ond bleibt em ESP32 gspeichert.'
  });
  Object.assign(I18N['swg-ob'], {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Rückmeldung',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärk',
    'project.displayRotation180': 'Display um 180° dreha',
    'project.displayMode.dayProgress': 'Tagesverlauf – heit + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit dr letschta Unterbrechung',
    'project.settings.title': 'Hardware-Rückmeldung',
    'project.settings.desc': 'Display, Display-Rückmeldung ond Ton send getrennt gruppiert. Ändrunga geltet sofort ond bleibet gspeichert.',
    'card.language.desc': 'D Sproch gilt fürs Web ond OLED ond bleibt em ESP32 gspeichert.'
  });
'''
state_marker = "\n  const state = {"
if js.count(state_marker) != 1:
    raise SystemExit("app state marker mismatch")
js = js.replace(state_marker, translation_patch + state_marker, 1)

old_state = "    projectSettings: { soundEnabled: true, soundMode: 'fixed', soundTrack: 2, soundTrackCount: 0, displayEnabled: true, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 50, displayDimAfterMinutes: 10, displayDimBrightness: 10 },"
new_state = "    projectSettings: { soundEnabled: true, soundVolume: 100, soundMode: 'rotate', soundTrack: 2, soundTrackCount: 0, language: 'en', languageStored: false, displayEnabled: true, displayRotation180: false, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 65, displayDimAfterMinutes: 10, displayDimBrightness: 5 },"
if js.count(old_state) != 1:
    raise SystemExit("projectSettings state marker mismatch")
js = js.replace(old_state, new_state, 1)

# A manual language selection is still cached in the browser, but now it also
# becomes the device/OLED language through the same one-field preference API.
old_language_store = "    PreferenceStore.set('language', value);\n    document.documentElement.lang = value;"
new_language_store = "    PreferenceStore.set('language', value);\n    Transport.setProjectPreference('language', value);\n    document.documentElement.lang = value;"
if js.count(old_language_store) != 1:
    raise SystemExit("setLanguage marker mismatch")
js = js.replace(old_language_store, new_language_store, 1)

# If a fresh browser has no own choice, prefer an already stored device/OLED
# language. If the device has none yet, synchronize the browser-detected choice.
bootstrap_marker = "        patchState({ project: data.project || {}, firmware: data.firmware || {}, projectSettings: data.projectSettings || state.projectSettings, timeManagement: data.timeManagement || state.timeManagement, status: { ...(data.status || {}), api: 'ok' } });\n        Bindings.notify('projectSettings');"
bootstrap_new = bootstrap_marker + r'''
        const deviceLanguage = data.projectSettings?.language || '';
        const deviceLanguageStored = data.projectSettings?.languageStored === true;
        if (!state.preferences.languageStored && deviceLanguageStored && state.preferences.availableLanguages.includes(deviceLanguage)) {
          state.preferences.language = deviceLanguage;
          state.preferences.languageMode = 'manual';
          state.preferences.languageStored = true;
          PreferenceStore.set('language', deviceLanguage);
          document.documentElement.lang = deviceLanguage;
        } else if (state.preferences.availableLanguages.includes(state.preferences.language) &&
                   (!deviceLanguageStored || deviceLanguage !== state.preferences.language)) {
          this.setProjectPreference('language', state.preferences.language);
        }'''
if js.count(bootstrap_marker) != 1:
    raise SystemExit("bootstrap sync marker mismatch")
js = js.replace(bootstrap_marker, bootstrap_new, 1)

# Locale-aware calendar labels for the two additional standard languages too.
js = js.replace(
    "  function localeForLabels() { return state.preferences.language === 'en' ? 'en-GB' : 'de-DE'; }",
    "  function localeForLabels() { const lang = state.preferences.language; return lang === 'en' ? 'en-GB' : lang === 'fr' ? 'fr-FR' : lang === 'it' ? 'it-IT' : 'de-DE'; }",
    1,
)

# Replace the mixed device settings grid with three clear hardware groups.
new_project_settings = r'''  function renderProjectSettings() {
    const root = el('div', 'project-settings');
    const controls = {};

    const addSection = (titleKey) => {
      const section = el('section', 'project-settings-section');
      const head = el('div', 'project-settings-section-head');
      head.textContent = t(titleKey);
      const grid = el('div', 'project-settings-grid');
      section.append(head, grid); root.append(section);
      return grid;
    };

    const addSwitch = (grid, field, labelKey) => {
      const row = el('label', 'project-setting-row project-setting-switch');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const control = el('span', 'project-sound-control');
      const input = el('input'); input.type = 'checkbox'; input.dataset.projectSetting = field;
      const visual = el('span', 'project-sound-visual');
      const stateText = el('strong', 'project-sound-state');
      control.append(input, visual, stateText); row.append(label, control); grid.append(row);
      controls[field] = { input, stateText, row };
    };

    const addSelect = (grid, field, labelKey, options) => {
      const row = el('label', 'project-setting-row');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const select = el('select', 'project-setting-input'); select.dataset.projectSetting = field;
      for (const [value, key] of options) { const option = el('option'); option.value = value; option.textContent = t(key); select.append(option); }
      row.append(label, select); grid.append(row); controls[field] = { input: select, row };
    };

    const addNumber = (grid, field, labelKey, min, max, suffixKey = '') => {
      const row = el('label', 'project-setting-row');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const wrap = el('span', 'project-number-control');
      const input = el('input', 'project-setting-input'); input.type = 'number'; input.min = String(min); input.max = String(max); input.step = '1'; input.dataset.projectSetting = field;
      wrap.append(input);
      if (suffixKey) { const suffix = el('span', 'project-setting-suffix'); suffix.textContent = t(suffixKey); wrap.append(suffix); }
      row.append(label, wrap); grid.append(row); controls[field] = { input, row };
    };

    const addRange = (grid, field, labelKey, min, max) => {
      const row = el('label', 'project-setting-row');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const wrap = el('span', 'project-range-control');
      const input = el('input', 'project-setting-range'); input.type = 'range'; input.min = String(min); input.max = String(max); input.step = '1'; input.dataset.projectSetting = field;
      const output = el('output', 'project-range-value'); output.textContent = '0 %';
      input.addEventListener('input', () => { output.textContent = `${input.value} %`; });
      wrap.append(input, output); row.append(label, wrap); grid.append(row); controls[field] = { input, output, row };
    };

    const addHardwareAction = (grid, moduleId, actionId, labelKey, iconName) => {
      const row = el('div', 'project-setting-row project-setting-action');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const button = el('button', 'button'); button.type = 'button'; button.dataset.hardwareAction = actionId; button.dataset.hardwareModule = moduleId;
      button.append(icon(iconName)); const text = el('span'); text.textContent = t(labelKey); button.append(text);
      row.append(label, button); grid.append(row);
    };

    const displayGrid = addSection('project.section.display');
    addSwitch(displayGrid, 'displayEnabled', 'project.displayEnabled');
    addSelect(displayGrid, 'displayMode', 'project.displayMode', [
      ['standard','project.displayMode.standard'], ['count','project.displayMode.count'], ['last','project.displayMode.last'],
      ['day-progress','project.displayMode.dayProgress'], ['focus','project.displayMode.focus']
    ]);
    addSwitch(displayGrid, 'displayRotation180', 'project.displayRotation180');
    addRange(displayGrid, 'displayBrightness', 'project.displayBrightness', 1, 100);
    addNumber(displayGrid, 'displayDimAfterMinutes', 'project.displayDimAfter', 0, 1440, 'project.minutes');
    const dimHint = el('div', 'form-note project-setting-note'); dimHint.textContent = t('project.dimDisabled'); displayGrid.append(dimHint);
    addRange(displayGrid, 'displayDimBrightness', 'project.displayDimBrightness', 0, 100);
    addHardwareAction(displayGrid, 'display', 'test', 'action.displayTest', 'display');

    const feedbackGrid = addSection('project.section.displayFeedback');
    addSwitch(feedbackGrid, 'displayFlashEnabled', 'project.displayFlash');

    const soundGrid = addSection('project.section.sound');
    addSwitch(soundGrid, 'soundEnabled', 'interruptions.sound');
    addRange(soundGrid, 'soundVolume', 'project.soundVolume', 0, 100);
    addSelect(soundGrid, 'soundMode', 'project.soundMode', [['fixed','project.soundMode.fixed'],['rotate','project.soundMode.rotate']]);
    addNumber(soundGrid, 'soundTrack', 'project.soundTrack', 2, 65535);
    const soundHint = el('div', 'form-note project-setting-note'); soundGrid.append(soundHint);
    addHardwareAction(soundGrid, 'audio', 'test', 'action.audioTest', 'audio');

    const message = el('div', 'project-setting-message'); message.setAttribute('aria-live', 'polite'); message.dataset.projectSettingMessage = '1';
    root.append(message);

    const update = () => {
      const ps = state.projectSettings || {};
      for (const [field, entry] of Object.entries(controls)) {
        const input = entry.input;
        if (document.activeElement !== input) {
          if (input.type === 'checkbox') input.checked = !!ps[field];
          else if (ps[field] != null) input.value = String(ps[field]);
        }
        if (entry.stateText) entry.stateText.textContent = t(input.checked ? 'common.on' : 'common.off');
        if (entry.output) entry.output.textContent = `${input.value} %`;
      }
      const count = Number(ps.soundTrackCount || 0);
      controls.soundTrack.input.max = count >= 2 ? String(count) : '65535';
      const fixedMode = (ps.soundMode || 'rotate') === 'fixed';
      controls.soundTrack.row.hidden = !fixedMode;
      soundHint.textContent = count >= 2
        ? t('project.soundTracksAvailable').replaceAll('{n}', String(count))
        : `${t('project.soundTrackHint')} ${t('project.soundTracksUnknown')}`;
    };
    Bindings.add('projectSettings', update);
    return root;
  }
'''
pattern = r"  function renderProjectSettings\(\) \{.*?\n  \}\n\n  function localeForLabels"
js, count = re.subn(pattern, new_project_settings + "\n  function localeForLabels", js, count=1, flags=re.S)
if count != 1:
    raise SystemExit("renderProjectSettings replacement failed")

write(APP, js)

# Group styling stays inside the existing embedded-first card system.
css = read(CSS)
css_marker = ".project-settings { display: grid; gap: var(--space-3); }\n"
css_add = css_marker + ".project-settings-section { display: grid; gap: var(--space-2); }\n.project-settings-section + .project-settings-section { padding-top: var(--space-3); border-top: 1px solid var(--border); }\n.project-settings-section-head { font-size: .86rem; font-weight: 800; letter-spacing: .04em; text-transform: uppercase; color: var(--text-muted); }\n.project-setting-action .button { justify-self: end; }\n"
if css.count(css_marker) != 1:
    raise SystemExit("project settings CSS marker mismatch")
css = css.replace(css_marker, css_add, 1)
write(CSS, css)

# ---------------------------------------------------------------------------
# Release checks
# ---------------------------------------------------------------------------
check_path = ROOT / "tools" / "release_check.py"
checks = read(check_path)
checks = checks.replace("Unterbrechungszaehler 3.1.0", "Unterbrechungszaehler 3.2.0", 1)
checks = checks.replace("'SOFTWARE_VERSION[] = \"3.1.0\"' in config, \"project version 3.1.0\"", "'SOFTWARE_VERSION[] = \"3.2.0\"' in config, \"project version 3.2.0\"", 1)
checks = checks.replace("DISPLAY_BOOT_SCREEN_MIN_MS = 2000", "DISPLAY_BOOT_SCREEN_MIN_MS = 4000", 1)
checks = checks.replace("two-second nonblocking boot screen minimum", "four-second nonblocking boot screen minimum", 1)
checks = checks.replace("delay(2000)", "delay(4000)")
checks = checks.replace("boot screen has no blocking two-second delay", "boot screen has no blocking four-second delay", 1)
insert_marker = '''    check("displayEnabled" in JS and "project.displayEnabled" in JS, "display master switch in UI")
'''
extra_checks = insert_marker + '''    check("DISPLAY_BRIGHTNESS_DEFAULT_PERCENT = 65" in project and "DISPLAY_DIM_BRIGHTNESS_DEFAULT_PERCENT = 5" in project, "display brightness defaults 65/5 percent")
    check("SOUND_VOLUME_DEFAULT_PERCENT = 100" in project, "sound volume default 100 percent")
    check("INTERRUPTION_SOUND_MODE_DEFAULT = ProjectPreferences::SoundMode::Rotate" in project, "rotating sound is the fresh default")
    check("DISPLAY_ROTATION_180_DEFAULT = false" in project and "displayRotation180" in JS, "persistent 180-degree display option")
    check("day-progress" in JS and "project.displayMode.focus" in JS, "five OLED display modes exposed")
    check("normalizeDisplayText" in (ROOT / "display_sh1106.cpp").read_text(encoding="utf-8") and 'append("AE")' in (ROOT / "display_sh1106.cpp").read_text(encoding="utf-8"), "OLED UTF-8 transliteration fallback")
    check("ProjectPreferences::language()" in (ROOT / "display_views.cpp").read_text(encoding="utf-8") and 'prefs.putString(key' in (ROOT / "project_preferences.cpp").read_text(encoding="utf-8"), "OLED language follows persistent UI language")
    check("soundVolume" in JS and "setVolumePercent" in (ROOT / "audio_dy_sv17f.cpp").read_text(encoding="utf-8"), "DY-SV17F volume control")
    check("ota.hint" not in JS and "Export Compiled Binary" not in JS, "obsolete Arduino sketch BIN hint removed")
'''
if checks.count(insert_marker) != 1:
    raise SystemExit("release check insertion marker mismatch")
checks = checks.replace(insert_marker, extra_checks, 1)
lang_marker = '''    check(
        "I18N.it = {" in JS
        and "I18N.fr = {" in JS
        and "I18N['swg-alb'] = {" in JS
        and "I18N['swg-ob'] = {" in JS,
        "additional bundled UI languages",
    )
'''
lang_extra = lang_marker + '''    for language in ("de", "en", "it", "fr", "swg", "swg-alb", "swg-ob"):
        token = f"Object.assign(I18N{'.' + language if '-' not in language else '[' + repr(language) + ']'}, {{"
        check(token in JS, f"3.2.0 UI additions present for {language}")
'''
if checks.count(lang_marker) != 1:
    raise SystemExit("additional language check marker mismatch")
checks = checks.replace(lang_marker, lang_extra, 1)
write(check_path, checks)

# ---------------------------------------------------------------------------
# Documentation and release notes
# ---------------------------------------------------------------------------
root_readme = REPO / "README.md"
text = read(root_readme)
text = text.replace('> **Aktueller Stand:** `3.1.0`', '> **Aktueller Stand:** `3.2.0`', 1)
text = text.replace(
    'Das Display kann persistent ein- und ausgeschaltet werden; beim echten Geräteboot bleibt das Startbild mindestens zwei Sekunden sichtbar.',
    'Das Display kann persistent ein- und ausgeschaltet sowie um 180° gedreht werden. Es folgt der gewählten UI-Sprache; nicht im kleinen OLED-Font enthaltene Umlaute/Akzente werden lesbar transliteriert. Beim echten Geräteboot bleibt das Startbild mindestens vier Sekunden sichtbar.',
    1,
)
sound_anchor = '> **Solange das DY-SV17F per Micro-USB mit dem Computer verbunden ist bzw. sein interner Speicher über USB verwendet wird, funktioniert die normale Soundausgabe nicht.** Nach dem Kopieren deshalb den Datenträger auswerfen, USB trennen und erst dann Soundtest, Boot-Ton oder Unterbrechungston prüfen.\n'
sound_add = sound_anchor + '\nAb 3.2.0 ist die Lautstärke in der Weboberfläche von **0–100 %** einstellbar; bei einer frischen Konfiguration sind **100 %** voreingestellt. Der Standardmodus für Unterbrechungstöne ist **Wechseln/Rotation** über die erkannten Tracks 2…N. Bereits gespeicherte Einstellungen älterer 3.x-Stände werden nicht überschrieben.\n'
if text.count(sound_anchor) != 1:
    raise SystemExit("root sound doc anchor mismatch")
text = text.replace(sound_anchor, sound_add, 1)
quick_anchor = '## Schnellstart\n'
display_doc = '''## Display in 3.2.0

Das SH1106 folgt der in der Weboberfläche gewählten Sprache. Die Sprache wird zusätzlich auf dem ESP32 gespeichert, damit das OLED sie auch direkt nach einem Neustart kennt. Der kompakte OLED-Font bleibt bewusst klein: Umlaute und Akzente werden bei Bedarf lesbar nach ASCII umgesetzt, zum Beispiel `ä → AE`, `ö → OE`, `ü → UE`, `ß → SS` und `é → E`.

Für eine frische Konfiguration gelten **65 % normale Helligkeit** und **5 % gedimmte Helligkeit**. Zusätzlich kann das komplette OLED persistent um **180°** gedreht werden. Der nicht blockierende Bootscreen bleibt mindestens **4 Sekunden** sichtbar.

Die fünf Displaymodi sind:

- Standard – Heute + letzte Unterbrechung
- Nur Anzahl
- Nur letzte Unterbrechung
- Tagesfortschritt – Anzahl heute + Ø abgeschlossener Abstand + Tageszeit-Fortschrittsbalken
- Fokus – groß die Zeit seit der letzten Unterbrechung + heutige Anzahl

'''
if text.count(quick_anchor) != 1:
    raise SystemExit("root quick anchor mismatch")
text = text.replace(quick_anchor, display_doc + quick_anchor, 1)
write(root_readme, text)

# Compact language documentation: keep the three maintained README variants in sync.
for rel, title, bullets in [
    ("docs/de/README.md", "# Unterbrechungszähler 3.2.0", "- OLED folgt der gewählten UI-Sprache, inklusive kompakter Umlaut-/Akzent-Transliteration\n- fünf OLED-Modi, 180°-Drehung und mindestens 4 Sekunden Bootscreen\n- neue Display-Defaults 65 % normal / 5 % gedimmt\n- DY-SV17F-Lautstärke 0–100 %, Standard 100 %, Tonmodus standardmäßig wechselnd"),
    ("docs/en/README.md", "# Interruption Counter 3.2.0", "- OLED follows the selected UI language with compact umlaut/accent transliteration\n- five OLED modes, 180° rotation and at least a four-second boot screen\n- new display defaults: 65% normal / 5% dimmed\n- DY-SV17F volume 0–100%, default 100%, rotating sounds by default"),
    ("docs/swg/README.md", "# Unterbrechungszähler 3.2.0 – Schwäbisch", "- S OLED folgt dr ausgwählte Sproch ond macht Umlaute/Akzente notfalls lesbar als ASCII\n- fünf OLED-Modi, 180° dreha ond mindestens vier Sekunda Bootbild\n- neue Display-Standards: 65 % normal / 5 % gedimmt\n- DY-SV17F-Lautstärk 0–100 %, Standard 100 %, Tön standardmäßig wechselnd"),
]:
    path = REPO / rel
    doc = read(path)
    doc = re.sub(r"^# .*?3\.0\.1.*$|^# .*?3\.1\.0.*$", title, doc, count=1, flags=re.M)
    doc = doc.replace('> **Current version:** `3.0.0`', '> **Current version:** `3.2.0`')
    doc = doc.replace('> **Aktueller Stand:** `3.0.0`', '> **Aktueller Stand:** `3.2.0`')
    marker = "## Funktionen\n" if rel.endswith("de/README.md") else "## What can the device do?\n" if rel.endswith("en/README.md") else "## Was kann s Gerät?\n"
    if marker in doc:
        doc = doc.replace(marker, marker + "\n**Neu in 3.2.0:**\n\n" + bullets + "\n\n", 1)
    else:
        doc += "\n\n## 3.2.0\n\n" + bullets + "\n"
    write(path, doc)

# Hardware docs mention the operational defaults/features without duplicating the full README.
for rel, paragraph in [
    ("docs/de/HARDWARE.md", "\n## OLED-Einstellungen ab 3.2.0\n\nDas SH1106 kann persistent um 180° gedreht werden. Standardwerte einer frischen Konfiguration sind 65 % Helligkeit und 5 % gedimmte Helligkeit. Der Bootscreen bleibt mindestens vier Sekunden sichtbar und blockiert die übrigen Gerätefunktionen nicht.\n"),
    ("docs/en/HARDWARE.md", "\n## OLED settings from 3.2.0\n\nThe SH1106 can be persistently rotated by 180°. Fresh defaults are 65% normal brightness and 5% dim brightness. The boot screen remains visible for at least four seconds without blocking the remaining device services.\n"),
]:
    path = REPO / rel
    doc = read(path)
    if "OLED-Einstellungen ab 3.2.0" not in doc and "OLED settings from 3.2.0" not in doc:
        doc += paragraph
    write(path, doc)

changelog = REPO / "CHANGELOG.md"
cl = read(changelog)
entry = '''## 3.2.0

- OLED-Inhalte folgen der persistent synchronisierten UI-Sprache; kompakte OLED-Transliteration für Umlaute und Akzente
- SH1106-Bootscreen auf mindestens 4 Sekunden verlängert, weiterhin nicht blockierend
- persistente 180°-Displaydrehung ergänzt
- frische Helligkeitsdefaults auf 65 % normal und 5 % gedimmt gesetzt
- Geräteeinstellungen nach Display, Display-Feedback und DY-SV17F-Sound gruppiert
- DY-SV17F-Lautstärke von 0–100 %, Standard 100 %, zentral auf den Modulbereich 0…30 abgebildet
- Wechsel-/Rotationsmodus ist bei einer frischen Konfiguration der neue Tonstandard; Track 1 bleibt Boot vorbehalten
- OLED-Modi **Tagesfortschritt** und **Fokus** ergänzt
- Arduino-IDE-Hinweis zum Erzeugen einer Sketch-BIN vollständig aus dem OTA-UI entfernt; Releases liefern die fertige OTA-BIN
- Webbundle, Sprachchecks und Releasechecks auf 3.2.0 erweitert

'''
if "## 3.2.0" not in cl:
    cl = cl.replace("# Changelog\n\n", "# Changelog\n\n" + entry, 1)
write(changelog, cl)

release_notes = ROOT / "RELEASE_NOTES.md"
rn = read(release_notes)
if "# Unterbrechungszähler 3.2.0" not in rn:
    rn = '''# Unterbrechungszähler 3.2.0

- OLED folgt der gewählten UI-Sprache mit ressourcenschonender Transliteration für Sonderzeichen
- Bootscreen mindestens 4 Sekunden, weiterhin nicht blockierend
- persistente 180°-Drehung und fünf Displaymodi inklusive Tagesfortschritt/Fokus
- frische Displaydefaults 65 % / 5 %
- DY-SV17F-Lautstärke 0–100 %, Standard 100 %; Wechselmodus ist neuer Standard
- Einstellungen klar nach Display, Display-Feedback und Sound gegliedert
- veralteter Arduino-Sketch-BIN-Hinweis im OTA-UI entfernt

''' + rn
write(release_notes, rn)

# Sketch documentation and test report version/current features.
for rel in ["README.md", "PROJECT_ARCHITECTURE.md", "TEST_REPORT.md"]:
    path = ROOT / rel
    doc = read(path)
    doc = doc.replace("3.1.0", "3.2.0")
    doc = doc.replace("3.0.0", "3.2.0") if rel == "TEST_REPORT.md" else doc
    if rel == "TEST_REPORT.md" and "Display 180" not in doc:
        doc += "\n## 3.2.0 zusätzliche Hardwaretests\n\n- Displayinhalte in allen sieben UI-Sprachvarianten und OLED-Transliteration prüfen\n- 4-Sekunden-Bootscreen messen und Taster/Sound währenddessen testen\n- 180°-Drehung inklusive Boot, Test, Flash und allen fünf Ansichten prüfen\n- Helligkeit 65 % / gedimmt 5 % auf frischer NVS-Konfiguration prüfen\n- DY-SV17F-Lautstärke 0/50/100 % und Wechselmodus auf realem Modul prüfen\n"
    write(path, doc)

print("3.2.0 UI/docs patch applied")
