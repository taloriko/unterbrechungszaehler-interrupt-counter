#pragma once

#include <Arduino.h>

// Display-Konfiguration und echte 128x64-Pixelvorschau.
// Das Canvas rendert den Framebuffer aus DisplayService. Die Simulation kann
// denselben Renderpfad auch ohne physisch angeschlossenes OLED verwenden.
static const char WEB_UI_DISPLAY[] PROGMEM = R"HTML(
<style>
.oledPreviewBlock{margin:2px 0 16px;padding:13px;border:1px solid var(--line);border-radius:10px;background:var(--bg)}
.oledPreviewHead{display:flex;justify-content:space-between;align-items:center;gap:10px;margin-bottom:9px}
.oledPreviewHead b{font-size:.88rem}.oledPreviewBadges{display:flex;gap:5px;align-items:center;flex-wrap:wrap;justify-content:flex-end}
.oledBadge{font-size:.68rem;border:1px solid var(--line);border-radius:999px;padding:3px 7px;color:var(--muted);background:var(--card)}
.oledBadge.on{color:var(--ok);border-color:var(--ok)}.oledBadge.dim{color:#9a7200;border-color:#c79500}.oledBadge.off{color:var(--danger);border-color:var(--danger)}.oledBadge.sim{color:var(--accent);border-color:var(--accent)}
.oledShell{position:relative;max-width:540px;margin:auto;padding:15px 18px;border-radius:15px;background:#111;border:1px solid #3a3a3a;box-shadow:inset 0 0 0 3px #202020,0 3px 10px rgba(0,0,0,.14)}
.oledScreen{display:block;width:100%;aspect-ratio:2/1;background:#020609;border-radius:3px;image-rendering:pixelated;image-rendering:crisp-edges}
.oledOffOverlay{display:none;position:absolute;inset:15px 18px;align-items:center;justify-content:center;background:rgba(0,0,0,.62);color:#adb5bd;font-size:.8rem;letter-spacing:.08em;border-radius:3px}.oledOffOverlay.show{display:flex}
.oledPreviewMeta{text-align:center;margin-top:8px;font-size:.72rem;color:var(--muted)}
.displayAdvanced{border-top:1px solid var(--line);padding-top:12px;margin-top:12px}.displayChecks{display:flex;gap:14px;flex-wrap:wrap;margin:10px 0}.displayCheck{display:inline-flex;align-items:center;gap:6px;font-size:.86rem}.displayCheck input{width:17px;height:17px}
.displayHint{font-size:.76rem;color:var(--muted);margin-top:4px}
.displaySimulationRow{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:2px 0 13px;padding:10px 12px;border:1px solid var(--line);border-radius:9px;background:var(--bg)}
.displaySimulationText{min-width:0}.displaySimulationText b{display:block;font-size:.88rem}.displaySimulationText span{font-size:.76rem;color:var(--muted)}
.simToggle{display:inline-flex;align-items:center;gap:8px;cursor:pointer;white-space:nowrap}.simToggle input{position:absolute;opacity:0;pointer-events:none}.simTrack{width:42px;height:24px;border-radius:999px;background:var(--line);position:relative;transition:.2s}.simTrack:after{content:'';position:absolute;width:18px;height:18px;left:3px;top:3px;border-radius:50%;background:var(--card);box-shadow:0 1px 3px rgba(0,0,0,.25);transition:.2s}.simToggle input:checked+.simTrack{background:var(--accent)}.simToggle input:checked+.simTrack:after{transform:translateX(18px)}.simToggle input:focus-visible+.simTrack{outline:2px solid var(--accent);outline-offset:2px}
.hwIcon.simulated{opacity:1!important;filter:none!important;color:var(--accent)!important;border-color:var(--accent)!important}
@media(max-width:520px){.oledPreviewBlock{padding:9px}.oledShell{padding:10px 12px}.oledOffOverlay{inset:10px 12px}.oledPreviewHead{align-items:flex-start;flex-direction:column}.displaySimulationRow{align-items:flex-start}}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const text=(de,en)=>document.documentElement.lang==='en'?en:de;
let displayDirty=false;
let displayAvailable=false;
let displayHardwarePresent=false;
let displaySimulation=false;
let lastPreviewRevision=-1;

function addRow(card,id,label,control){
  if($(id))return;
  const row=document.createElement('div');
  row.className='settingsRow displayExtraRow';
  row.id=id;
  row.innerHTML='<label>'+label+'</label>'+control;
  const actions=card.querySelector('.actions');
  card.insertBefore(row,actions);
}

function ensureDisplayUi(){
  const card=$('displayCard');
  if(!card)return;
  const title=card.querySelector('h3');

  if(!$('displaySimulationRow')){
    const simulation=document.createElement('div');
    simulation.id='displaySimulationRow';
    simulation.className='displaySimulationRow';
    simulation.innerHTML='<div class="displaySimulationText"><b>'+text('Display-Simulation','Display simulation')+'</b><span id="displaySimulationState">'+text('Simulation aus','Simulation off')+'</span></div>'+ 
      '<label class="simToggle"><input id="displaySimulation" type="checkbox"><span class="simTrack" aria-hidden="true"></span><span>'+text('Simulieren','Simulate')+'</span></label>';
    title.insertAdjacentElement('afterend',simulation);
  }

  if(!$('oledPreviewBlock')){
    const preview=document.createElement('div');
    preview.id='oledPreviewBlock';
    preview.className='oledPreviewBlock';
    preview.innerHTML='<div class="oledPreviewHead"><b>'+text('OLED Live-Vorschau','OLED live preview')+'</b><div class="oledPreviewBadges"><span id="oledModeBadge" class="oledBadge">-</span><span id="oledStateBadge" class="oledBadge">-</span><span id="oledBrightnessBadge" class="oledBadge">-</span></div></div>'+ 
      '<div class="oledShell"><canvas id="oledPreviewCanvas" class="oledScreen" width="128" height="64"></canvas><div id="oledOffOverlay" class="oledOffOverlay">DISPLAY AUS</div></div>'+ 
      '<div id="oledPreviewMeta" class="oledPreviewMeta">128 × 64 · SH1106 · -</div>';
    $('displaySimulationRow').insertAdjacentElement('afterend',preview);
  }

  if(!$('displayDimBrightnessRow')){
    const brightRow=$('displayBrightness')&&$('displayBrightness').closest('.settingsRow');
    if(brightRow){
      const row=document.createElement('div');
      row.className='settingsRow displayExtraRow';
      row.id='displayDimBrightnessRow';
      row.innerHTML='<label for="displayDimBrightness">'+text('Helligkeit gedimmt','Dimmed brightness')+'</label><div class="rangeRow"><input id="displayDimBrightness" type="range" min="1" max="255" step="1"><input id="displayDimBrightnessNumber" type="number" min="1" max="255"></div>';
      brightRow.insertAdjacentElement('afterend',row);
    }
  }

  addRow(card,'displayOffAfterRow',text('Ganz ausschalten nach','Switch off after'),'<div><input id="displayOffAfter" type="number" min="0" max="86400" step="5" style="width:100%"><div class="displayHint">'+text('0 = im Normalbetrieb nie ganz ausschalten; Autark bleibt fest bei 15 s.','0 = never switch off in normal mode; standalone remains fixed at 15 s.')+'</div></div>');
  addRow(card,'displayLayoutRow',text('Layout','Layout'),'<select id="displayLayout"><option value="0">'+text('Standard','Standard')+'</option><option value="1">'+text('Kompakt','Compact')+'</option><option value="2">'+text('Große Uhr','Large clock')+'</option></select>');

  if(!card.querySelector('.displayAdvanced')){
    const advanced=document.createElement('div');
    advanced.className='displayAdvanced';
    advanced.innerHTML='<div class="displayChecks">'+
      '<label class="displayCheck"><input id="displayWakeOnEvent" type="checkbox">'+text('Bei Ereignis aufwecken','Wake on event')+'</label>'+ 
      '<label class="displayCheck"><input id="displayInvert" type="checkbox">'+text('Anzeige invertieren','Invert display')+'</label>'+ 
      '<label class="displayCheck"><input id="displayRotation" type="checkbox">'+text('180° drehen','Rotate 180°')+'</label>'+ 
      '</div><p class="infoHelp">'+text('Die Simulation verwendet exakt denselben 128×64-Framebuffer wie das echte OLED. Ist keine Hardware angeschlossen, können damit Layout, Helligkeit, Dimmen, Ausschalten und Ereignisreaktionen vollständig in der Web-Vorschau getestet werden.','Simulation uses exactly the same 128×64 framebuffer as the real OLED. Without connected hardware, layout, brightness, dimming, switch-off and event reactions can all be tested in the web preview.')+'</p>';
    const actions=card.querySelector('.actions');
    card.insertBefore(advanced,actions);
  }

  hookDisplayControls();
}

function syncPair(a,b){
  const x=$(a),y=$(b);if(!x||!y||x.dataset.paired)return;
  x.dataset.paired='1';y.dataset.paired='1';
  x.addEventListener('input',()=>{y.value=x.value;displayDirty=true});
  y.addEventListener('input',()=>{x.value=y.value;displayDirty=true});
}

function markDirty(){displayDirty=true}
function hookDisplayControls(){
  syncPair('displayDimBrightness','displayDimBrightnessNumber');
  ['displayBrightness','displayBrightnessNumber','displayDimAfter','displayOffAfter','displayLayout','displayWakeOnEvent','displayInvert','displayRotation'].forEach(id=>{const el=$(id);if(el&&!el.dataset.displayHook){el.dataset.displayHook='1';el.addEventListener('input',markDirty)}});

  const simulation=$('displaySimulation');
  if(simulation&&!simulation.dataset.displayHook){
    simulation.dataset.displayHook='1';
    simulation.addEventListener('change',()=>setDisplaySimulation(simulation.checked));
  }

  // Der alte Button besitzt bereits einen Listener fuer die fruehere Zweifeld-
  // API. Klonen entfernt diesen Listener, danach gilt nur noch die neue API.
  const old=$('displaySave');
  if(old&&!old.dataset.extended){
    const button=old.cloneNode(true);
    button.dataset.extended='1';
    old.replaceWith(button);
    button.addEventListener('click',saveDisplaySettings);
  }
}

function currentValue(id,fallback){const el=$(id);return el?el.value:fallback}
function currentChecked(id,fallback){const el=$(id);return el?el.checked:fallback}

async function setDisplaySimulation(enabled){
  const toggle=$('displaySimulation');
  const state=$('displaySimulationState');
  if(toggle)toggle.disabled=true;
  if(state)state.textContent=text('Schalte Simulation …','Switching simulation …');
  try{
    const body='enabled='+(enabled?'1':'0');
    const r=await fetch('/api/display-simulation',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body,cache:'no-store'});
    if(!r.ok)throw new Error(await r.text());
    displaySimulation=enabled;
    await refreshDisplayStatus();
    await loadPreview(true);
  }catch(e){
    if(toggle)toggle.checked=!enabled;
    if(state)state.textContent=text('Simulation konnte nicht umgeschaltet werden.','Simulation could not be changed.');
  }finally{
    if(toggle)toggle.disabled=false;
  }
}

async function refreshDisplayStatus(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    if(r.ok){const d=await r.json();applySettings(d,true)}
  }catch(e){}
}

async function saveDisplaySettings(){
  const state=$('displayState');
  const params=new URLSearchParams();
  params.set('brightness',currentValue('displayBrightness',127));
  params.set('dimBrightness',currentValue('displayDimBrightness',32));
  params.set('dimAfter',currentValue('displayDimAfter',60));
  params.set('offAfter',currentValue('displayOffAfter',0));
  params.set('layout',currentValue('displayLayout',0));
  params.set('wakeOnEvent',currentChecked('displayWakeOnEvent',true)?'1':'0');
  params.set('inverted',currentChecked('displayInvert',false)?'1':'0');
  params.set('rotation180',currentChecked('displayRotation',false)?'1':'0');
  try{
    const r=await fetch('/api/display-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:params.toString(),cache:'no-store'});
    if(!r.ok)throw new Error(await r.text());
    displayDirty=false;
    if(state){state.textContent=text('Display-Einstellungen gespeichert.','Display settings saved.');state.className='small statusOk'}
    await loadPreview(true);
  }catch(e){if(state){state.textContent=text('Display-Einstellungen konnten nicht gespeichert werden.','Display settings could not be saved.');state.className='small statusBad'}}
}

function updateHeaderState(d){
  const icon=$('headDisplay');if(!icon)return;
  icon.classList.remove('available','unavailable','simulated');
  if(d.displayPresent){icon.classList.add('available');icon.title='OLED SH1106'}
  else if(d.displaySimulation){icon.classList.add('simulated');icon.title=text('OLED Simulation aktiv','OLED simulation active')}
  else{icon.classList.add('unavailable');icon.title=text('OLED nicht vorhanden','OLED not present')}
}

function applySettings(d,force){
  if(!d)return;
  displayHardwarePresent=!!d.displayPresent;
  displaySimulation=!!d.displaySimulation;
  displayAvailable=!!d.displayAvailable||displayHardwarePresent||displaySimulation;

  const card=$('displayCard');
  if(card){
    card.classList.toggle('unavailable',!displayAvailable);
    card.querySelectorAll('input,select,button').forEach(el=>{el.disabled=!displayAvailable&&el.id!=='displaySimulation'});
  }

  const sim=$('displaySimulation');
  if(sim&&!sim.disabled)sim.checked=displaySimulation;
  const simState=$('displaySimulationState');
  if(simState){
    simState.textContent=displayHardwarePresent
      ? (displaySimulation?text('Hardware erkannt · Simulation zusätzlich aktiv','Hardware detected · simulation also active'):text('Echte Hardware erkannt','Real hardware detected'))
      : (displaySimulation?text('Simulation aktiv · keine Hardware erkannt','Simulation active · no hardware detected'):text('Keine Hardware erkannt · Simulation aus','No hardware detected · simulation off'));
  }
  setTimeout(()=>updateHeaderState(d),30);

  if(displayDirty&&!force)return;
  const set=(id,v)=>{const el=$(id);if(el&&document.activeElement!==el)el.value=v};
  const check=(id,v)=>{const el=$(id);if(el)el.checked=!!v};
  set('displayDimBrightness',d.displayDimBrightness||32);
  set('displayDimBrightnessNumber',d.displayDimBrightness||32);
  set('displayOffAfter',d.displayOffAfter||0);
  set('displayLayout',d.displayLayout||0);
  check('displayWakeOnEvent',d.displayWakeOnEvent!==false);
  check('displayInvert',d.displayInverted);
  check('displayRotation',d.displayRotation180);
}

function drawPreview(data){
  const canvas=$('oledPreviewCanvas');if(!canvas||!data||!data.frame)return;
  const ctx=canvas.getContext('2d',{alpha:false});
  const image=ctx.createImageData(128,64);
  const hex=data.frame;
  const bytes=new Uint8Array(1024);
  for(let i=0;i<1024;i++)bytes[i]=parseInt(hex.substr(i*2,2),16)||0;

  const effective=Math.max(1,Number(data.effectiveBrightness||1));
  const intensity=Math.round(95+(effective/255)*160);
  for(let y=0;y<64;y++){
    const page=y>>3,bit=1<<(y&7);
    for(let x=0;x<128;x++){
      let on=(bytes[page*128+x]&bit)!==0;
      if(data.inverted)on=!on;
      const p=(y*128+x)*4;
      const value=on?intensity:2;
      image.data[p]=on?Math.min(220,value):2;
      image.data[p+1]=on?Math.min(245,value+18):6;
      image.data[p+2]=on?255:9;
      image.data[p+3]=255;
    }
  }
  ctx.putImageData(image,0,0);

  const mode=$('oledModeBadge'),badge=$('oledStateBadge'),brightness=$('oledBrightnessBadge'),overlay=$('oledOffOverlay'),meta=$('oledPreviewMeta');
  if(mode){mode.className='oledBadge '+(data.hardwarePresent?'on':'sim');mode.textContent=data.hardwarePresent?text('HARDWARE','HARDWARE'):text('SIMULATION','SIMULATION')}
  if(badge){badge.className='oledBadge '+(!data.active?'off':(data.dimmed?'dim':'on'));badge.textContent=!data.active?text('AUS','OFF'):(data.dimmed?text('GEDIMMT','DIMMED'):text('AN','ON'))}
  if(brightness)brightness.textContent=String(data.effectiveBrightness)+'/255';
  if(overlay){overlay.classList.toggle('show',!data.active);overlay.textContent=text('DISPLAY AUS','DISPLAY OFF')}
  if(meta)meta.textContent='128 × 64 · Frame '+data.revision+' · '+(data.rotation180?'180°':'0°');
  lastPreviewRevision=Number(data.revision||0);
}

async function loadPreview(force){
  if(!displayAvailable)return;
  const settings=$('settings');
  if(!force&&(!settings||!settings.classList.contains('active')))return;
  try{const r=await fetch('/api/display-preview',{cache:'no-store'});if(r.ok)drawPreview(await r.json())}catch(e){}
}

function hookStatus(){
  const original=window.fetch.bind(window);
  window.fetch=async function(){
    const args=arguments,u=String(args[0]||''),r=await original.apply(null,args);
    try{
      if(u.indexOf('/api/status')>=0&&r.ok){
        const d=await r.clone().json();
        ensureDisplayUi();
        applySettings(d,false);
        if(displayAvailable&&Number(d.displayFrameRevision||0)!==lastPreviewRevision)loadPreview(false);

        // Die klassische Basis-UI kennt nur displayPresent. Im Simulationsmodus
        // bekommt nur der Browser eine logisch verfuegbare Anzeige, waehrend
        // displayHardwarePresent weiterhin die echte Diagnose enthaelt.
        let changedForBase=displaySimulation&&!displayHardwarePresent;
        const view=Object.assign({},d,{
          displayHardwarePresent:displayHardwarePresent,
          displayPresent:displayAvailable
        });

        // Waehrend der Benutzer editiert, bekommt auch die Basis-UI die noch
        // nicht gespeicherten Werte zurueck. Dadurch springen Slider nicht
        // durch den 1-s-Statuspoll an die alten Werte zurueck.
        if(displayDirty){
          changedForBase=true;
          Object.assign(view,{
            displayBrightness:Number(currentValue('displayBrightness',d.displayBrightness)),
            displayDimBrightness:Number(currentValue('displayDimBrightness',d.displayDimBrightness)),
            displayDimAfter:Number(currentValue('displayDimAfter',d.displayDimAfter)),
            displayOffAfter:Number(currentValue('displayOffAfter',d.displayOffAfter)),
            displayLayout:Number(currentValue('displayLayout',d.displayLayout)),
            displayWakeOnEvent:currentChecked('displayWakeOnEvent',d.displayWakeOnEvent),
            displayInverted:currentChecked('displayInvert',d.displayInverted),
            displayRotation180:currentChecked('displayRotation',d.displayRotation180)
          });
        }
        if(changedForBase)return new Response(JSON.stringify(view),{status:r.status,statusText:r.statusText,headers:r.headers});
      }
    }catch(e){}
    return r;
  };
}

ensureDisplayUi();
hookStatus();
setInterval(()=>loadPreview(false),1000);
const settingsTab=document.querySelector('.tab[data-view="settings"]');
if(settingsTab)settingsTab.addEventListener('click',()=>setTimeout(()=>loadPreview(true),80));
const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(()=>{
  const block=$('oledPreviewBlock');if(block)block.remove();
  const sim=$('displaySimulationRow');if(sim)sim.remove();
  document.querySelectorAll('.displayExtraRow,.displayAdvanced').forEach(el=>el.remove());
  ensureDisplayUi();
  refreshDisplayStatus();
  loadPreview(true);
},20));
})();
</script>
)HTML";
