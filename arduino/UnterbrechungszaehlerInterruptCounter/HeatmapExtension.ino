// HeatmapExtension.ino
// Lightweight browser-side extension for settings and long-term heatmaps.
// Heatmaps use /api/aggregate so the browser never needs the complete archive.

static const char HEATMAP_EXTENSION_JS[] PROGMEM = R"JS(
(function(){
'use strict';

var lastAggregate=null;

function q(id){return document.getElementById(id)}
function pad(n){return String(n).padStart(2,'0')}
function getRange(){
  var s=parseInt(localStorage.getItem('uic-heat-start'),10);
  var e=parseInt(localStorage.getItem('uic-heat-end'),10);
  if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){s=5;e=18}
  return{s:s,e:e};
}

function buildUi(){
  var tabs=document.querySelector('.tabs');
  var autarkTab=document.querySelector('.tab[data-view="autark"]');
  if(tabs&&!document.querySelector('.tab[data-view="settings"]')){
    var b=document.createElement('button');
    b.className='tab';b.dataset.view='settings';
    b.innerHTML='<span class="ico">&#9881;</span><span class="label">Einstellungen</span>';
    tabs.insertBefore(b,autarkTab||null);
    b.addEventListener('click',function(){switchExtView(b,'settings')});
  }

  var panel=document.querySelector('.tabpanel');
  if(panel&&!q('settings')){
    var s=document.createElement('section');s.id='settings';s.className='view';
    s.innerHTML='<div class="infoGrid"><div id="extOriginalSettings"></div><div class="infoBox"><h3><span class="infoIcon">&#128293;</span>Heatmap-Zeitbereich</h3><div class="settingsRow"><label for="heatStartHour">Startstunde</label><input id="heatStartHour" type="number" min="0" max="23" step="1" style="font:inherit;padding:8px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)"></div><div class="settingsRow"><label for="heatEndHour">Endstunde</label><input id="heatEndHour" type="number" min="0" max="23" step="1" style="font:inherit;padding:8px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)"></div><div class="actions"><button id="heatRangeSaveBtn" class="btn" type="button">Zeitbereich speichern</button><span id="heatRangeState" class="small"></span></div><p class="small">Standard 05 bis 18 Uhr, Endstunde inklusive. Start muss kleiner als Ende sein.</p></div></div>';
    panel.insertBefore(s,q('device')||q('autark'));
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
    q('heatRangeSaveBtn').dataset.bound='1';q('heatRangeSaveBtn').addEventListener('click',saveRange);
  }
  if(q('monthWeekYear')&&!q('monthWeekYear').dataset.bound){
    q('monthWeekYear').dataset.bound='1';
    q('monthWeekYear').addEventListener('change',function(){refreshHeatmaps(parseInt(this.value,10))});
  }
}

function switchExtView(btn,id){
  document.querySelectorAll('.tab').forEach(function(x){x.classList.remove('active')});
  document.querySelectorAll('.view').forEach(function(x){x.classList.remove('active')});
  btn.classList.add('active');var v=q(id);if(v)v.classList.add('active');
}

function saveRange(){
  var s=parseInt(q('heatStartHour').value,10),e=parseInt(q('heatEndHour').value,10),out=q('heatRangeState');
  if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){out.style.color='var(--danger)';out.textContent='Ungueltiger Bereich: 0 bis 23 Uhr, Start muss kleiner als Ende sein.';return}
  localStorage.setItem('uic-heat-start',String(s));localStorage.setItem('uic-heat-end',String(e));
  out.style.color='var(--ok)';out.textContent='Gespeichert: '+pad(s)+':00 bis '+pad(e)+':00 Uhr';
  if(lastAggregate)renderWeek(lastAggregate);
}

function maxIn2d(a){var max=1;(a||[]).forEach(function(row){(row||[]).forEach(function(v){if(v>max)max=v})});return max}

function renderWeek(data){
  var wrap=q('heatWrap');if(!wrap)return;
  var r=getRange(),names=['So','Mo','Di','Mi','Do','Fr','Sa'],a=data.weekdayHour||[],max=1;
  [1,2,3,4,5,6,0].forEach(function(day){for(var h=r.s;h<=r.e;h++){var v=(a[day]&&a[day][h])||0;if(v>max)max=v}});
  var html='<table class="heat"><thead><tr><th></th>';for(var h=r.s;h<=r.e;h++)html+='<th>'+pad(h)+'</th>';html+='</tr></thead><tbody>';
  [1,2,3,4,5,6,0].forEach(function(day){html+='<tr><th>'+names[day]+'</th>';for(var hour=r.s;hour<=r.e;hour++){var v=(a[day]&&a[day][hour])||0,alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}html+='</tr>'});
  wrap.innerHTML=html+'</tbody></table>';
}

function updateYearSelect(data){
  var sel=q('monthWeekYear');if(!sel)return;
  var old=parseInt(sel.value,10),years=(data.years||[]).slice();
  if(years.indexOf(data.baseYear)<0)years.unshift(data.baseYear);
  years=years.filter(function(v,i,a){return a.indexOf(v)===i}).sort(function(a,b){return b-a});
  sel.innerHTML=years.map(function(y){return '<option value="'+y+'">'+y+'</option>'}).join('');
  if(years.indexOf(data.selectedYear)>=0)sel.value=data.selectedYear;
  else if(years.indexOf(old)>=0)sel.value=old;
  else sel.value=data.baseYear;
}

function renderMonthWeek(data){
  var wrap=q('monthWeekHeatWrap');if(!wrap)return;
  updateYearSelect(data);
  var rows=data.monthWeek||[],max=maxIn2d(rows),months=['Jan','Feb','Maer','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'];
  var html='<table class="heat" style="min-width:2250px"><thead><tr><th style="position:sticky;left:0;z-index:3;background:var(--card);min-width:64px">Monat</th>';
  for(var w=1;w<=52;w++)html+='<th style="min-width:38px">KW '+w+'</th>';html+='</tr></thead><tbody>';
  for(var m=0;m<12;m++){
    html+='<tr><th style="position:sticky;left:0;z-index:2;background:var(--card);min-width:64px">'+months[m]+'</th>';
    for(var w=0;w<52;w++){var v=(rows[m]&&rows[m][w])||0,alpha=v?0.15+0.75*(v/max):0;html+='<td title="'+months[m]+' '+data.selectedYear+' / KW '+(w+1)+': '+v+'" style="min-width:38px;background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}
    html+='</tr>';
  }
  wrap.innerHTML=html+'</tbody></table>';
}

function renderYearMonth(data){
  var wrap=q('yearMonthHeatWrap');if(!wrap)return;
  var rows=data.yearMonth||[],max=maxIn2d(rows),months=['Jan','Feb','Maer','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'];
  var html='<table class="heat"><thead><tr><th></th>';months.forEach(function(m){html+='<th>'+m+'</th>'});html+='</tr></thead><tbody>';
  for(var y=0;y<5;y++){
    var year=data.baseYear-y;html+='<tr><th>'+year+'</th>';
    for(var m=0;m<12;m++){var v=(rows[y]&&rows[y][m])||0,alpha=v?0.15+0.75*(v/max):0;html+='<td title="'+months[m]+' '+year+': '+v+'" style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}
    html+='</tr>';
  }
  wrap.innerHTML=html+'</tbody></table>';
}

async function refreshHeatmaps(year){
  try{
    var selected=Number.isInteger(year)?year:(q('monthWeekYear')&&parseInt(q('monthWeekYear').value,10));
    var url='/api/aggregate?x='+Date.now();if(Number.isInteger(selected))url+='&year='+selected;
    var r=await fetch(url,{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);
    var data=await r.json();if(!data.ok)throw new Error('Aggregat nicht verfuegbar');
    lastAggregate=data;renderWeek(data);renderMonthWeek(data);renderYearMonth(data);
  }catch(err){
    var text='<span class="muted">Fehler beim Laden: '+err.message+'</span>';
    if(q('heatWrap'))q('heatWrap').innerHTML=text;if(q('monthWeekHeatWrap'))q('monthWeekHeatWrap').innerHTML=text;if(q('yearMonthHeatWrap'))q('yearMonthHeatWrap').innerHTML=text;
  }
}

function bindExistingTabs(){var hb=document.querySelector('.tab[data-view="heatmap"]');if(hb&&!hb.dataset.extBound){hb.dataset.extBound='1';hb.addEventListener('click',function(){setTimeout(function(){refreshHeatmaps()},30)})}}

buildUi();bindExistingTabs();
setTimeout(function(){buildUi();bindExistingTabs();refreshHeatmaps()},100);
setTimeout(function(){refreshHeatmaps()},800);
setInterval(function(){var v=q('heatmap');if(v&&v.classList.contains('active'))refreshHeatmaps()},15000);
})();
)JS";

static void serveHeatmapExtensionJs() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "application/javascript; charset=utf-8", HEATMAP_EXTENSION_JS);
}

static void serveHeatmapExtendedIndex() {
  String page = FPSTR(INDEX_HTML);
  const char* includeScript = "<script src=\"/heatmap-extension.js?v=16\"></script></body>";
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
