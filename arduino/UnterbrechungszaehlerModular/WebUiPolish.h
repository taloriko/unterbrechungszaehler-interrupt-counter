#pragma once

#include <Arduino.h>

// Visueller Feinschliff fuer Kopfbereich und Heatmaps.
// Die Heatmap-Berechnung selbst bleibt im AnalyticsService; hier werden nur
// aktuelle Zeitbereiche hervorgehoben und Scrollpositionen sinnvoll gesetzt.
static const char WEB_UI_POLISH[] PROGMEM = R"HTML(
<style>
/* Der Kopfbereich greift dieselbe Karten-Sprache wie der restliche Inhalt auf. */
header{
  min-height:54px!important;
  padding:9px 13px;
  background:var(--card);
  border:1px solid var(--line);
  border-radius:12px;
  box-shadow:var(--shadow);
}
.tabs{margin-top:10px!important}
.headTime #deviceClock{font-weight:600;color:var(--text)}
.headTitle h1{font-size:1.18rem!important;font-weight:650!important}
.titleIcon{
  width:28px;height:28px;display:inline-flex;align-items:center;justify-content:center;
  border:1px solid var(--line);border-radius:8px;background:var(--bg);font-size:1rem!important
}
header .hwIcon{background:var(--bg)}
.headerWifiText{font-weight:500}

/* Aktuelle Positionen in den Heatmaps sind klar, aber nicht aufdringlich markiert. */
.heat .heatCurrentLabel{
  color:var(--accent)!important;font-weight:700!important;
  box-shadow:inset 0 0 0 2px var(--accent);border-radius:6px
}
.heat td.heatCurrentCell{
  border-color:var(--accent)!important;
  box-shadow:inset 0 0 0 1px var(--accent)
}
.heat tr.heatTodayRow td{
  border-top-color:var(--accent)!important;
  border-bottom-color:var(--accent)!important
}
.heat tr.heatTodayRow th:first-child{
  color:var(--accent)!important;font-weight:700;
  box-shadow:inset 0 0 0 2px var(--accent);border-radius:6px
}
.heat .heatCurrentAxis{color:var(--accent)!important;font-weight:700!important}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);

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
  const left=target.offsetLeft-(wrap.clientWidth-target.offsetWidth)/2;
  wrap.scrollLeft=Math.max(0,left);
}

function centerBoth(wrap,target,key){
  if(!wrap||!target)return;
  if(wrap.dataset.focusBoth===key)return;
  wrap.dataset.focusBoth=key;
  if(wrap.scrollWidth>wrap.clientWidth){
    wrap.scrollLeft=Math.max(0,target.offsetLeft-(wrap.clientWidth-target.offsetWidth)/2);
  }
  if(wrap.scrollHeight>wrap.clientHeight){
    wrap.scrollTop=Math.max(0,target.offsetTop-(wrap.clientHeight-target.offsetHeight)/2);
  }
}

function clearMarks(table){
  if(!table)return;
  table.querySelectorAll('.heatCurrentLabel,.heatCurrentCell,.heatCurrentAxis,.heatTodayRow').forEach(el=>{
    el.classList.remove('heatCurrentLabel','heatCurrentCell','heatCurrentAxis','heatTodayRow');
  });
}

function decorateWeek(now,iso){
  const wrap=$('weekHeat');
  const table=wrap&&wrap.querySelector('table.heat');
  if(!table)return;
  clearMarks(table);

  const selectedYear=Number($('weekYear')&&$('weekYear').value);
  const selectedWeek=Number($('weekNumber')&&$('weekNumber').value);
  if(selectedYear!==iso.year||selectedWeek!==iso.week)return;

  const rows=table.rows;
  const day=now.getDay();
  const rowIndex=day===0?7:day; // Kopfzeile=0, Montag=1 ... Sonntag=7
  const row=rows[rowIndex];
  if(!row)return;
  row.classList.add('heatTodayRow');
  if(row.cells[0])row.cells[0].classList.add('heatCurrentLabel');

  const start=Math.max(0,Math.min(23,Number($('heatStartHour')&&$('heatStartHour').value)||5));
  const end=Math.max(start,Math.min(23,Number($('heatEndHour')&&$('heatEndHour').value)||18));
  const hour=now.getHours();
  if(hour>=start&&hour<=end){
    const col=hour-start+1;
    if(rows[0]&&rows[0].cells[col])rows[0].cells[col].classList.add('heatCurrentAxis');
    if(row.cells[col]){
      row.cells[col].classList.add('heatCurrentCell');
      centerHorizontal(wrap,row.cells[col],selectedYear+'-'+selectedWeek+'-'+hour);
    }
  }else if(row.cells[0]){
    centerHorizontal(wrap,row.cells[0],selectedYear+'-'+selectedWeek+'-day');
  }
}

function decorateMonthWeek(now,iso){
  const wrap=$('monthWeekHeat');
  const table=wrap&&wrap.querySelector('table.heat');
  if(!table)return;
  clearMarks(table);

  const selectedYear=Number($('monthYear')&&$('monthYear').value);
  if(selectedYear!==now.getFullYear())return;
  const col=iso.week; // Zelle 0 ist Monatsname, KW1 liegt auf Index 1.
  const row=now.getMonth()+1; // Kopfzeile=0, Januar=1.
  if(table.rows[0]&&table.rows[0].cells[col])table.rows[0].cells[col].classList.add('heatCurrentLabel');
  if(table.rows[row]){
    if(table.rows[row].cells[0])table.rows[row].cells[0].classList.add('heatCurrentAxis');
    if(table.rows[row].cells[col]){
      table.rows[row].cells[col].classList.add('heatCurrentCell');
      centerHorizontal(wrap,table.rows[row].cells[col],selectedYear+'-'+iso.week);
    }
  }
}

function decorateYearMonth(now){
  const wrap=$('yearMonthHeat');
  const table=wrap&&wrap.querySelector('table.heat');
  if(!table)return;
  clearMarks(table);

  const year=String(now.getFullYear());
  let row=null;
  for(let i=1;i<table.rows.length;i++){
    if(table.rows[i].cells[0]&&table.rows[i].cells[0].textContent.trim()===year){row=table.rows[i];break}
  }
  if(!row)return;
  const col=now.getMonth()+1;
  if(row.cells[0])row.cells[0].classList.add('heatCurrentLabel');
  if(table.rows[0]&&table.rows[0].cells[col])table.rows[0].cells[col].classList.add('heatCurrentAxis');
  if(row.cells[col]){
    row.cells[col].classList.add('heatCurrentCell');
    centerBoth(wrap,row.cells[col],year+'-'+col);
  }
}

function decorateHeatmaps(){
  const now=new Date();
  const iso=isoParts(now);
  decorateWeek(now,iso);
  decorateMonthWeek(now,iso);
  decorateYearMonth(now);
}

function watch(id){
  const el=$(id);
  if(!el)return;
  const observer=new MutationObserver(()=>setTimeout(decorateHeatmaps,0));
  observer.observe(el,{childList:true});
}

watch('weekHeat');
watch('monthWeekHeat');
watch('yearMonthHeat');
document.querySelectorAll('.tab[data-view="heatmap"]').forEach(tab=>tab.addEventListener('click',()=>setTimeout(decorateHeatmaps,30)));
['weekYear','weekNumber','monthYear','heatStartHour','heatEndHour'].forEach(id=>{
  const el=$(id);if(el)el.addEventListener('change',()=>setTimeout(decorateHeatmaps,30));
});
setTimeout(decorateHeatmaps,50);
})();
</script>
)HTML";
