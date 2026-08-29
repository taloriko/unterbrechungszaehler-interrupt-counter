// HeatmapExtension.ino
// Lightweight browser-side extension for settings and additional heatmaps.
// Only a tiny script reference is injected into INDEX_HTML. The actual JS is
// served separately so the ESP32 does not need to rebuild a much larger HTML
// page in RAM for every request.

static const char HEATMAP_EXTENSION_JS[] PROGMEM = R"JS(
(function(){
'use strict';

var lastHeatEvents=[];

function q(id){return document.getElementById(id)}
function pad(n){return String(n).padStart(2,'0')}
function getRange(){
  var s=parseInt(localStorage.getItem('uic-heat-start'),10);
  var e=parseInt(localStorage.getItem('uic-heat-end'),10);
  if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){s=5;e=18}
  return{s:s,e:e};
}
function isoWeek(d){
  var x=new Date(Date.UTC(d.getFullYear(),d.getMonth(),d.getDate()));
  var day=x.getUTCDay()||7;
  x.setUTCDate(x.getUTCDate()+4-day);
  var yearStart=new Date(Date.UTC(x.getUTCFullYear(),0,1));
  return Math.ceil((((x-yearStart)/86400000)+1)/7);
}

function buildUi(){
  var tabs=document.querySelector('.tabs');
  var autarkTab=document.querySelector('.tab[data-view="autark"]');
  if(tabs&&!document.querySelector('.tab[data-view="settings"]')){
    var b=document.createElement('button');
    b.className='tab';
    b.dataset.view='settings';
    b.innerHTML='<span class="ico">&#9881;</span><span class="label">Einstellungen</span>';
    tabs.insertBefore(b,autarkTab||null);
    b.addEventListener('click',function(){switchExtView(b,'settings')});
  }

  var panel=document.querySelector('.tabpanel');
  if(panel&&!q('settings')){
    var s=document.createElement('section');
    s.id='settings';s.className='view';
    s.innerHTML='<div class="infoGrid"><div id="extOriginalSettings"></div><div class="infoBox"><h3><span class="infoIcon">&#128293;</span>Heatmap-Zeitbereich</h3><div class="settingsRow"><label for="heatStartHour">Startstunde</label><input id="heatStartHour" type="number" min="0" max="23" step="1" style="font:inherit;padding:8px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)"></div><div class="settingsRow"><label for="heatEndHour">Endstunde</label><input id="heatEndHour" type="number" min="0" max="23" step="1" style="font:inherit;padding:8px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)"></div><div class="actions"><button id="heatRangeSaveBtn" class="btn" type="button">Zeitbereich speichern</button><span id="heatRangeState" class="small"></span></div><p class="small">Standard 05 bis 18 Uhr, Endstunde inklusive. Start muss kleiner als Ende sein.</p></div></div>';
    var device=q('device');
    panel.insertBefore(s,device||q('autark'));
  }

  var deviceGrid=q('device')&&q('device').querySelector('.infoGrid');
  var settingsHost=q('extOriginalSettings');
  if(deviceGrid&&settingsHost){
    var original=deviceGrid.querySelector('.infoBox');
    if(original&&!settingsHost.contains(original))settingsHost.appendChild(original);
  }

  var heat=q('heatmap');
  if(heat&&!q('monthWeekHeatWrap')){
    var root=heat.querySelector('.grid');
    if(!root){
      var oldCard=heat.querySelector('.card');
      root=document.createElement('div');root.className='grid';
      if(oldCard){oldCard.classList.add('span12');root.appendChild(oldCard)}
      heat.appendChild(root);
    }
    var c1=document.createElement('div');c1.className='card span12';
    c1.innerHTML='<div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap"><h2 style="margin-bottom:0">Heatmap - Monat / Kalenderwoche</h2><label class="small">Jahr <select id="monthWeekYear" style="margin-left:6px"></select></label></div><div id="monthWeekHeatWrap" class="heatwrap" style="margin-top:12px"><span class="muted">Lade Daten...</span></div>';
    var c2=document.createElement('div');c2.className='card span12';
    c2.innerHTML='<h2>Heatmap - Jahr / Monat</h2><div id="yearMonthHeatWrap" class="heatwrap"><span class="muted">Lade Daten...</span></div>';
    root.appendChild(c1);root.appendChild(c2);
  }

  var r=getRange();
  if(q('heatStartHour'))q('heatStartHour').value=r.s;
  if(q('heatEndHour'))q('heatEndHour').value=r.e;
  if(q('heatRangeSaveBtn')&&!q('heatRangeSaveBtn').dataset.bound){
    q('heatRangeSaveBtn').dataset.bound='1';
    q('heatRangeSaveBtn').addEventListener('click',saveRange);
  }
  if(q('monthWeekYear')&&!q('monthWeekYear').dataset.bound){
    q('monthWeekYear').dataset.bound='1';
    q('monthWeekYear').addEventListener('change',function(){renderMonthWeek(lastHeatEvents)});
  }
}

function switchExtView(btn,id){
  document.querySelectorAll('.tab').forEach(function(x){x.classList.remove('active')});
  document.querySelectorAll('.view').forEach(function(x){x.classList.remove('active')});
  btn.classList.add('active');
  var v=q(id);if(v)v.classList.add('active');
}

function saveRange(){
  var s=parseInt(q('heatStartHour').value,10);
  var e=parseInt(q('heatEndHour').value,10);
  var out=q('heatRangeState');
  if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){
    out.style.color='var(--danger)';
    out.textContent='Ungueltiger Bereich: 0 bis 23 Uhr, Start muss kleiner als Ende sein.';
    return;
  }
  localStorage.setItem('uic-heat-start',String(s));
  localStorage.setItem('uic-heat-end',String(e));
  out.style.color='var(--ok)';
  out.textContent='Gespeichert: '+pad(s)+':00 bis '+pad(e)+':00 Uhr';
  refreshHeatmaps();
}

function renderWeek(ev){
  var wrap=q('heatWrap');if(!wrap)return;
  var r=getRange(),names=['So','Mo','Di','Mi','Do','Fr','Sa'],a=[],max=1;
  for(var d=0;d<7;d++)a[d]=new Array(24).fill(0);
  ev.forEach(function(ts){var x=new Date(ts*1000),h=x.getHours();if(h<r.s||h>r.e)return;a[x.getDay()][h]++;max=Math.max(max,a[x.getDay()][h])});
  var html='<table class="heat"><thead><tr><th></th>';
  for(var h=r.s;h<=r.e;h++)html+='<th>'+pad(h)+'</th>';
  html+='</tr></thead><tbody>';
  [1,2,3,4,5,6,0].forEach(function(day){html+='<tr><th>'+names[day]+'</th>';for(var hour=r.s;hour<=r.e;hour++){var v=a[day][hour],alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}html+='</tr>'});
  wrap.innerHTML=html+'</tbody></table>';
}

function availableYears(ev){
  var current=new Date().getFullYear(),set={};set[current]=true;
  ev.forEach(function(ts){set[new Date(ts*1000).getFullYear()]=true});
  return Object.keys(set).map(Number).sort(function(a,b){return b-a});
}

function updateYearSelect(ev){
  var sel=q('monthWeekYear');if(!sel)return;
  var current=new Date().getFullYear();
  var selected=parseInt(sel.value,10);
  var years=availableYears(ev);
  sel.innerHTML=years.map(function(y){return '<option value="'+y+'">'+y+'</option>'}).join('');
  if(years.indexOf(selected)>=0)sel.value=selected;
  else if(years.indexOf(current)>=0)sel.value=current;
  else sel.value=years[0];
}

function renderMonthWeek(ev){
  var wrap=q('monthWeekHeatWrap');if(!wrap)return;
  updateYearSelect(ev);
  var sel=q('monthWeekYear');
  var year=sel?parseInt(sel.value,10):new Date().getFullYear();
  var rows=[];for(var m=0;m<12;m++)rows[m]=new Array(52).fill(0);
  var max=1;
  ev.forEach(function(ts){
    var d=new Date(ts*1000);if(d.getFullYear()!==year)return;
    var w=isoWeek(d);if(w<1)return;if(w>52)w=52;
    rows[d.getMonth()][w-1]++;max=Math.max(max,rows[d.getMonth()][w-1]);
  });
  var months=['Jan','Feb','Maer','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'];
  var html='<table class="heat" style="min-width:2250px"><thead><tr><th style="position:sticky;left:0;z-index:3;background:var(--card);min-width:64px">Monat</th>';
  for(var w=1;w<=52;w++)html+='<th style="min-width:38px">KW '+w+'</th>';
  html+='</tr></thead><tbody>';
  for(var m=0;m<12;m++){
    html+='<tr><th style="position:sticky;left:0;z-index:2;background:var(--card);min-width:64px">'+months[m]+'</th>';
    for(var w=0;w<52;w++){
      var v=rows[m][w],alpha=v?0.15+0.75*(v/max):0;
      html+='<td title="'+months[m]+' '+year+' / KW '+(w+1)+': '+v+'" style="min-width:38px;background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>';
    }
    html+='</tr>';
  }
  wrap.innerHTML=html+'</tbody></table>';
}

function renderYearMonth(ev){
  var wrap=q('yearMonthHeatWrap');if(!wrap)return;
  var currentYear=new Date().getFullYear(),years={},max=1;
  for(var y=currentYear;y>=currentYear-4;y--)years[y]=new Array(12).fill(0);
  ev.forEach(function(ts){var d=new Date(ts*1000),y=d.getFullYear(),m=d.getMonth();if(!years[y])return;years[y][m]++;max=Math.max(max,years[y][m])});
  var months=['Jan','Feb','Maer','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'];
  var html='<table class="heat"><thead><tr><th></th>';months.forEach(function(m){html+='<th>'+m+'</th>'});html+='</tr></thead><tbody>';
  for(var y=currentYear;y>=currentYear-4;y--){
    html+='<tr><th>'+y+'</th>';
    for(var m=0;m<12;m++){var v=years[y][m],alpha=v?0.15+0.75*(v/max):0;html+='<td title="'+months[m]+' '+y+': '+v+'" style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}
    html+='</tr>';
  }
  wrap.innerHTML=html+'</tbody></table>';
}

async function refreshHeatmaps(){
  try{
    var r=await fetch('/api/events?heat='+Date.now(),{cache:'no-store'});
    if(!r.ok)throw new Error('HTTP '+r.status);
    var d=await r.json(),ev=(d.events||[]).sort(function(a,b){return a-b});
    lastHeatEvents=ev;
    renderWeek(ev);renderMonthWeek(ev);renderYearMonth(ev);
  }catch(err){
    var text='<span class="muted">Fehler beim Laden: '+err.message+'</span>';
    if(q('heatWrap'))q('heatWrap').innerHTML=text;
    if(q('monthWeekHeatWrap'))q('monthWeekHeatWrap').innerHTML=text;
    if(q('yearMonthHeatWrap'))q('yearMonthHeatWrap').innerHTML=text;
  }
}

function bindExistingTabs(){
  var hb=document.querySelector('.tab[data-view="heatmap"]');
  if(hb&&!hb.dataset.extBound){hb.dataset.extBound='1';hb.addEventListener('click',function(){setTimeout(refreshHeatmaps,30)})}
}

buildUi();bindExistingTabs();
setTimeout(function(){buildUi();bindExistingTabs();refreshHeatmaps()},100);
setTimeout(refreshHeatmaps,800);
setInterval(function(){var v=q('heatmap');if(v&&v.classList.contains('active'))refreshHeatmaps()},3000);
})();
)JS";

static void serveHeatmapExtensionJs() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "application/javascript; charset=utf-8", HEATMAP_EXTENSION_JS);
}

static void serveHeatmapExtendedIndex() {
  String page = FPSTR(INDEX_HTML);
  const char* includeScript = "<script src=\"/heatmap-extension.js?v=15\"></script></body>";
  page.replace("</body>", includeScript);
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "text/html; charset=utf-8", page);
}

class HeatmapExtensionRegistrar {
public:
  HeatmapExtensionRegistrar() {
    server.on("/", HTTP_GET, serveHeatmapExtendedIndex);
    server.on("/heatmap-extension.js", HTTP_GET, serveHeatmapExtensionJs);
  }
};
HeatmapExtensionRegistrar heatmapExtensionRegistrar;
