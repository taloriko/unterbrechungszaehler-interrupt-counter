#pragma once

#include <Arduino.h>

// Einheitliche Bedienrueckmeldung und Tastaturfokus der Weboberflaeche.
static const char WEB_UI_INTERACTION[] PROGMEM = R"HTML(
<style>
.stableStorageNumber{position:relative;color:transparent!important;min-width:130px;white-space:nowrap}
.stableStorageNumber::after{content:attr(data-formatted);position:absolute;right:0;top:0;color:var(--text);font-weight:600;white-space:nowrap}
.countAction svg{width:25px;height:25px;display:block;fill:none;stroke:currentColor;stroke-width:1.9;stroke-linecap:round;stroke-linejoin:round;pointer-events:none}.countAction.add svg{width:27px;height:27px}

button,.btn{transition:transform .08s ease,background .15s ease,border-color .15s ease,color .15s ease,box-shadow .15s ease}
button:active,.btn:active{transform:translateY(1px) scale(.98)}
#export .btn,#displaySave,#displayTest,#ntpSaveBtn,#rtcSync{display:inline-flex;align-items:center;justify-content:center;gap:7px;background:var(--accent)!important;border-color:var(--accent)!important;color:#fff!important}
#export .btn:hover,#displaySave:hover,#displayTest:hover,#ntpSaveBtn:hover,#rtcSync:hover{filter:brightness(1.05)}

.actionRunning,.displayActionRunning{background:var(--accent)!important;border-color:var(--accent)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(33,102,209,.16)!important}
.actionSuccess,.displayActionSuccess{background:var(--ok)!important;border-color:var(--ok)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(22,128,58,.15)!important}
.actionError,.displayActionError{background:var(--danger)!important;border-color:var(--danger)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(180,35,24,.14)!important}
.actionRunning::after,.displayActionRunning::after{content:'…';font-weight:800}.actionSuccess::after,.displayActionSuccess::after{content:'✓';font-weight:800}.actionError::after,.displayActionError::after{content:'!';font-weight:900}

.actionUnit{display:inline-flex;flex-direction:column;align-items:flex-start;gap:5px;max-width:100%}.actionUnit.center{align-items:center}.actionMessage{display:none;max-width:360px;font-size:.78rem;line-height:1.25;color:var(--muted)}.actionMessage.show{display:block}.actionMessage.ok{color:var(--ok)}.actionMessage.bad{color:var(--danger)}
.actions{align-items:flex-start!important}
#displayState,#rtcSyncState{display:none!important}

#hourBars .barrow.zeroHour .bar{width:0!important}

/* Aktuelle und fokussierte Heatmap-Felder werden gruen markiert. */
.heat .heatCurrentLabel{color:var(--ok)!important;box-shadow:inset 0 0 0 2px var(--ok)!important}
.heat td.heatCurrentCell{border-color:var(--ok)!important;box-shadow:inset 0 0 0 1px var(--ok)!important}
.heat tr.heatTodayRow td{border-top-color:var(--ok)!important;border-bottom-color:var(--ok)!important}
.heat tr.heatTodayRow th:first-child{color:var(--ok)!important;box-shadow:inset 0 0 0 2px var(--ok)!important}
.heat .heatCurrentAxis{color:var(--ok)!important}
.heat td.heatFocus,.heat td:focus-visible{outline:2px solid var(--ok)!important;outline-offset:1px;border-color:var(--ok)!important;box-shadow:inset 0 0 0 1px var(--ok)!important}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const ACTIONS={
  addBtn:{running:'action.add.running',success:'action.add.success',error:'action.add.error'},
  undoBtn:{running:'action.delete.running',success:'action.delete.success',error:'action.delete.error'},
  ntpSaveBtn:{running:'action.ntp.running',success:'action.ntp.success',error:'action.ntp.error'},
  rtcSync:{running:'action.rtc.running',success:'action.rtc.success',error:'action.rtc.error'},
  displaySave:{running:'action.display_save.running',success:'action.display_save.success',error:'action.display_save.error'},
  displayTest:{running:'action.display_test.running',success:'action.display_test.success',error:'action.display_test.error'}
};
const ENDPOINTS=[
  ['/api/delete-last','undoBtn'],['/api/display-settings','displaySave'],['/api/display-test','displayTest'],['/api/rtc-sync','rtcSync'],['/api/ntp','ntpSaveBtn'],['/api/add','addBtn']
];
const LABELS={
  displaySave:{de:'Display speichern',en:'Save display',swg:'Display speichera'},
  displayTest:{de:'Display testen',en:'Test display',swg:'Display ausprobiera'}
};
function lang(){return localStorage.getItem('uic-lang')||'de'}
function txt(values){return values[lang()]||values.de||''}
function tr(key){return window.UicI18n?window.UicI18n.t(key):key}
function fmt(n){const locale=window.UicI18n?window.UicI18n.locale():(lang()==='en'?'en-US':'de-DE');return new Intl.NumberFormat(locale).format(Number(n||0))}

function makeStorageNumberStable(id){const el=$(id);if(!el)return;el.classList.add('stableStorageNumber');if(!el.dataset.formatted)el.dataset.formatted=el.textContent||'-'}
function updateStableStorage(d){if(!d)return;[['devRecent',d.eventCount,d.ringCapacity],['devArchive',d.archiveCount,d.archiveCapacity],['devAutark',d.autarkCount,d.autarkCapacity]].forEach(v=>{const el=$(v[0]);if(!el)return;makeStorageNumberStable(v[0]);el.dataset.formatted=fmt(v[1])+' / '+fmt(v[2])})}

function installCounterIcons(){const add=$('addBtn'),del=$('undoBtn');if(add&&!add.querySelector('svg'))add.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 11V6.5a1.5 1.5 0 0 1 3 0V10"/><path d="M12 10V5.5a1.5 1.5 0 0 1 3 0V10"/><path d="M15 10V7a1.5 1.5 0 0 1 3 0v6.5"/><path d="M9 11 7.8 9.8a1.55 1.55 0 0 0-2.2 2.2l3.7 5.2A5 5 0 0 0 13.4 19H15a5 5 0 0 0 5-5v-2"/></svg>';if(del&&!del.querySelector('svg'))del.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18"/><path d="M8 6V4h8v2"/><path d="M19 6l-1 14H6L5 6"/><path d="M10 10v6M14 10v6"/></svg>'}

function actionKey(el){if(el.id)return el.id;if(el.matches('#export a[href="/export.csv"]'))return'export-normal';if(el.matches('#export a[href="/archive.csv"]'))return'export-archive';if(el.matches('#export a[href="/autark.csv"]'))return'export-autark';if(el.matches('[data-theme-choice]'))return'theme-'+el.dataset.themeChoice;return''}
function ensureActionUnit(el){if(!el||el.classList.contains('tab'))return null;let unit=el.parentElement&&el.parentElement.classList.contains('actionUnit')?el.parentElement:null;if(unit)return unit;const key=actionKey(el);if(!key)return null;unit=document.createElement('span');unit.className='actionUnit'+(el.classList.contains('countAction')?' center':'');el.parentNode.insertBefore(unit,el);unit.appendChild(el);const message=document.createElement('span');message.className='actionMessage';message.dataset.forAction=key;unit.appendChild(message);return unit}
function ensureActions(){document.querySelectorAll('#addBtn,#undoBtn,#ntpSaveBtn,#rtcSync,#displaySave,#displayTest,#export a.btn,[data-theme-choice]').forEach(ensureActionUnit)}
function messageEl(el){const unit=ensureActionUnit(el);return unit&&unit.querySelector('.actionMessage')}
function showMessage(el,text,kind){const m=messageEl(el);if(!m)return;m.textContent=text;m.className='actionMessage show '+(kind||'');clearTimeout(m._hideTimer);m._hideTimer=setTimeout(()=>{m.className='actionMessage';m.textContent=''},5000)}
function setState(el,state){if(!el)return;el.classList.remove('actionRunning','actionSuccess','actionError');if(state)el.classList.add(state);clearTimeout(el._stateTimer);if(state&&state!=='actionRunning')el._stateTimer=setTimeout(()=>el.classList.remove(state),1400)}
function begin(id){const el=$(id),def=ACTIONS[id];if(!el||!def)return;setState(el,'actionRunning');showMessage(el,tr(def.running),'')}
function finish(id,ok){const el=$(id),def=ACTIONS[id];if(!el||!def)return;setState(el,ok?'actionSuccess':'actionError');showMessage(el,tr(ok?def.success:def.error),ok?'ok':'bad')}
function endpointAction(url){for(const pair of ENDPOINTS)if(url.indexOf(pair[0])>=0)return pair[1];return''}

function normalizeDisplayLabels(){Object.entries(LABELS).forEach(([id,values])=>{const el=$(id);if(!el)return;const desired=txt(values);if(el.textContent!==desired)el.textContent=desired})}

function fillHourGaps(){const host=$('hourBars');if(!host)return;const rows=[...host.querySelectorAll('.barrow')];if(rows.length<2)return;const byHour=new Map();rows.forEach(r=>{const h=parseInt((r.children[0]&&r.children[0].textContent)||'',10);if(Number.isFinite(h))byHour.set(h,r)});const hours=[...byHour.keys()];if(hours.length<2)return;const first=Math.min(...hours),last=Math.max(...hours);let missing=false;for(let h=first;h<=last;h++)if(!byHour.has(h)){missing=true;break}if(!missing)return;const frag=document.createDocumentFragment();for(let h=first;h<=last;h++){if(byHour.has(h)){frag.appendChild(byHour.get(h));continue}const row=document.createElement('div');row.className='barrow zeroHour';row.innerHTML='<span>'+String(h).padStart(2,'0')+'-'+String((h+1)%24).padStart(2,'0')+'</span><div class="barbg"><div class="bar" style="width:0%"></div></div><b>0</b>';frag.appendChild(row)}host.replaceChildren(frag)}

function prepareHeatmapCells(){document.querySelectorAll('.heat td').forEach(cell=>{if(!cell.hasAttribute('tabindex'))cell.tabIndex=0})}
function focusHeatCell(cell){if(!cell||!cell.matches('.heat td'))return;document.querySelectorAll('.heat td.heatFocus').forEach(c=>c.classList.remove('heatFocus'));cell.classList.add('heatFocus')}

document.addEventListener('focusin',e=>focusHeatCell(e.target),true);
document.addEventListener('click',e=>{const cell=e.target.closest('.heat td');if(cell){focusHeatCell(cell);cell.focus({preventScroll:true});return}const el=e.target.closest('button,.btn');if(!el||el.classList.contains('tab'))return;ensureActionUnit(el);if(el.matches('#export a.btn')){setState(el,'actionSuccess');showMessage(el,tr('action.download.started'),'ok');return}if(el.matches('[data-theme-choice]')){setState(el,'actionSuccess');showMessage(el,tr('action.theme.changed'),'ok');return}if(ACTIONS[el.id]){if(el.id==='ntpSaveBtn'){const input=$('ntpServer');if(input&&!input.value.trim()){setState(el,'actionError');showMessage(el,lang()==='en'?'Please enter an NTP server.':(lang()==='swg'?'Trag erscht en NTP-Server ei.':'Bitte NTP-Server eintragen.'),'bad');return}}begin(el.id)}},true);

const originalFetch=window.fetch.bind(window);window.fetch=async function(){const args=arguments,url=String(args[0]||''),id=endpointAction(url);let response;try{response=await originalFetch.apply(null,args)}catch(error){if(id)finish(id,false);throw error}try{if(url.indexOf('/api/status')>=0&&response.ok)response.clone().json().then(updateStableStorage).catch(()=>{});if(id)finish(id,response.ok)}catch(e){}return response};

let scheduled=false;function refreshEnhancements(){if(scheduled)return;scheduled=true;setTimeout(()=>{scheduled=false;ensureActions();installCounterIcons();fillHourGaps();prepareHeatmapCells();normalizeDisplayLabels()},0)}
['devRecent','devArchive','devAutark'].forEach(makeStorageNumberStable);refreshEnhancements();
new MutationObserver(refreshEnhancements).observe(document.body,{childList:true,subtree:true,characterData:true});
const language=$('languageSelect');if(language)language.addEventListener('change',()=>setTimeout(()=>{refreshEnhancements();if(window.UicI18n)window.UicI18n.apply()},0));
})();
</script>
)HTML";
