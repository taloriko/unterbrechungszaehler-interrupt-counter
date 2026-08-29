#pragma once

#include <Arduino.h>

// Ruhigere Display-Darstellung im Browser: keine Sekunden in der Kopf-Uhr,
// kein laufender Frame-Zaehler und eindeutige Benennung der Standardansicht.
static const char WEB_UI_DISPLAY_CALM[] PROGMEM = R"HTML(
<style>
/* Der technische Frame-Zaehler war in der Vorschau optisch unruhig und hat
   keinen Nutzen fuer den normalen Betrieb. */
#oledPreviewMeta{display:none!important}

/* Auch ohne erkannte Hardware bleibt die Simulation bewusst als moegliche
   Aktion voll sichtbar. Nur die nicht nutzbaren Display-Bereiche werden matt. */
#displayCard.moduleCard.unavailable{opacity:1!important;filter:none!important}
#displayCard.moduleCard.unavailable>h3{opacity:.58}
#displayCard.moduleCard.unavailable>#oledPreviewBlock,
#displayCard.moduleCard.unavailable>.settingsRow,
#displayCard.moduleCard.unavailable>.displayAdvanced,
#displayCard.moduleCard.unavailable>.actions,
#displayCard.moduleCard.unavailable>p.infoHelp{opacity:.38;filter:grayscale(1)}
#displayCard.moduleCard.unavailable>#displaySimulationRow{opacity:1!important;filter:none!important;border-color:var(--accent)}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const text=(de,en)=>document.documentElement.lang==='en'?en:de;
let clockGuard=false;

function calmClock(){
  const clock=$('deviceClock');
  if(!clock||clockGuard)return;
  const value=(clock.textContent||'').trim();
  if(/^\d{2}:\d{2}:\d{2}$/.test(value)){
    clockGuard=true;
    clock.textContent=value.substring(0,5);
    clockGuard=false;
  }
}

function renameOverview(){
  const select=$('displayLayout');
  if(!select)return;
  const option=select.querySelector('option[value="0"]');
  if(option)option.textContent=text('Tagesübersicht','Daily overview');
}

function install(){
  calmClock();
  renameOverview();

  const clock=$('deviceClock');
  if(clock&&!clock.dataset.calmClock){
    clock.dataset.calmClock='1';
    new MutationObserver(calmClock).observe(clock,{childList:true,characterData:true,subtree:true});
  }

  const settings=$('settings');
  if(settings&&!settings.dataset.calmDisplay){
    settings.dataset.calmDisplay='1';
    new MutationObserver(renameOverview).observe(settings,{childList:true,subtree:true});
  }
}

install();
setTimeout(install,100);
const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(renameOverview,30));
})();
</script>
)HTML";
