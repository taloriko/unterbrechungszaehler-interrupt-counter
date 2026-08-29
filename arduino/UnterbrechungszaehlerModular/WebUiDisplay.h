#pragma once

#include <Arduino.h>

// Display-Konfiguration und echte 128x64-Pixelvorschau.
// Das Canvas rendert ausschliesslich den Framebuffer aus DisplayService.
static const char WEB_UI_DISPLAY[] PROGMEM = R"HTML(
<style>
.oledPreviewBlock{margin:2px 0 16px;padding:13px;border:1px solid var(--line);border-radius:10px;background:var(--bg)}
.oledPreviewHead{display:flex;justify-content:space-between;align-items:center;gap:10px;margin-bottom:9px}
.oledPreviewHead b{font-size:.88rem}.oledPreviewBadges{display:flex;gap:5px;align-items:center;flex-wrap:wrap;justify-content:flex-end}
.oledBadge{font-size:.68rem;border:1px solid var(--line);border-radius:999px;padding:3px 7px;color:var(--muted);background:var(--card)}
.oledBadge.on{color:var(--ok);border-color:var(--ok)}.oledBadge.dim{color:#9a7200;border-color:#c79500}.oledBadge.off{color:var(--danger);border-color:var(--danger)}
.oledShell{position:relative;max-width:540px;margin:auto;padding:15px 18px;border-radius:15px;background:#111;border:1px solid #3a3a3a;box-shadow:inset 0 0 0 3px #202020,0 3px 10px rgba(0,0,0,.14)}
.oledScreen{display:block;width:100%;aspect-ratio:2/1;background:#020609;border-radius:3px;image-rendering:pixelated;image-rendering:crisp-edges}
.oledOffOverlay{display:none;position:absolute;inset:15px 18px;align-items:center;justify-content:center;background:rgba(0,0,0,.62);color:#adb5bd;font-size:.8rem;letter-spacing:.08em;border-radius:3px}.oledOffOverlay.show{display:flex}
.oledPreviewMeta{text-align:center;margin-top:8px;font-size:.72rem;color:var(--muted)}
.displayAdvanced{border-top:1px solid var(--line);padding-top:12px;margin-top:12px}.displayChecks{display:flex;gap:14px;flex-wrap:wrap;margin:10px 0}.displayCheck{display:inline-flex;align-items:center;gap:6px;font-size:.86rem}.displayCheck input{width:17px;height:17px}
.displayHint{font-size:.76rem;color:var(--muted);margin-top:4px}
@media(max-width:520px){.oledPreviewBlock{padding:9px}.oledShell{padding:10px 12px}.oledOffOverlay{inset:10px 12px}.oledPreviewHead{align-items:flex-start;flex-direction:column}}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const text=(de,en)=>document.documentElement.lang==='en'?en:de;
let displayDirty=false;
let displayPresent=false;
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
  if(!card||$('oledPreviewBlock'))return;
  const title=card.querySelector('h3');

  const preview=document.createElement('div');
  preview.id='oledPreviewBlock';
  preview.className='oledPreviewBlock';
  preview.innerHTML='<div class="oledPreviewHead"><b>'+text('OLED Live-Vorschau','OLED live preview')+'</b><div class="oledPreviewBadges"><span id="oledStateBadge" class="oledBadge">-</span><span id="oledBrightnessBadge" class="oledBadge">-</span></div></div>'+ 
    '<div class="oledShell"><canvas id="oledPreviewCanvas" class="oledScreen" width="128" height="64"></canvas><div id="oledOffOverlay" class="oledOffOverlay">DISPLAY AUS</div></div>'+ 
    '<div id="oledPreviewMeta" class="oledPreviewMeta">128 × 64 · SH1106 · -</div>';
  title.insertAdjacentElement('afterend',preview);

  const brightRow=$('displayBrightness')&&$('displayBrightness').closest('.settingsRow');
  if(brightRow){
    const row=document.createElement('div');
    row.className='settingsRow displayExtraRow';
    row.id='displayDimBrightnessRow';
    row.innerHTML='<label for="displayDimBrightness">'+text('Helligkeit gedimmt','Dimmed brightness')+'</label><div class="rangeRow"><input id="displayDimBrightness" type="range" min="1" max="255" step="1"><input id="displayDimBrightnessNumber" type="number" min="1" max="255"></div>';
    brightRow.insertAdjacentElement('afterend',row);
  }

  addRow(card,'displayOffAfterRow',text('Ganz ausschalten nach','Switch off after'),'<div><input id="displayOffAfter" type="number" min="0" max="86400" step="5" style="width:100%"><div class="displayHint">'+text('0 = im Normalbetrieb nie ganz ausschalten; Autark bleibt fest bei 15 s.','0 = never switch off in normal mode; standalone remains fixed at 15 s.')+'</div></div>');
  addRow(card,'displayLayoutRow',text('Layout','Layout'),'<select id="displayLayout"><option value="0">'+text('Standard','Standard')+'</option><option value="1">'+text('Kompakt','Compact')+'</option><option value="2">'+text('Große Uhr','Large clock')+'</option></select>');

  const advanced=document.createElement('div');
  advanced.className='displayAdvanced';
  advanced.innerHTML='<div class="displayChecks">'+
    '<label class="displayCheck"><input id="displayWakeOnEvent" type="checkbox">'+text('Bei Ereignis aufwecken','Wake on event')+'</label>'+ 
    '<label class="displayCheck"><input id="displayInvert" type="checkbox">'+text('Anzeige invertieren','Invert display')+'</label>'+ 
    '<label class="displayCheck"><input id="displayRotation" type="checkbox">'+text('180° drehen','Rotate 180°')+'</label>'+ 
    '</div><p class="infoHelp">'+text('Die Vorschau zeigt exakt den 128×64-Framebuffer, der an das OLED gesendet wird. Rotation wird am SH1106-Controller ausgeführt; der logische Bildinhalt bleibt deshalb in der Vorschau aufrecht.','The preview shows the exact 128×64 framebuffer sent to the OLED. Rotation is performed by the SH1106 controller, so the logical image remains upright in the preview.')+'</p>';
  const actions=card.querySelector('.actions');
  card.insertBefore(advanced,actions);

  hookDisplayControls();
}

function syncPair(a,b){
  const x=$(a),y=$(b);if(!x||!y)return;
  x.addEventListener('input',()=>{y.value=x.value;displayDirty=true});
  y.addEventListener('input',()=>{x.value=y.value;displayDirty=true});
}

function markDirty(){displayDirty=true}
function hookDisplayControls(){
  syncPair('displayDimBrightness','displayDimBrightnessNumber');
  ['displayBrightness','displayBrightnessNumber','displayDimAfter','displayOffAfter','displayLayout','displayWakeOnEvent','displayInvert','displayRotation'].forEach(id=>{const el=$(id);if(el)el.addEventListener('input',markDirty)});

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

function applySettings(d,force){
  if(!d)return;
  displayPresent=!!d.displayPresent;
  const card=$('displayCard');
  if(card)card.querySelectorAll('input,select,button').forEach(el=>el.disabled=!displayPresent);
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

  const badge=$('oledStateBadge'),brightness=$('oledBrightnessBadge'),overlay=$('oledOffOverlay'),meta=$('oledPreviewMeta');
  if(badge){badge.className='oledBadge '+(!data.active?'off':(data.dimmed?'dim':'on'));badge.textContent=!data.active?text('AUS','OFF'):(data.dimmed?text('GEDIMMT','DIMMED'):text('AN','ON'))}
  if(brightness)brightness.textContent=String(data.effectiveBrightness)+'/255';
  if(overlay){overlay.classList.toggle('show',!data.active);overlay.textContent=text('DISPLAY AUS','DISPLAY OFF')}
  if(meta)meta.textContent='128 × 64 · Frame '+data.revision+' · '+(data.rotation180?'180°':'0°');
  lastPreviewRevision=Number(data.revision||0);
}

async function loadPreview(force){
  if(!displayPresent)return;
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
        if(displayPresent&&Number(d.displayFrameRevision||0)!==lastPreviewRevision)loadPreview(false);

        // Waehrend der Benutzer editiert, bekommt auch die Basis-UI die noch
        // nicht gespeicherten Werte zurueck. Dadurch springen Slider nicht
        // durch den 1-s-Statuspoll an die alten Werte zurueck.
        if(displayDirty){
          const view=Object.assign({},d,{
            displayBrightness:Number(currentValue('displayBrightness',d.displayBrightness)),
            displayDimBrightness:Number(currentValue('displayDimBrightness',d.displayDimBrightness)),
            displayDimAfter:Number(currentValue('displayDimAfter',d.displayDimAfter)),
            displayOffAfter:Number(currentValue('displayOffAfter',d.displayOffAfter)),
            displayLayout:Number(currentValue('displayLayout',d.displayLayout)),
            displayWakeOnEvent:currentChecked('displayWakeOnEvent',d.displayWakeOnEvent),
            displayInverted:currentChecked('displayInvert',d.displayInverted),
            displayRotation180:currentChecked('displayRotation',d.displayRotation180)
          });
          return new Response(JSON.stringify(view),{status:r.status,statusText:r.statusText,headers:r.headers});
        }
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
  const block=$('oledPreviewBlock');
  if(block)block.remove();
  document.querySelectorAll('.displayExtraRow,.displayAdvanced').forEach(el=>el.remove());
  ensureDisplayUi();
  loadPreview(true);
},20));
})();
</script>
)HTML";
