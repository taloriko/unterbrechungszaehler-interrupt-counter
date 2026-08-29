#pragma once

#include <Arduino.h>

// Header-, WLAN-, Mobile- und Heatmap-Feinschliff fuer die klassische Oberflaeche.
static const char WEB_UI_NETWORK[] PROGMEM = R"HTML(
<style>
/* Uhrzeit und KW in einer Zeile, Datum sauber darunter. Der Trenner bekommt
   links und rechts exakt denselben Abstand. */
.headTime{display:grid!important;grid-template-columns:max-content max-content;column-gap:0;row-gap:2px;align-items:baseline;text-align:left!important;line-height:1.2}
.headTime #deviceClock{font-size:.86rem;font-weight:600;color:var(--text)}
.headTime .headWeek{display:inline!important;margin:0!important;font-size:.8rem;color:var(--muted);white-space:nowrap}
.headTime .headWeek::before{content:'|';display:inline-block;margin:0 7px;color:var(--muted)}
.headTime .headDate{display:block!important;grid-column:1/-1;margin:0!important;font-size:.74rem;color:var(--muted)}

/* Der Kopfbereich nutzt dieselbe Karten-Sprache wie der restliche Inhalt. */
header{min-height:54px!important;padding:9px 13px;background:var(--card);border:1px solid var(--line);border-radius:12px;box-shadow:var(--shadow)}
.tabs{margin-top:10px!important}
.headTitle h1{font-size:1.18rem!important;font-weight:650!important}
.titleIcon{width:28px;height:28px;display:inline-flex;align-items:center;justify-content:center;border:1px solid var(--line);border-radius:8px;background:var(--bg);font-size:1rem!important}
header .hwIcon{background:var(--bg)}

/* Zusatzmodule und Verbindungsstatus gemeinsam oben rechts. Alle Status-Icons
   haben exakt denselben Abstand. Nur der Text bekommt etwas Luft zum Icon. */
.headWifi{display:flex!important;align-items:center;justify-content:flex-end;gap:4px;white-space:nowrap}
.headWifi .headModules{display:inline-flex;gap:4px;margin:0}
.headWifi .dot{display:none!important}
.wifiStateIcon{color:var(--muted)}
.wifiStateIcon.available{color:var(--ok);border-color:var(--ok)}
.wifiStateIcon.unavailable{color:var(--danger)}
.wifiStateIcon svg{width:18px;height:18px;display:block;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round}
.headerWifiText{font-size:.8rem;color:var(--muted);margin-left:4px;font-weight:500}

/* WLAN-Signalstaerke direkt im Reiter Geraet. */
.wifiSignalWrap{margin-top:12px}
.wifiSignalBar{height:12px;background:var(--line);border-radius:7px;overflow:hidden;position:relative}
.wifiSignalBar>span{display:block;height:100%;width:0;transition:width .25s ease,background .25s ease}
.wifiSignalBar>span.good{background:var(--ok)}
.wifiSignalBar>span.warn{background:#d49a00}
.wifiSignalBar>span.bad{background:var(--danger)}
.wifiSignalBar>span.none{background:var(--line)}
/* Vier feine Teilstriche bei 20/40/60/80 %. */
.wifiSignalBar::after{content:'';position:absolute;inset:0;pointer-events:none;background:linear-gradient(to right,transparent 19.6%,var(--card) 19.6% 20%,transparent 20% 39.6%,var(--card) 39.6% 40%,transparent 40% 59.6%,var(--card) 59.6% 60%,transparent 60% 79.6%,var(--card) 79.6% 80%,transparent 80%)}

/* Aktuelle Positionen in den Heatmaps. */
.heat .heatCurrentLabel{color:var(--accent)!important;font-weight:700!important;box-shadow:inset 0 0 0 2px var(--accent);border-radius:6px}
.heat td.heatCurrentCell{border-color:var(--accent)!important;box-shadow:inset 0 0 0 1px var(--accent)}
.heat tr.heatTodayRow td{border-top-color:var(--accent)!important;border-bottom-color:var(--accent)!important}
.heat tr.heatTodayRow th:first-child{color:var(--accent)!important;font-weight:700;box-shadow:inset 0 0 0 2px var(--accent);border-radius:6px}
.heat .heatCurrentAxis{color:var(--accent)!important;font-weight:700!important}

/* Smartphone: Titel als eigene Zeile, darunter Zeit links und Statusmodule rechts. */
@media(max-width:720px){
  header{
    display:grid!important;
    grid-template-columns:minmax(0,1fr) auto!important;
    grid-template-areas:"title title" "time wifi";
    gap:8px 12px!important;
    padding:10px 12px!important;
    min-height:0!important
  }
  .headTitle{
    grid-area:title;
    width:100%;
    justify-content:center!important;
    padding-bottom:8px;
    border-bottom:1px solid var(--line)
  }
  .headTitle h1{font-size:1.08rem!important}
  .titleIcon{display:inline-flex!important;width:26px;height:26px}
  .headTime{grid-area:time;justify-self:start;align-self:center;min-width:0}
  .headWifi{grid-area:wifi;justify-self:end;align-self:center;gap:4px!important}
  .headWifi .headModules{gap:4px!important}
  .headerWifiText{display:none!important}

  /* Footer nicht mehr mitten in Git-Link oder Versionsnummer umbrechen. */
  .footerWrap{grid-template-columns:1fr!important;gap:8px!important;text-align:center!important;margin-top:18px!important}
  .footerWrap>div:first-child,.footerRight{text-align:center!important;white-space:nowrap}
  .footerCenter{font-size:0;text-align:center!important}
  .footerCenter>span{display:block;font-size:.82rem;line-height:1.35}
  .footerCenter>#footerVersion{display:block;font-size:.82rem;line-height:1.35;margin-top:3px;white-space:nowrap;word-break:keep-all;overflow-wrap:normal}
  .footerCenter>#footerVersion::before{content:'Version ';font-weight:400;color:var(--muted)}
}

@media(max-width:390px){
  header{gap:7px 8px!important;padding:9px 10px!important}
  .headTitle h1{font-size:1rem!important}
  .headTime #deviceClock{font-size:.82rem}
  .headTime .headWeek{font-size:.76rem}
  .headTime .headWeek::before{margin:0 5px}
  .headWifi .hwIcon{width:25px;height:25px}
}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const text=(de,en)=>document.documentElement.lang==='en'?en:de;

function isoWeekFromDate(){
  const date=$('deviceDateHead');
  if(!date)return null;
  const m=(date.textContent||'').match(/(\d{2})\.(\d{2})\.(\d{4})/);
  if(!m)return null;
  const d=new Date(Date.UTC(+m[3],+m[2]-1,+m[1]));
  const day=d.getUTCDay()||7;
  d.setUTCDate(d.getUTCDate()+4-day);
  const first=new Date(Date.UTC(d.getUTCFullYear(),0,1));
  return Math.ceil((((d-first)/86400000)+1)/7);
}

function setupHeaderTime(){
  const clock=$('deviceClock'),date=$('deviceDateHead');
  if(!clock||!date)return;
  let week=$('headWeek');
  if(!week){
    week=document.createElement('span');
    week.id='headWeek';
    week.className='headWeek';
  }
  clock.insertAdjacentElement('afterend',week);
  const number=isoWeekFromDate();
  week.textContent=number===null?'':text('KW ','CW ')+number;
}

function wifiSvg(){
  return '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4.5 9.5a11.5 11.5 0 0 1 15 0"/><path d="M7.8 12.8a6.7 6.7 0 0 1 8.4 0"/><path d="M10.7 16a2.3 2.3 0 0 1 2.6 0"/><path d="M12 19h.01"/></svg>';
}

function setupHeaderStatus(){
  const right=document.querySelector('.headWifi');
  const modules=document.querySelector('.headModules');
  const conn=$('conn');
  if(!right||!conn)return;
  if(modules&&modules.parentElement!==right) right.insertBefore(modules,right.firstChild);
  let icon=$('headWifiIcon');
  if(!icon){
    icon=document.createElement('span');
    icon.id='headWifiIcon';
    icon.className='hwIcon wifiStateIcon unavailable';
    icon.innerHTML=wifiSvg();
    right.insertBefore(icon,conn);
  }
  conn.classList.add('headerWifiText');
}

function ensureDeviceWifi(){
  const state=$('devWifi');
  if(!state)return;
  const box=state.closest('.infoBox');
  const kv=state.closest('.kv');
  if(!box||!kv)return;
  if(!$('deviceWifiRssi')){
    kv.insertAdjacentHTML('beforeend','<span id="deviceWifiSignalLabel"></span><span id="deviceWifiRssi">-</span><span id="deviceWifiRatingLabel"></span><span id="deviceWifiQuality">-</span>');
  }
  if(!$('deviceWifiBar')){
    const wrap=document.createElement('div');
    wrap.className='wifiSignalWrap';
    wrap.innerHTML='<div class="wifiSignalBar"><span id="deviceWifiBar" class="none"></span></div>';
    box.appendChild(wrap);
  }
  const signalLabel=$('deviceWifiSignalLabel');
  const ratingLabel=$('deviceWifiRatingLabel');
  if(signalLabel)signalLabel.textContent=text('Signal','Signal');
  if(ratingLabel)ratingLabel.textContent=text('Bewertung','Rating');
}

function qualityPercent(rssi){
  if(rssi<=-100)return 0;
  if(rssi>=-50)return 100;
  return Math.max(0,Math.min(100,2*(rssi+100)));
}

function signalLabel(rssi){
  if(rssi>=-60)return text('sehr gut','very good');
  if(rssi>=-67)return text('gut','good');
  if(rssi>=-75)return text('ausreichend','fair');
  return text('schwach','weak');
}

function updateWifi(d){
  if(!d)return;
  setupHeaderTime();
  setupHeaderStatus();
  ensureDeviceWifi();
  const local=!d.wifi&&d.ip&&d.ip!=='-';
  const connected=!!d.wifi;
  const icon=$('headWifiIcon');
  if(icon){
    icon.className='hwIcon wifiStateIcon '+((connected||local)?'available':'unavailable');
    icon.title=connected?(text('WLAN verbunden','Wi-Fi connected')+(d.rssi?' · '+d.rssi+' dBm':'')):(local?text('Lokaler Zugangspunkt aktiv','Local access point active'):text('Offline','Offline'));
  }
  const rssi=$('deviceWifiRssi'),quality=$('deviceWifiQuality'),bar=$('deviceWifiBar');
  const dbm=Number(d.rssi||0);
  if(!connected||!dbm){
    if(rssi)rssi.textContent='-';
    if(quality)quality.textContent='-';
    if(bar){bar.style.width='0%';bar.className='none'}
    return;
  }
  if(rssi)rssi.textContent=dbm+' dBm';
  if(quality)quality.textContent=signalLabel(dbm);
  if(bar){bar.style.width=qualityPercent(dbm)+'%';bar.className=dbm>=-60?'good':(dbm>=-75?'warn':'bad')}
}

function hookStatus(){
  const original=window.fetch.bind(window);
  window.fetch=async function(){
    const response=await original.apply(null,arguments);
    try{
      const url=String(arguments[0]||'');
      if(url.indexOf('/api/status')>=0&&response.ok)response.clone().json().then(updateWifi).catch(()=>{});
    }catch(e){}
    return response;
  };
}

function isoParts(date){
  const d=new Date(Date.UTC(date.getFullYear(),date.getMonth(),date.getDate()));
  const day=d.getUTCDay()||7;
  d.setUTCDate(d.getUTCDate()+4-day);
  const year=d.getUTCFullYear();
  const start=new Date(Date.UTC(year,0,1));
  return {year:year,week:Math.ceil((((d-start)/86400000)+1)/7)};
}

function centerHorizontal(wrap,target,key){
  if(!wrap||!target||wrap.scrollWidth<=wrap.clientWidth)return;
  if(wrap.dataset.focusX===key)return;
  wrap.dataset.focusX=key;
  wrap.scrollLeft=Math.max(0,target.offsetLeft-(wrap.clientWidth-target.offsetWidth)/2);
}

function centerBoth(wrap,target,key){
  if(!wrap||!target||wrap.dataset.focusBoth===key)return;
  wrap.dataset.focusBoth=key;
  if(wrap.scrollWidth>wrap.clientWidth)wrap.scrollLeft=Math.max(0,target.offsetLeft-(wrap.clientWidth-target.offsetWidth)/2);
  if(wrap.scrollHeight>wrap.clientHeight)wrap.scrollTop=Math.max(0,target.offsetTop-(wrap.clientHeight-target.offsetHeight)/2);
}

function clearMarks(table){
  if(!table)return;
  table.querySelectorAll('.heatCurrentLabel,.heatCurrentCell,.heatCurrentAxis,.heatTodayRow').forEach(el=>el.classList.remove('heatCurrentLabel','heatCurrentCell','heatCurrentAxis','heatTodayRow'));
}

function decorateWeek(now,iso){
  const wrap=$('weekHeat'),table=wrap&&wrap.querySelector('table.heat');
  if(!table)return;
  clearMarks(table);
  const selectedYear=Number($('weekYear')&&$('weekYear').value),selectedWeek=Number($('weekNumber')&&$('weekNumber').value);
  if(selectedYear!==iso.year||selectedWeek!==iso.week)return;
  const rows=table.rows,day=now.getDay(),rowIndex=day===0?7:day,row=rows[rowIndex];
  if(!row)return;
  row.classList.add('heatTodayRow');
  if(row.cells[0])row.cells[0].classList.add('heatCurrentLabel');
  const start=Math.max(0,Math.min(23,Number($('heatStartHour')&&$('heatStartHour').value)||5));
  const end=Math.max(start,Math.min(23,Number($('heatEndHour')&&$('heatEndHour').value)||18));
  const hour=now.getHours();
  if(hour>=start&&hour<=end){
    const col=hour-start+1;
    if(rows[0]&&rows[0].cells[col])rows[0].cells[col].classList.add('heatCurrentAxis');
    if(row.cells[col]){row.cells[col].classList.add('heatCurrentCell');centerHorizontal(wrap,row.cells[col],selectedYear+'-'+selectedWeek+'-'+hour)}
  }else if(row.cells[0])centerHorizontal(wrap,row.cells[0],selectedYear+'-'+selectedWeek+'-day');
}

function decorateMonthWeek(now,iso){
  const wrap=$('monthWeekHeat'),table=wrap&&wrap.querySelector('table.heat');
  if(!table)return;
  clearMarks(table);
  const selectedYear=Number($('monthYear')&&$('monthYear').value);
  if(selectedYear!==now.getFullYear())return;
  const col=iso.week,rowIndex=now.getMonth()+1,row=table.rows[rowIndex];
  if(table.rows[0]&&table.rows[0].cells[col])table.rows[0].cells[col].classList.add('heatCurrentLabel');
  if(row){
    if(row.cells[0])row.cells[0].classList.add('heatCurrentAxis');
    if(row.cells[col]){row.cells[col].classList.add('heatCurrentCell');centerHorizontal(wrap,row.cells[col],selectedYear+'-'+iso.week)}
  }
}

function decorateYearMonth(now){
  const wrap=$('yearMonthHeat'),table=wrap&&wrap.querySelector('table.heat');
  if(!table)return;
  clearMarks(table);
  const year=String(now.getFullYear());
  let row=null;
  for(let i=1;i<table.rows.length;i++){if(table.rows[i].cells[0]&&table.rows[i].cells[0].textContent.trim()===year){row=table.rows[i];break}}
  if(!row)return;
  const col=now.getMonth()+1;
  if(row.cells[0])row.cells[0].classList.add('heatCurrentLabel');
  if(table.rows[0]&&table.rows[0].cells[col])table.rows[0].cells[col].classList.add('heatCurrentAxis');
  if(row.cells[col]){row.cells[col].classList.add('heatCurrentCell');centerBoth(wrap,row.cells[col],year+'-'+col)}
}

function decorateHeatmaps(){const now=new Date(),iso=isoParts(now);decorateWeek(now,iso);decorateMonthWeek(now,iso);decorateYearMonth(now)}
function watch(id){const el=$(id);if(!el)return;new MutationObserver(()=>setTimeout(decorateHeatmaps,0)).observe(el,{childList:true})}

setupHeaderTime();
setupHeaderStatus();
ensureDeviceWifi();
hookStatus();
watch('weekHeat');watch('monthWeekHeat');watch('yearMonthHeat');
document.querySelectorAll('.tab[data-view="heatmap"]').forEach(tab=>tab.addEventListener('click',()=>setTimeout(decorateHeatmaps,30)));
['weekYear','weekNumber','monthYear','heatStartHour','heatEndHour'].forEach(id=>{const el=$(id);if(el)el.addEventListener('change',()=>setTimeout(decorateHeatmaps,30))});
setTimeout(decorateHeatmaps,50);

const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(()=>{ensureDeviceWifi();setupHeaderTime();decorateHeatmaps()},0));
})();
</script>
)HTML";
