#pragma once

#include <Arduino.h>

// Klare Rueckmeldung fuer Display-Aktionen und sichtbare Simulation auch dann,
// wenn keine physische OLED-Hardware erkannt wurde.
static const char WEB_UI_DISPLAY_FEEDBACK[] PROGMEM = R"HTML(
<style>
/* Die Display-Karte selbst bleibt lesbar. Ohne Hardware/Simulation werden nur
   die tatsaechlich nicht nutzbaren Bereiche gedimmt. Der Simulations-Schalter
   bleibt bewusst voll sichtbar und bedienbar. */
#displayCard.moduleCard.unavailable{opacity:1!important;filter:none!important}
#displayCard.moduleCard.unavailable>h3{opacity:.58}
#displayCard.moduleCard.unavailable>#oledPreviewBlock,
#displayCard.moduleCard.unavailable>.settingsRow,
#displayCard.moduleCard.unavailable>.displayAdvanced,
#displayCard.moduleCard.unavailable>.actions,
#displayCard.moduleCard.unavailable>p.infoHelp{opacity:.38;filter:grayscale(1)}
#displayCard.moduleCard.unavailable>#displaySimulationRow{opacity:1!important;filter:none!important}
#displaySimulationRow{box-shadow:0 0 0 0 rgba(33,102,209,0);transition:box-shadow .18s,border-color .18s,background .18s}
#displayCard.moduleCard.unavailable>#displaySimulationRow{border-color:var(--accent);box-shadow:0 0 0 2px rgba(33,102,209,.10)}

/* Sofortige Rueckmeldung fuer Speichern und Testen. */
#displaySave,#displayTest{transition:transform .08s,box-shadow .15s,background .15s,border-color .15s,color .15s}
#displaySave:active,#displayTest:active{transform:translateY(1px) scale(.97)}
.displayActionRunning{background:var(--accent)!important;border-color:var(--accent)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(33,102,209,.18)!important}
.displayActionSuccess{background:var(--ok)!important;border-color:var(--ok)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(22,128,58,.17)!important}
.displayActionError{background:var(--danger)!important;border-color:var(--danger)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(180,35,24,.15)!important}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const text=(de,en)=>document.documentElement.lang==='en'?en:de;

function resetButton(button,label,delay){
  setTimeout(()=>{
    if(!button)return;
    button.classList.remove('displayActionRunning','displayActionSuccess','displayActionError');
    button.textContent=label;
    button.disabled=false;
  },delay||1100);
}

function state(button,kind,label){
  if(!button)return;
  button.classList.remove('displayActionRunning','displayActionSuccess','displayActionError');
  if(kind)button.classList.add(kind);
  button.textContent=label;
}

async function testDisplay(){
  const button=$('displayTest');
  if(!button)return;
  const normal=text('Display testen','Test display');
  button.disabled=true;
  state(button,'displayActionRunning',text('Test läuft …','Testing …'));
  try{
    const response=await fetch('/api/display-test',{method:'POST',cache:'no-store'});
    if(!response.ok)throw new Error(await response.text());
    state(button,'displayActionSuccess',text('Test gestartet ✓','Test started ✓'));
    resetButton(button,normal,1200);
  }catch(e){
    state(button,'displayActionError',text('Test fehlgeschlagen','Test failed'));
    resetButton(button,normal,1600);
  }
}

function hookTestButton(){
  const old=$('displayTest');
  if(!old||old.dataset.feedbackHook)return;
  // Der klassische WebUi-Handler wird durch Klonen entfernt, damit nicht zwei
  // HTTP-Requests gleichzeitig gesendet werden.
  const button=old.cloneNode(true);
  button.dataset.feedbackHook='1';
  old.replaceWith(button);
  button.addEventListener('click',testDisplay);
}

function hookSaveFeedback(){
  const button=$('displaySave');
  if(!button||button.dataset.feedbackVisual)return;
  button.dataset.feedbackVisual='1';

  // WebUiDisplay besitzt bereits den eigentlichen Speicher-Handler. Wir zeigen
  // schon beim pointerdown eine unmittelbare Reaktion; der vorhandene Status-
  // Text entscheidet anschliessend ueber Erfolg oder Fehler.
  button.addEventListener('pointerdown',()=>{
    if(button.disabled)return;
    state(button,'displayActionRunning',text('Speichert …','Saving …'));
  });

  const status=$('displayState');
  if(status){
    const observer=new MutationObserver(()=>{
      const value=(status.textContent||'').trim();
      if(!value)return;
      const success=status.classList.contains('statusOk');
      const error=status.classList.contains('statusBad');
      if(success){
        state(button,'displayActionSuccess',text('Gespeichert ✓','Saved ✓'));
        resetButton(button,text('Display speichern','Save display'),1200);
      }else if(error){
        state(button,'displayActionError',text('Speichern fehlgeschlagen','Save failed'));
        resetButton(button,text('Display speichern','Save display'),1600);
      }
    });
    observer.observe(status,{childList:true,characterData:true,subtree:true,attributes:true,attributeFilter:['class']});
  }
}

function install(){hookTestButton();hookSaveFeedback()}
install();
setTimeout(install,100);
const settingsTab=document.querySelector('.tab[data-view="settings"]');
if(settingsTab)settingsTab.addEventListener('click',()=>setTimeout(install,30));
const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(install,40));
})();
</script>
)HTML";
