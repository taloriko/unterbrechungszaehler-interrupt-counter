// HeatmapExtension.ino
// Heatmap UI extension for Unterbrechungszaehler.

static const char* HEATMAP_EXTENSION_VERSION = "2026-08-29-12";

class HeatmapVersionInitializer {
public:
  HeatmapVersionInitializer() { APP_VERSION = HEATMAP_EXTENSION_VERSION; }
};
HeatmapVersionInitializer heatmapVersionInitializer;

static void serveHeatmapExtendedIndex() {
  String page = FPSTR(INDEX_HTML);
  page.reserve(page.length() + 16000);

  const String extraCss = R"CSS(
#device .infoGrid>.infoBox:first-child{display:none}.settingsGrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.settingsCard{border:1px solid var(--line);border-radius:10px;padding:14px}.settingsCard h3{font-size:.95rem;margin:0 0 10px}.heatSettings{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-top:10px}.heatSettings label{display:grid;gap:5px;font-size:.82rem;color:var(--muted)}.heatSettings input{width:100%;font:inherit;padding:8px 9px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)}.heatSettingsActions{display:flex;align-items:center;gap:10px;margin-top:9px;flex-wrap:wrap}.heatSettingsState.ok{color:var(--ok)}.heatSettingsState.err{color:var(--danger)}.yearHeat{min-width:650px}.yearHeat td{min-width:44px}.monthWeekHeat{min-width:430px}.monthWeekHeat td{min-width:58px}@media(max-width:800px){.settingsGrid{grid-template-columns:1fr}}@media(max-width:520px){.heatSettings{grid-template-columns:1fr}}
)CSS";
  page.replace("</style>", extraCss + "</style>");

  const String autarkTab = R"HTML(<button class="tab" data-view="autark"><span class="ico">&#128267;</span><span class="label" data-i18n="tab.autark">Autark</span><span class="beta">BETA</span></button>)HTML";
  const String settingsAndAutarkTab = R"HTML(<button class="tab" data-view="settings"><span class="ico">&#9881;</span><span class="label" data-hi18n="settingsTab">Einstellungen</span></button><button class="tab" data-view="autark"><span class="ico">&#128267;</span><span class="label" data-i18n="tab.autark">Autark</span><span class="beta">BETA</span></button>)HTML";
  page.replace(autarkTab, settingsAndAutarkTab);

  const String oldHeatmap = R"HTML(<section id="heatmap" class="view"><div class="card"><h2 data-i18n="heatmap.title">Heatmap - Wochentag / Uhrzeit</h2><div id="heatWrap" class="heatwrap"></div></div></section>)HTML";
  const String newHeatmap = R"HTML(<section id="heatmap" class="view"><div class="grid"><div class="card span12"><h2 data-i18n="heatmap.title">Heatmap - Wochentag / Uhrzeit</h2><div id="heatWrap" class="heatwrap"></div></div><div class="card span12"><h2 data-hi18n="monthWeekTitle">Heatmap - Monat / Woche</h2><div id="monthWeekHeatWrap" class="heatwrap"></div></div><div class="card span12"><h2 data-hi18n="yearMonthTitle">Heatmap - Jahr / Monat</h2><div id="yearMonthHeatWrap" class="heatwrap"></div></div></div></section>)HTML";
  page.replace(oldHeatmap, newHeatmap);

  const String deviceSection = R"HTML(<section id="device" class="view">)HTML";
  const String settingsView = R"HTML(<section id="settings" class="view"><div class="settingsGrid"><div id="settingsHost"></div><div class="settingsCard"><h3 data-hi18n="heatRangeTitle">Heatmap-Zeitbereich</h3><div class="heatSettings"><label><span data-hi18n="heatStart">Startstunde</span><input id="heatStartHour" type="number" min="0" max="23" step="1" inputmode="numeric"></label><label><span data-hi18n="heatEnd">Endstunde</span><input id="heatEndHour" type="number" min="0" max="23" step="1" inputmode="numeric"></label></div><div class="heatSettingsActions"><button id="heatRangeSaveBtn" class="btn" type="button" data-hi18n="heatSave">Zeitbereich speichern</button><span id="heatRangeState" class="small heatSettingsState"></span></div><p class="small" data-hi18n="heatHelp">Gilt fuer die Wochentag/Uhrzeit-Heatmap. Standard: 05 bis 18 Uhr. Start muss kleiner als Ende sein.</p></div></div></section><section id="device" class="view">)HTML";
  page.replace(deviceSection, settingsView);

  const String extensionScript = R"HTML(
<script>
(function(){'use strict';
var H={
 de:{settingsTab:'Einstellungen',monthWeekTitle:'Heatmap - Monat / Woche',yearMonthTitle:'Heatmap - Jahr / Monat',heatRangeTitle:'Heatmap-Zeitbereich',heatStart:'Startstunde',heatEnd:'Endstunde',heatSave:'Zeitbereich speichern',heatHelp:'Gilt für die Wochentag/Uhrzeit-Heatmap. Standard: 05 bis 18 Uhr. Start muss kleiner als Ende sein.',saved:'Gespeichert: {start}:00 bis {end}:00 Uhr.',invalid:'Ungültiger Zeitbereich. Erlaubt sind 0 bis 23 Uhr und Start muss kleiner als Ende sein.',noData:'Noch keine Daten.'},
 sw:{settingsTab:'Einstellungen',monthWeekTitle:'Heatmap - Monat / Woch',yearMonthTitle:'Heatmap - Johr / Monat',heatRangeTitle:'Heatmap-Zeitbereich',heatStart:'Startstond',heatEnd:'Endstond',heatSave:'Zeitbereich speichra',heatHelp:'Gilt für d Wochadag/Uhrzeit-Heatmap. Standard: 05 bis 18 Uhr. Start muaß kloiner sei als Ende.',saved:'Gspeichert: {start}:00 bis {end}:00 Uhr.',invalid:'Zeitbereich passt net. 0 bis 23 Uhr ond Start muaß kloiner sei als Ende.',noData:'No koi Dada.'},
 en:{settingsTab:'Settings',monthWeekTitle:'Heatmap - month / week',yearMonthTitle:'Heatmap - year / month',heatRangeTitle:'Heatmap time range',heatStart:'Start hour',heatEnd:'End hour',heatSave:'Save time range',heatHelp:'Applies to the weekday/time heatmap. Default: 05 to 18. Start must be earlier than end.',saved:'Saved: {start}:00 to {end}:00.',invalid:'Invalid time range. Use 0 to 23 and make sure start is earlier than end.',noData:'No data yet.'},
 it:{settingsTab:'Impostazioni',monthWeekTitle:'Mappa di calore - mese / settimana',yearMonthTitle:'Mappa di calore - anno / mese',heatRangeTitle:'Intervallo orario mappa di calore',heatStart:'Ora iniziale',heatEnd:'Ora finale',heatSave:'Salva intervallo',heatHelp:'Vale per la mappa giorno/ora. Predefinito: dalle 05 alle 18. L’inizio deve precedere la fine.',saved:'Salvato: dalle {start}:00 alle {end}:00.',invalid:'Intervallo non valido. Usare da 0 a 23 e impostare l’inizio prima della fine.',noData:'Nessun dato.'},
 fr:{settingsTab:'Paramètres',monthWeekTitle:'Carte thermique - mois / semaine',yearMonthTitle:'Carte thermique - année / mois',heatRangeTitle:'Plage horaire de la carte',heatStart:'Heure de début',heatEnd:'Heure de fin',heatSave:'Enregistrer la plage',heatHelp:'S’applique à la carte jour/heure. Par défaut : 05 à 18. Le début doit être antérieur à la fin.',saved:'Enregistré : {start}:00 à {end}:00.',invalid:'Plage horaire invalide. Utilisez 0 à 23 et un début antérieur à la fin.',noData:'Aucune donnée.'}
};
function q(id){return document.getElementById(id)}function pad(n){return String(n).padStart(2,'0')}function lang(){var s=q('languageSelect');return s&&H[s.value]?s.value:'de'}function tx(k,v){var x=(H[lang()]||H.de)[k]||H.de[k]||k;if(v)Object.keys(v).forEach(function(a){x=x.replaceAll('{'+a+'}',v[a])});return x}
function range(){var s=parseInt(localStorage.getItem('uic-heat-start'),10),e=parseInt(localStorage.getItem('uic-heat-end'),10);if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){s=5;e=18}return{s:s,e:e}}
function applyLabels(){document.querySelectorAll('[data-hi18n]').forEach(function(n){n.textContent=tx(n.dataset.hi18n)});var r=range();if(q('heatStartHour'))q('heatStartHour').value=r.s;if(q('heatEndHour'))q('heatEndHour').value=r.e}
function moveSettings(){var host=q('settingsHost'),device=q('device');if(!host||!device)return;var card=device.querySelector('.infoGrid>.infoBox:first-child');if(card&&!host.contains(card)){host.appendChild(card);card.style.display='block'}}
function renderWeek(ev){var wrap=q('heatWrap');if(!wrap)return;var r=range(),names=['So','Mo','Di','Mi','Do','Fr','Sa'],a=[],max=1;for(var d=0;d<7;d++)a[d]=new Array(24).fill(0);ev.forEach(function(ts){var x=new Date(ts*1000),h=x.getHours();if(h<r.s||h>r.e)return;a[x.getDay()][h]++;max=Math.max(max,a[x.getDay()][h])});var html='<table class="heat"><thead><tr><th></th>';for(var h=r.s;h<=r.e;h++)html+='<th>'+pad(h)+'</th>';html+='</tr></thead><tbody>';[1,2,3,4,5,6,0].forEach(function(day){html+='<tr><th>'+names[day]+'</th>';for(var hour=r.s;hour<=r.e;hour++){var v=a[day][hour],alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')">'+(v||'')+'</td>'}html+='</tr>'});html+='</tbody></table>';wrap.innerHTML=html}
function renderMonthWeek(ev){var wrap=q('monthWeekHeatWrap');if(!wrap)return;var rows={},max=1;ev.forEach(function(ts){var d=new Date(ts*1000),key=d.getFullYear()+'-'+pad(d.getMonth()+1),week=Math.floor((d.getDate()-1)/7);if(!rows[key])rows[key]=new Array(5).fill(0);rows[key][week]++;max=Math.max(max,rows[key][week])});var keys=Object.keys(rows).sort().reverse();if(!keys.length){wrap.innerHTML='<span class="muted">'+tx('noData')+'</span>';return}var html='<table class="heat monthWeekHeat"><thead><tr><th></th>';for(var w=1;w<=5;w++)html+='<th>W'+w+'</th>';html+='</tr></thead><tbody>';keys.forEach(function(k){var p=k.split('-'),label=p[1]+'.'+p[0];html+='<tr><th>'+label+'</th>';for(var w=0;w<5;w++){var v=rows[k][w],alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')" title="'+label+' / W'+(w+1)+': '+v+'">'+(v||'')+'</td>'}html+='</tr>'});html+='</tbody></table>';wrap.innerHTML=html}
function renderYearMonth(ev){var wrap=q('yearMonthHeatWrap');if(!wrap)return;var years={},max=1;ev.forEach(function(ts){var d=new Date(ts*1000),y=d.getFullYear(),m=d.getMonth();if(!years[y])years[y]=new Array(12).fill(0);years[y][m]++;max=Math.max(max,years[y][m])});var ys=Object.keys(years).map(Number).sort(function(a,b){return b-a});if(!ys.length){wrap.innerHTML='<span class="muted">'+tx('noData')+'</span>';return}var months=['Jan','Feb','Mär','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'],html='<table class="heat yearHeat"><thead><tr><th></th>';months.forEach(function(m){html+='<th>'+m+'</th>'});html+='</tr></thead><tbody>';ys.forEach(function(y){html+='<tr><th>'+y+'</th>';for(var m=0;m<12;m++){var v=years[y][m],alpha=v?0.15+0.75*(v/max):0;html+='<td style="background:rgba(33,102,209,'+alpha.toFixed(2)+')" title="'+y+'-'+pad(m+1)+': '+v+'">'+(v||'')+'</td>'}html+='</tr>'});html+='</tbody></table>';wrap.innerHTML=html}
async function refreshHeatmaps(){try{var r=await fetch('/api/events?heat='+Date.now(),{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);var d=await r.json(),ev=(d.events||[]).sort(function(a,b){return a-b});renderWeek(ev);renderMonthWeek(ev);renderYearMonth(ev)}catch(e){var msg='<span class="muted">'+e.message+'</span>';if(q('monthWeekHeatWrap'))q('monthWeekHeatWrap').innerHTML=msg;if(q('yearMonthHeatWrap'))q('yearMonthHeatWrap').innerHTML=msg}}
function saveRange(){var s=parseInt(q('heatStartHour').value,10),e=parseInt(q('heatEndHour').value,10),out=q('heatRangeState');if(!Number.isInteger(s)||!Number.isInteger(e)||s<0||s>23||e<0||e>23||s>=e){out.className='small heatSettingsState err';out.textContent=tx('invalid');return}localStorage.setItem('uic-heat-start',String(s));localStorage.setItem('uic-heat-end',String(e));out.className='small heatSettingsState ok';out.textContent=tx('saved',{start:pad(s),end:pad(e)});refreshHeatmaps()}
moveSettings();applyLabels();var save=q('heatRangeSaveBtn');if(save)save.addEventListener('click',saveRange);var ls=q('languageSelect');if(ls)ls.addEventListener('change',function(){setTimeout(function(){applyLabels();refreshHeatmaps()},0)});document.querySelectorAll('.tab[data-view="heatmap"],.tab[data-view="settings"]').forEach(function(b){b.addEventListener('click',function(){setTimeout(function(){moveSettings();applyLabels();if(b.dataset.view==='heatmap')refreshHeatmaps()},0)})});refreshHeatmaps();setInterval(function(){var v=q('heatmap');if(v&&v.classList.contains('active'))refreshHeatmaps()},3000);
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
