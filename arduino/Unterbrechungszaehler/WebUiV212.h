#pragma once

#include <Arduino.h>

// Oberflaechenkorrekturen fuer 2.1.2: eindeutige Speicheranzeigen,
// Fadenkreuze in allen Heatmaps und getrennte Hardwarediagnose.
static const char WEB_UI_V212[] PROGMEM = R"HTML(
<style>
/* Auslastungsbalken verwenden dieselbe segmentierte Darstellung wie WLAN. */
.usageBar{height:12px!important;border-radius:7px!important;position:relative;background:var(--line)!important}
.usageBar>span{transition:width .25s ease,background .25s ease!important}
.usageBar>span.usageGood{background:var(--ok)!important}.usageBar>span.usageWarn{background:#d49a00!important}.usageBar>span.usageBad{background:var(--danger)!important}
.usageBar::after{content:'';position:absolute;inset:0;pointer-events:none;background:linear-gradient(to right,transparent 19.6%,var(--card) 19.6% 20%,transparent 20% 39.6%,var(--card) 39.6% 40%,transparent 40% 59.6%,var(--card) 59.6% 60%,transparent 60% 79.6%,var(--card) 79.6% 80%,transparent 80%)}
.v212StorageDuplicate{display:none!important}
/* Das aktuelle Zeit-/Datumsfeld wird als echtes Fadenkreuz ueber die komplette Heatmap gezogen. */
.heat td.v212CurrentRow,.heat td.v212CurrentCol{border-color:var(--ok)!important;box-shadow:inset 0 0 0 1px var(--ok)}
.heat th.v212CurrentAxis{color:var(--ok)!important;font-weight:700!important;box-shadow:inset 0 0 0 2px var(--ok);border-radius:6px}
.heat td.v212CurrentCross{box-shadow:inset 0 0 0 2px var(--ok),0 0 0 1px var(--ok)!important;font-weight:700}
.v2WdTable{min-width:850px!important}.v2WdTable td.v212Hardware{font-weight:600;color:var(--text)}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
let latestStatus=null;
function localText(de,en,swg){const texts={de:de,en:en,swg:swg};const code=(localStorage.getItem('uic-lang')||'de').toLowerCase();return texts[code]||de}
function pctClass(used,total){let p=Number(total)>0?100*Number(used||0)/Number(total):0;return p>=90?'usageBad':(p>=70?'usageWarn':'usageGood')}
function paintMeter(id,used,total){let bar=$(id+'Bar');if(!bar)return;bar.classList.remove('usageGood','usageWarn','usageBad');bar.classList.add(pctClass(used,total))}
function paintMeters(d){if(!d)return;paintMeter('meterRecent',d.eventCount,d.ringCapacity);paintMeter('meterArchive',d.archiveCount,d.archiveCapacity);paintMeter('meterAutark',d.autarkCount,d.autarkCapacity);paintMeter('meterFs',d.fsUsed,d.fsTotal);let ht=Number(d.heapTotal||0);paintMeter('meterRam',Math.max(0,ht-Number(d.heapFree||0)),ht);paintMeter('meterFlash',d.sketchUsed,Number(d.sketchUsed||0)+Number(d.sketchFree||0))}
function hideStorageDuplicates(){['devRecent','devArchive','devAutark'].forEach(id=>{let value=$(id);if(!value)return;value.classList.add('v212StorageDuplicate');let label=value.previousElementSibling;if(label)label.classList.add('v212StorageDuplicate')})}
function clearCross(table){if(!table)return;table.querySelectorAll('.v212CurrentRow,.v212CurrentCol,.v212CurrentCross,.v212CurrentAxis').forEach(x=>x.classList.remove('v212CurrentRow','v212CurrentCol','v212CurrentCross','v212CurrentAxis'))}
function isoParts(date){let d=new Date(Date.UTC(date.getFullYear(),date.getMonth(),date.getDate())),day=d.getUTCDay()||7;d.setUTCDate(d.getUTCDate()+4-day);let y=d.getUTCFullYear(),s=new Date(Date.UTC(y,0,1));return{year:y,week:Math.ceil((((d-s)/86400000)+1)/7)}}
function markCross(table,rowIndex,colIndex){if(!table||rowIndex<1||colIndex<1||rowIndex>=table.rows.length)return;let row=table.rows[rowIndex];if(!row||colIndex>=row.cells.length)return;if(row.cells[0])row.cells[0].classList.add('v212CurrentAxis');if(table.rows[0]&&table.rows[0].cells[colIndex])table.rows[0].cells[colIndex].classList.add('v212CurrentAxis');for(let c=1;c<row.cells.length;c++)row.cells[c].classList.add('v212CurrentRow');for(let r=1;r<table.rows.length;r++)if(table.rows[r].cells[colIndex])table.rows[r].cells[colIndex].classList.add('v212CurrentCol');row.cells[colIndex].classList.add('v212CurrentCross')}
function decorateHeatmaps(){let now=new Date(),iso=isoParts(now),table,wrap,start,end,row,col;
 wrap=$('weekHeat');table=wrap&&wrap.querySelector('table.heat');if(table){clearCross(table);if(Number($('weekYear')&&$('weekYear').value)===iso.year&&Number($('weekNumber')&&$('weekNumber').value)===iso.week){start=Math.max(0,Math.min(23,Number($('heatStartHour')&&$('heatStartHour').value)||5));end=Math.max(start,Math.min(23,Number($('heatEndHour')&&$('heatEndHour').value)||18));if(now.getHours()>=start&&now.getHours()<=end){row=now.getDay()===0?7:now.getDay();col=now.getHours()-start+1;markCross(table,row,col)}}}
 wrap=$('monthWeekHeat');table=wrap&&wrap.querySelector('table.heat');if(table){clearCross(table);if(Number($('monthYear')&&$('monthYear').value)===now.getFullYear())markCross(table,now.getMonth()+1,iso.week)}
 wrap=$('yearMonthHeat');table=wrap&&wrap.querySelector('table.heat');if(table){clearCross(table);row=-1;for(let r=1;r<table.rows.length;r++)if(table.rows[r].cells[0]&&Number(table.rows[r].cells[0].textContent)===now.getFullYear()){row=r;break}if(row>0)markCross(table,row,now.getMonth()+1)}
}
function hardwareFor(name,d){let ok=localText('erkannt','detected','gfunda'),missing=localText('nicht erkannt','not detected','ned gfunda'),active=localText('aktiv','active','aktiv'),internal=localText('intern','internal','intern');switch(name){case'MainLoop':return'ESP32 · '+internal;case'Input':return'GPIO 27 · '+active;case'Mode':return'ESP32 · '+internal;case'Storage':return'LittleFS · '+active;case'Time':return'ESP32 RTC · '+active;case'RTC':return'DS3231 · '+(d.rtcPresent?ok:missing);case'Autark':return'GPIO 33 · '+active;case'Display':return'OLED · '+(d.displayPresent?ok:(d.displaySimulation?localText('Simulation','simulation','Simulation'):missing));case'LED':return'GPIO 2 · '+active;case'Network':return'WLAN · '+(d.wifi?localText('verbunden','connected','verbunda'):localText('offline','offline','offline'));case'Web':return'TCP/IP · '+active;case'Analytics':return'LittleFS · '+active;case'Sound':return'DY-SV17F · '+((d.sound||{}).present?ok:missing);case'Diagnostics':return'ESP32 · '+internal;default:return'-'}}
function cleanDetail(m){let x=String(m.detail||'-');if(['gpio','rings_ok','optional','wifi','hardware_lost'].includes(x))return'-';if(x==='serial')return'Serial 115200';if(x==='loop_cycle')return localText('Zyklus','cycle','Zyklus');if(x==='http_80')return'Port 80';if(x==='cache_ready')return localText('Cache bereit','cache ready','Cache bereit');if(x.startsWith('comm=')){let p={};x.split(';').forEach(v=>{let a=v.split('=');p[a[0]]=a.slice(1).join('=')});let out=[];out.push(p.valid==='yes'?localText('Zeit gültig','time valid','Zeit passt'):localText('Zeit ungültig','time invalid','Zeit passt ned'));out.push(p.osf==='clear'?'OSF OK':'OSF '+(p.osf||'?'));if(p.temp)out.push(p.temp+' °C');if(p.age)out.push(localText('Prüfung ','check ','Prüfung ')+Math.round(Number(p.age)/1000)+' s');return out.join(' · ')}return x.replace(/_/g,' ')}
function ensureHardwareHeader(){let table=document.querySelector('.v2WdTable'),head=table&&table.tHead&&table.tHead.rows[0];if(!head)return;if(!head.querySelector('[data-v212-hardware]')){let th=document.createElement('th');th.dataset.v212Hardware='1';th.textContent='Hardware';head.insertBefore(th,head.cells[2]||null)}}
function decorateWatchdog(){let body=$('watchdogRows');if(!body||!latestStatus)return;ensureHardwareHeader();let mods=latestStatus.modules||[];[...body.rows].forEach((row,i)=>{let m=mods[i];if(!m)return;let cell=row.querySelector('.v212Hardware');if(!cell){cell=document.createElement('td');cell.className='v212Hardware';row.insertBefore(cell,row.cells[2]||null)}cell.textContent=hardwareFor(m.name,latestStatus);let detailIndex=3;if(row.cells[detailIndex])row.cells[detailIndex].textContent=cleanDetail(m)})}
function hookFetch(){let original=window.fetch.bind(window);window.fetch=async function(){let response=await original.apply(null,arguments);try{let url=String(arguments[0]||'');if(url.indexOf('/api/status')>=0&&response.ok)response.clone().json().then(d=>{latestStatus=d;setTimeout(()=>{hideStorageDuplicates();paintMeters(d);decorateWatchdog()},0)}).catch(()=>{})}catch(e){}return response}}
function watch(id,fn){let el=$(id);if(el)new MutationObserver(()=>setTimeout(fn,5)).observe(el,{childList:true,subtree:true})}
function init(){hideStorageDuplicates();hookFetch();watch('weekHeat',decorateHeatmaps);watch('monthWeekHeat',decorateHeatmaps);watch('yearMonthHeat',decorateHeatmaps);watch('watchdogRows',decorateWatchdog);setTimeout(()=>{hideStorageDuplicates();decorateHeatmaps();decorateWatchdog()},80);setInterval(decorateHeatmaps,60000);let selector=$('languageSelect');if(selector)selector.addEventListener('change',()=>setTimeout(()=>{decorateWatchdog();decorateHeatmaps()},10))}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();
</script>
)HTML";
