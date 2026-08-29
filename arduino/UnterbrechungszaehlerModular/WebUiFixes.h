#pragma once

#include <Arduino.h>

// Kleine visuelle Korrekturen, die auf der bestehenden klassischen UI aufsetzen.
// Die eigentlichen Werte werden weiterhin von WebUi.h gepflegt; hier wird nur
// verhindert, dass rohe und lokalisierte Zahlen abwechselnd sichtbar werden.
static const char WEB_UI_FIXES[] PROGMEM = R"HTML(
<style>
/* Speicherwerte: Die Basis-UI darf den Rohwert weiter aktualisieren, sichtbar
   bleibt aber ausschliesslich der lokalisierte Wert aus data-formatted. */
.stableStorageNumber{position:relative;color:transparent!important;min-width:130px;white-space:nowrap}
.stableStorageNumber::after{content:attr(data-formatted);position:absolute;right:0;top:0;color:var(--text);font-weight:600;white-space:nowrap}

/* Ruhige, einheitliche SVG-Icons statt plattformabhaengiger Emoji-Symbole. */
.countAction svg{width:25px;height:25px;display:block;fill:none;stroke:currentColor;stroke-width:1.9;stroke-linecap:round;stroke-linejoin:round;pointer-events:none}
.countAction.add svg{width:27px;height:27px}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const locale=()=>document.documentElement.lang==='en'?'en-US':'de-DE';
const formatNumber=n=>new Intl.NumberFormat(locale()).format(Number(n||0));

function makeStorageNumberStable(id){
  const el=$(id);
  if(!el)return;
  el.classList.add('stableStorageNumber');
  if(!el.dataset.formatted) el.dataset.formatted=el.textContent||'-';
}

function updateStableStorage(d){
  if(!d)return;
  const values=[
    ['devRecent',d.eventCount,d.ringCapacity],
    ['devArchive',d.archiveCount,d.archiveCapacity],
    ['devAutark',d.autarkCount,d.autarkCapacity]
  ];
  values.forEach(v=>{
    const el=$(v[0]);
    if(!el)return;
    makeStorageNumberStable(v[0]);
    el.dataset.formatted=formatNumber(v[1])+' / '+formatNumber(v[2]);
  });
}

function installIcons(){
  const add=$('addBtn');
  const del=$('undoBtn');
  if(add){
    add.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 11V6.5a1.5 1.5 0 0 1 3 0V10"/><path d="M12 10V5.5a1.5 1.5 0 0 1 3 0V10"/><path d="M15 10V7a1.5 1.5 0 0 1 3 0v6.5"/><path d="M9 11 7.8 9.8a1.55 1.55 0 0 0-2.2 2.2l3.7 5.2A5 5 0 0 0 13.4 19H15a5 5 0 0 0 5-5v-2"/></svg>';
    add.title=document.documentElement.lang==='en'?'Add interruption':'Unterbrechung hinzufügen';
    add.setAttribute('aria-label',add.title);
  }
  if(del){
    del.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18"/><path d="M8 6V4h8v2"/><path d="M19 6l-1 14H6L5 6"/><path d="M10 10v6M14 10v6"/></svg>';
    del.title=document.documentElement.lang==='en'?'Delete last interruption':'Letzte Unterbrechung löschen';
    del.setAttribute('aria-label',del.title);
  }
}

function hookStatus(){
  const original=window.fetch.bind(window);
  window.fetch=async function(){
    const response=await original.apply(null,arguments);
    try{
      const url=String(arguments[0]||'');
      if(url.indexOf('/api/status')>=0&&response.ok){
        response.clone().json().then(updateStableStorage).catch(()=>{});
      }
    }catch(e){}
    return response;
  };
}

['devRecent','devArchive','devAutark'].forEach(makeStorageNumberStable);
installIcons();
hookStatus();

// Sprachwechsel aendert nur die Darstellung der Tausendertrennung und Tooltips.
const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(installIcons,0));
})();
</script>
)HTML";
