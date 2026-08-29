// HeatmapExtension.ino
// Self-contained settings and heatmap extension.

static const char* HEATMAP_EXTENSION_VERSION = "2026-08-29-13";

class HeatmapVersionInitializer {
public:
  HeatmapVersionInitializer() { APP_VERSION = HEATMAP_EXTENSION_VERSION; }
};
HeatmapVersionInitializer heatmapVersionInitializer;

static void serveHeatmapExtendedIndex() {
  String page = FPSTR(INDEX_HTML);
  page.reserve(page.length() + 15000);

  const String extraCss = R"CSS(
#device .infoGrid>.infoBox:first-child{display:none}.settingsGrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.settingsCard{border:1px solid var(--line);border-radius:10px;padding:14px}.settingsCard h3{font-size:.95rem;margin:0 0 12px}.settingsLine{display:grid;grid-template-columns:145px minmax(0,1fr);gap:10px;align-items:center;margin:10px 0}.settingsLine select,.settingsLine input{font:inherit;padding:8px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)}.extTheme{display:flex;gap:5px;flex-wrap:wrap}.heatSettings{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.heatSettings label{display:grid;gap:5px;font-size:.82rem;color:var(--muted)}.heatSettings input{width:100%;font:inherit;padding:8px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)}.heatSettingsState.ok{color:var(--ok)}.heatSettingsState.err{color:var(--danger)}.yearHeat{min-width:650px}.monthWeekHeat{min-width:430px}@media(max-width:800px){.settingsGrid{grid-template-columns:1fr}}@media(max-width:520px){.settingsLine,.heatSettings{grid-template-columns:1fr}}
)CSS";
  page.replace("</style>", extraCss + "</style>");

  const String autarkTab = R"HTML(<button class="tab" data-view="autark"><span class="ico">&#128267;</span><span class="label" data-i18n="tab.autark">Autark</span><span class="beta">BETA</span></button>)HTML";
  const String tabs = R"HTML(<button class="tab" data-view="settings"><span class="ico">&#9881;</span><span class="label">Einstellungen</span></button><button class="tab" data-view="autark"><span class="ico">&#128267;</span><span class="label" data-i18n="tab.autark">Autark</span><span class="beta">BETA</span></button>)HTML";
  page.replace(autarkTab, tabs);

  const String oldHeatmap = R"HTML(<section id="heatmap" class="view"><div class="card"><h2 data-i18n="heatmap.title">Heatmap - Wochentag / Uhrzeit</h2><div id="heatWrap" class="heatwrap"></div></div></section>)HTML";
  const String newHeatmap = R"HTML(<section id="heatmap" class="view"><div class="grid"><div class="card span12"><h2 data-i18n="heatmap.title">Heatmap - Wochentag / Uhrzeit</h2><div id="heatWrap" class="heatwrap"><span class="muted">Lade Daten...</span></div></div><div class="card span12"><h2>Heatmap - Monat / Woche</h2><div id="monthWeekHeatWrap" class="heatwrap"><span class="muted">Lade Daten...</span></div></div><div class="card span12"><h2>Heatmap - Jahr / Monat</h2><div id="yearMonthHeatWrap" class="heatwrap"><span class="muted">Lade Daten...</span></div></div></div></section>)HTML";
  page.replace(oldHeatmap, newHeatmap);

  const String deviceSection = R"HTML(<section id="device" class="view">)HTML";
  const String settingsView = R"HTML(<section id="settings" class="view"><div class="settingsGrid"><div class="settingsCard"><h3>Sprache & Darstellung</h3><div class="settingsLine"><label for="extLanguage">Sprache</label><select id="extLanguage"><option value="de">Deutsch</option><option value="sw">Schwaebisch</option><option value="en">English</option><option value="it">Italiano</option><option value="fr">Francais</option></select></div><div class="settingsLine"><span>Darstellung</span><div class="extTheme"><button type="button" data-ext-theme="system">System</button><button type="button" data-ext-theme="light">&#9728;&#65039;</button><button type="button" data-ext-theme="dark">&#127769;</button></div></div><p class="small">Sprache und Darstellung werden wie bisher lokal auf diesem Endgeraet gespeichert.</p></div><div class="settingsCard"><h3>Heatmap-Zeitbereich</h3><div class="heatSettings"><label>Startstunde<input id="heatStartHour" type="number" min="0" max="23" step="1" value="5"></label><label>Endstunde<input id="heatEndHour" type="number" min="0" max="23" step="1" value="18"></label></div><div class="actions" style="margin-top:10px"><button id="heatRangeSaveBtn" class="btn" type="button">Zeitbereich speichern</button><span id="heatRangeState" class="small heatSettingsState"></span></div><p class="small">Standard 05 bis 18 Uhr, Endstunde inklusive. Start muss kleiner als Ende sein.</p></div></div></section><section id="device" class="view">)HTML";
  page.replace(deviceSection, settingsView);

  const String extensionScript = R"HTML(
<script>
(function(){
'use strict';
function q(id){return document.getElementById(id)}
function pad(n){return String(n).padStart(2,'0')}
function getRange(){var s=parseInt(localStorage.getItem('uic-heat-start'),10);var e=parseInt(localStorage.getItem('uic-heat-end'),10);if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){s=5;e=18}return{s:s,e:e}}
function initSettings(){var r=getRange();q('heatStartHour').value=r.s;q('heatEndHour').value=r.e;var hiddenLang=q('languageSelect');q('extLanguage').value=hiddenLang?hiddenLang.value:(localStorage.getItem('uic-lang')||'de')}
function saveRange(){var s=parseInt(q('heatStartHour').value,10);var e=parseInt(q('heatEndHour').value,10);var out=q('heatRangeState');if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){out.className='small heatSettingsState err';out.textContent='Ungueltiger Bereich: 0 bis 23 Uhr, Start muss kleiner als Ende sein.';return}localStorage.setItem('uic-heat-start',String(s));localStorage.setItem('uic-heat-end',String(e));out.className='small heatSettingsState ok';out.textContent='Gespeichert: '+pad(s)+':00 bis '+pad(e)+':00 Uhr';refreshHeatmaps()}
function renderWeek(ev){var wrap=q('heatWrap');var r=getRange();var names=['So','Mo','Di','Mi','Do','Fr','Sa'];var a=[];var max=1;for(var d=0;d<7;d++)a[d]=new Array(24).fill(0);ev.forEach(function(ts){var x=new Date(ts*1000);var h=x.getHours();if(h<r.s||h>r.e)return;a[x.getDay()][h]++;max=Math.max(max,a[x.getDay()][h])});var html='<table class="heat"><thead><tr><th></th>';for(var h=r.s;h<=r.e;h++)html+='<th>'+pad(h)+'</th>';html+='</tr></thead><tbody>';[1,2,3,4,5,6,0].forEach(function(day){html+='<tr><th>'+names[day]+'</th>';for(var hour=r.s;hour<=r.e;hour++){var v=a[day][hour];var alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}html+='</tr>'});html+='</tbody></table>';wrap.innerHTML=html}
function renderMonthWeek(ev){var wrap=q('monthWeekHeatWrap');var rows={};var max=1;ev.forEach(function(ts){var d=new Date(ts*1000);var key=d.getFullYear()+'-'+pad(d.getMonth()+1);var w=Math.floor((d.getDate()-1)/7);if(!rows[key])rows[key]=new Array(5).fill(0);rows[key][w]++;max=Math.max(max,rows[key][w])});var keys=Object.keys(rows).sort().reverse();if(!keys.length){wrap.innerHTML='<span class="muted">Noch keine Daten.</span>';return}var html='<table class="heat monthWeekHeat"><thead><tr><th></th><th>1-7</th><th>8-14</th><th>15-21</th><th>22-28</th><th>29-Ende</th></tr></thead><tbody>';keys.forEach(function(k){var p=k.split('-');var label=p[1]+'.'+p[0];html+='<tr><th>'+label+'</th>';for(var w=0;w<5;w++){var v=rows[k][w];var alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}html+='</tr>'});html+='</tbody></table>';wrap.innerHTML=html}
function renderYearMonth(ev){var wrap=q('yearMonthHeatWrap');var years={};var max=1;ev.forEach(function(ts){var d=new Date(ts*1000);var y=d.getFullYear();var m=d.getMonth();if(!years[y])years[y]=new Array(12).fill(0);years[y][m]++;max=Math.max(max,years[y][m])});var ys=Object.keys(years).map(Number).sort(function(a,b){return b-a});if(!ys.length){wrap.innerHTML='<span class="muted">Noch keine Daten.</span>';return}var months=['Jan','Feb','Maer','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'];var html='<table class="heat yearHeat"><thead><tr><th></th>';months.forEach(function(m){html+='<th>'+m+'</th>'});html+='</tr></thead><tbody>';ys.forEach(function(y){html+='<tr><th>'+y+'</th>';for(var m=0;m<12;m++){var v=years[y][m];var alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}html+='</tr>'});html+='</tbody></table>';wrap.innerHTML=html}
async function refreshHeatmaps(){try{var r=await fetch('/api/events?heat='+Date.now(),{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);var d=await r.json();var ev=(d.events||[]).sort(function(a,b){return a-b});renderWeek(ev);renderMonthWeek(ev);renderYearMonth(ev)}catch(err){var text='<span class="muted">Fehler beim Laden: '+err.message+'</span>';q('heatWrap').innerHTML=text;q('monthWeekHeatWrap').innerHTML=text;q('yearMonthHeatWrap').innerHTML=text}}
q('heatRangeSaveBtn').addEventListener('click',saveRange);
q('extLanguage').addEventListener('change',function(){var hidden=q('languageSelect');if(hidden){hidden.value=this.value;hidden.dispatchEvent(new Event('change',{bubbles:true}))}});
document.querySelectorAll('[data-ext-theme]').forEach(function(btn){btn.addEventListener('click',function(){var hidden=document.querySelector('[data-theme-choice="'+btn.dataset.extTheme+'"]');if(hidden)hidden.click()})});
document.querySelectorAll('.tab[data-view="heatmap"]').forEach(function(btn){btn.addEventListener('click',function(){setTimeout(refreshHeatmaps,20)})});
initSettings();
setTimeout(refreshHeatmaps,250);
setTimeout(refreshHeatmaps,1200);
setInterval(function(){var v=q('heatmap');if(v&&v.classList.contains('active'))refreshHeatmaps()},3000);
})();
</script>
)HTML";
  page.replace("</body>", extensionScript + "</body>");
  server.send(200, "text/html; charset=utf-8", page);
}

class HeatmapExtensionRegistrar {
public:
  HeatmapExtensionRegistrar() { server.on("/", HTTP_GET, serveHeatmapExtendedIndex); }
};
HeatmapExtensionRegistrar heatmapExtensionRegistrar;
