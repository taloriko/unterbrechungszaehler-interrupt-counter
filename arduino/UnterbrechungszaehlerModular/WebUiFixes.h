#pragma once

#include <Arduino.h>

// UI-Korrekturen fuer Darstellung, Bedienrueckmeldung und zusaetzliche Sprache.
static const char WEB_UI_FIXES[] PROGMEM = R"HTML(
<style>
.stableStorageNumber{position:relative;color:transparent!important;min-width:130px;white-space:nowrap}
.stableStorageNumber::after{content:attr(data-formatted);position:absolute;right:0;top:0;color:var(--text);font-weight:600;white-space:nowrap}
.countAction svg{width:25px;height:25px;display:block;fill:none;stroke:currentColor;stroke-width:1.9;stroke-linecap:round;stroke-linejoin:round;pointer-events:none}
.countAction.add svg{width:27px;height:27px}

/* Alle ausloesenden Bedienelemente verwenden dieselbe optische Rueckmeldung. */
button,.btn{transition:transform .08s ease,background .15s ease,border-color .15s ease,color .15s ease,box-shadow .15s ease}
button:active,.btn:active{transform:translateY(1px) scale(.98)}
button.actionRunning,.btn.actionRunning{background:var(--accent)!important;border-color:var(--accent)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(33,102,209,.16)!important}
button.actionSuccess,.btn.actionSuccess{background:var(--ok)!important;border-color:var(--ok)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(22,128,58,.15)!important}
button.actionError,.btn.actionError{background:var(--danger)!important;border-color:var(--danger)!important;color:#fff!important;box-shadow:0 0 0 3px rgba(180,35,24,.14)!important}
.actions{display:flex!important;align-items:center!important;gap:8px!important;flex-wrap:wrap!important}
.actionMessage{display:block;min-height:1.1em;margin-top:7px;font-size:.8rem;color:var(--muted);text-align:left}
.actionMessage.ok{color:var(--ok)}.actionMessage.bad{color:var(--danger)}
#countFeedback{text-align:center;margin-top:8px}
#themeFeedback{margin-top:8px}

/* Stundenbalken bleiben auch bei Nullwerten als geschlossene Zeitachse lesbar. */
#hourBars .barrow.zeroHour .bar{width:0!important}
</style>
<script>
(function(){'use strict';
const $=id=>document.getElementById(id);
const swActive=()=>localStorage.getItem('uic-lang')==='swg'||document.documentElement.lang==='swg';
const locale=()=>document.documentElement.lang==='en'?'en-US':'de-DE';
const formatNumber=n=>new Intl.NumberFormat(locale()).format(Number(n||0));

/* Mittelschwäbische, bewusst gut lesbare und leicht humorvolle Oberflaechentexte. */
const SW={
'app.title':'Unterbrechungszählerle','connection.connecting':'I verbind mi grad …','connection.wifi':'WLAN isch do','connection.local':'Lokal verbunda','connection.offline':'Grad koi Netz','tab.today':'Heit','tab.history':'Verlauf','tab.heatmap':'Hitzekärtle','tab.details':'Genauer','tab.export':'Raustraga','tab.device':'Kistle','tab.settings':'Eistellunga','tab.autark':'Ohne Netz','today.count':'Unterbrechunga heit','today.avg_gap':'Ø Ruah dazwischa','today.max_gap':'längschte Ruah','today.last':'letschte','today.by_time':'Unterbrechunga nach Uhrzeit','today.latest':'Was zletscht war','common.time':'Zeit','common.gap':'Abstand','common.date':'Datum','common.count':'Azahl','common.status':'Zustand','common.total':'Alles zamma','common.used':'Belegt','common.free':'Frei','common.ok':'Basst','common.error':'Des isch nix','common.not_present':'Isch ned do','history.daily':'Tagesverlauf','history.overview':'Überblick','history.limit':'Damit s Kistle ned ins Schnaufa kommt, zeiget mir höchstens {days} Täg und {limit} rohe Ereignisse. Export und Langzeitspeicher bleibet komplett.','heat.week':'Hitzekärtle – Wochadag / Uhrzeit','heat.monthweek':'Hitzekärtle – Monat / Kalenderwoch','heat.yearmonth':'Hitzekärtle – Johr / Monat','heat.year':'Johr','heat.week_number':'Kalenderwoch','heat.start_hour':'Von Stund','heat.end_hour':'Bis Stund','heat.loading':'I kram grad im Langzeitspeicher …','heat.week_prefix':'KW','details.title':'Tagesdetaille','export.title':'Raustraga & Sichera','export.description':'Kompletter CSV-Export vom normale Ringspeicher. Nix wird verschlampert.','export.download':'CSV raustraga','export.autark_title':'Ohne-Netz-Dada','export.autark_description':'CSV mit Session, Typ, relativer Zeit und Zeitanker.','export.autark_download':'Ohne-Netz-CSV raustraga','device.system':'Kistle','device.firmware':'Firmware','device.uptime':'Läuft scho','device.ram_free':'RAM no frei','device.network':'WLAN','device.ip':'IP-Adress','device.storage':'Speicher','device.recent':'Letschte Ereignisse','device.archive':'Langzeitspeicher','device.autark_store':'Ohne-Netz-Speicher','device.rtc_sync':'RTC mit Systemzeit gradziaga','device.display_test':'Display ausprobiera','settings.language':'Sproch','settings.language_help':'Die Sproch gilt bloß auf dem Gerät hier und wird im Browser gmerkt.','settings.appearance':'Ausseha','settings.theme_system':'Wie s Gerät','settings.theme_help':'Wie s Gerät nimmt automatisch hell oder dunkel. Muss mer nix macha.','settings.rtc_help':'Wenn koi RTC dran isch, bleibt se sichtbar, aber grau. Wenn se do isch, kann se beim Start d Uhrzeit bringa und wird nach NTP wieder gradgricht.','display.brightness':'Helligkeit','display.dim_after':'Dunkler nach Sekunda','display.save':'Display speichera','display.help':'Nach der eingstellte Zeit wird s Display dunkler. Bei ere neue Unterbrechung wird s sofort wieder hell. Ohne Netz geht s nach 15 Sekunda ganz aus.','display.saved':'Display-Eistellunga send gspeichert. Basst.','display.error':'Display-Eistellunga send ned gspeichert worda.','time.title':'Zeitverwaltung','time.ntp_title':'1. NTP','time.ntp_role':'Des gibt dr Takt vor','time.rtc_title':'2. RTC','time.rtc_role':'Start / ohne Netz','time.browser_title':'3. Browser','time.browser_role':'Wenn sonscht nix goht','time.none_title':'4. Ohne Uhrzeit','time.none_role':'bloß relativ','time.active_source':'Wo d Zeit herkommt','time.system_time':'Systemzeit','time.rtc_time':'RTC-Zeit','time.rtc_difference':'Unterschied RTC / System','time.ntp_section':'NTP','time.ntp_save':'Prüfa & speichera','time.ntp_help':'Dr eingtragene NTP-Server wird erscht geprüft und dann dauerhaft gspeichert. Beim Tippen funkt koaner dazwischa.','time.browser_section':'Browser als Notnagel','time.browser_help':'Bloß wenn weder NTP no RTC a gültige Zeit hend, darf dr Browser einmal sei Uhrzeit rüberschieba.','time.difference_title':'Wenn d Uhra andersch laufet','time.difference_text':'NTP isch dr Chef bei dr Zeit. Nach em Abgleich wird d RTC gradgricht; alte gspeicherte Dada werdet ned heimlich verboga.','time.source_ntp':'NTP','time.source_rtc':'RTC','time.source_browser':'Händy / Browser','time.source_none':'Koi absolute Uhrzeit','time.rtc_ok':'Gfunda / Zeit basst','time.rtc_invalid':'Gfunda / Zeit isch krumm','time.diff_seconds':'{value} s','time.ntp_enter':'Trag erscht en NTP-Server ei.','time.ntp_checking':'I guck, ob dr NTP-Server do isch …','time.ntp_saved':'NTP-Server isch gspeichert. Basst.','time.ntp_error':'Dr NTP-Server antwortet ned oder dr Name taugt nix.','time.rtc_sync_ok':'RTC isch wieder gradgricht.','time.rtc_sync_error':'RTC hot sich ned gradziaga lassa.','autark.title':'Ohne-Netz-Modus','autark.session':'Runde','autark.runtime':'Laufzeit','autark.current_pulses':'Impulse','autark.power':'Strom spare','autark.description':'Ohne Netz send WLAN und Webserver aus; CPU 80 MHz und Light-Sleep bleibet aktiv. Des Kistle soll schließlich ned bloß d Powerbank warm macha.','autark.last_entries':'Letschte Ohne-Netz-Einträg','autark.session_type':'Runde / Art','autark.time_reference':'Zeitbezug','autark.type_start':'Akku läuft','autark.type_event':'Impuls','autark.type_end':'Ende / Netz','autark.no_data':'No nix do. Isch au a Ergebnis.','autark.active':'LÄUFT','autark.normal':'Netz/WLAN','footer.made':'Mit Liebe ❤️ gmacht – weil Ruah offenbar Luxus isch','footer.project':'🛠️ Projekt bei GitHub'
};

const EXTRA_SW={
'Deutsch':'Deutsch','English':'English','Schwäbisch':'Schwäbisch','System':'Wie s Gerät','Standard':'Normal','Kompakt':'Kompakt','Große Uhr':'Große Uhr','Tagesübersicht':'Tagesüberblick','Display-Simulation':'Display bloß zum Gugga','Simulation aus':'Simulation aus','Simulation an':'Simulation läuft','Simulieren':'So tua als ob','OLED Live-Vorschau':'OLED zum Gugga','Display speichern':'Display speichera','Display testen':'Display ausprobiera','Speichern fehlgeschlagen':'Speichera hot ned klappt','Gespeichert ✓':'Gspeichert ✓','Speichert …':'I speicher …','Test läuft …':'I probier s aus …','Test gestartet ✓':'Test läuft ✓','WLAN verbunden':'WLAN isch do','Lokal verbunden':'Lokal verbunda','Offline':'Grad koi Netz','Nicht vorhanden':'Isch ned do','Erkannt / Zeit OK':'Gfunda / Zeit basst','Erkannt / Zeit ungültig':'Gfunda / Zeit isch krumm','Keine absolute Zeit':'Koi absolute Uhrzeit','Handy / Browser':'Händy / Browser','AKTIV':'LÄUFT','Belegt: ':'Drin: ','Zweck:':'Wofür s gut isch:'
};

function makeStorageNumberStable(id){const el=$(id);if(!el)return;el.classList.add('stableStorageNumber');if(!el.dataset.formatted)el.dataset.formatted=el.textContent||'-'}
function updateStableStorage(d){if(!d)return;[['devRecent',d.eventCount,d.ringCapacity],['devArchive',d.archiveCount,d.archiveCapacity],['devAutark',d.autarkCount,d.autarkCapacity]].forEach(v=>{const el=$(v[0]);if(!el)return;makeStorageNumberStable(v[0]);el.dataset.formatted=formatNumber(v[1])+' / '+formatNumber(v[2])})}

function installIcons(){
  const add=$('addBtn'),del=$('undoBtn');
  if(add){add.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 11V6.5a1.5 1.5 0 0 1 3 0V10"/><path d="M12 10V5.5a1.5 1.5 0 0 1 3 0V10"/><path d="M15 10V7a1.5 1.5 0 0 1 3 0v6.5"/><path d="M9 11 7.8 9.8a1.55 1.55 0 0 0-2.2 2.2l3.7 5.2A5 5 0 0 0 13.4 19H15a5 5 0 0 0 5-5v-2"/></svg>';add.title=swActive()?'Unterbrechung nei damit':document.documentElement.lang==='en'?'Add interruption':'Unterbrechung hinzufügen';add.setAttribute('aria-label',add.title)}
  if(del){del.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18"/><path d="M8 6V4h8v2"/><path d="M19 6l-1 14H6L5 6"/><path d="M10 10v6M14 10v6"/></svg>';del.title=swActive()?'Letschtes wieder naus':document.documentElement.lang==='en'?'Delete last interruption':'Letzte Unterbrechung löschen';del.setAttribute('aria-label',del.title)}
}

function addLanguage(){const s=$('languageSelect');if(!s||s.querySelector('option[value="swg"]'))return;const o=document.createElement('option');o.value='swg';o.textContent='Schwäbisch';s.appendChild(o);if(localStorage.getItem('uic-lang')==='swg')s.value='swg'}

function translateStaticSw(){
  if(!swActive())return;
  document.documentElement.lang='swg';
  document.querySelectorAll('[data-i18n]').forEach(el=>{const v=SW[el.dataset.i18n];if(v!==undefined&&el.textContent!==v)el.textContent=v});
  document.querySelectorAll('option').forEach(o=>{const v=EXTRA_SW[o.textContent.trim()];if(v&&o.textContent!==v)o.textContent=v});
  installIcons();
  translateDynamicSw();
}

function replaceExact(id,map){const el=$(id);if(!el)return;const now=el.textContent.trim(),v=map[now];if(v&&el.textContent!==v)el.textContent=v}
function translateDynamicSw(){
  if(!swActive())return;
  const states={'WLAN verbunden':'WLAN isch do','Lokal verbunden':'Lokal verbunda','Offline':'Grad koi Netz','Nicht vorhanden':'Isch ned do','Erkannt / Zeit OK':'Gfunda / Zeit basst','Erkannt / Zeit ungültig':'Gfunda / Zeit isch krumm','Handy / Browser':'Händy / Browser','Keine absolute Zeit':'Koi absolute Uhrzeit','AKTIV':'LÄUFT'};
  ['conn','devWifi','settingsRtcState','tmSource','autarkState'].forEach(id=>replaceExact(id,states));
  const sim=$('displaySimulationState');if(sim){const t=sim.textContent.trim();if(t==='Simulation aus')sim.textContent='Simulation aus';else if(t==='Simulation an')sim.textContent='Simulation läuft'}
  const meta=[['recentExportMeta','Belegt: ','Drin: '],['archiveExportMeta','Belegt: ','Drin: '],['autarkExportMeta','Belegt: ','Drin: ']];meta.forEach(x=>{const e=$(x[0]);if(e&&e.textContent.startsWith(x[1]))e.textContent=x[2]+e.textContent.slice(x[1].length)});
  const heat=$('weekHeat');if(heat){const labels=['Mo','Di','Mi','Do','Fr','Sa','So'];heat.querySelectorAll('table.heat tr').forEach((r,i)=>{if(i>0&&i<=7&&r.cells[0]&&r.cells[0].textContent!==labels[i-1])r.cells[0].textContent=labels[i-1]})}
}

function fillHourGaps(){
  const host=$('hourBars');if(!host)return;
  const rows=[...host.querySelectorAll('.barrow')];if(rows.length<2)return;
  const hours=rows.map(r=>parseInt((r.children[0]&&r.children[0].textContent)||'',10)).filter(Number.isFinite);if(hours.length<2)return;
  const first=Math.min(...hours),last=Math.max(...hours),byHour=new Map();rows.forEach(r=>{const h=parseInt((r.children[0]&&r.children[0].textContent)||'',10);if(Number.isFinite(h))byHour.set(h,r)});
  let changed=false,frag=document.createDocumentFragment();
  for(let h=first;h<=last;h++){
    if(byHour.has(h)){frag.appendChild(byHour.get(h));continue}
    const r=document.createElement('div');r.className='barrow zeroHour';r.innerHTML='<span>'+String(h).padStart(2,'0')+'-'+String((h+1)%24).padStart(2,'0')+'</span><div class="barbg"><div class="bar" style="width:0%"></div></div><b>0</b>';frag.appendChild(r);changed=true;
  }
  if(changed)host.replaceChildren(frag)
}

function ensureMessages(){
  const count=$('todayCount')&&$('todayCount').closest('.big');if(count&&!$('countFeedback')){const m=document.createElement('div');m.id='countFeedback';m.className='actionMessage';count.querySelector('.countRow').insertAdjacentElement('afterend',m)}
  const theme=document.querySelector('.themeSwitch');if(theme&&!$('themeFeedback')){const m=document.createElement('div');m.id='themeFeedback';m.className='actionMessage';theme.insertAdjacentElement('afterend',m)}
}
function msg(id,text,kind){const e=$(id);if(!e)return;e.textContent=text;e.className='actionMessage '+(kind||'')}
function pulse(el,kind,ms){if(!el)return;el.classList.remove('actionRunning','actionSuccess','actionError');if(kind)el.classList.add(kind);setTimeout(()=>el&&el.classList.remove(kind),ms||900)}
function langText(de,en,sw){return swActive()?sw:(document.documentElement.lang==='en'?en:de)}

function setupActionFeedback(){
  if(document.documentElement.dataset.actionFeedbackReady)return;document.documentElement.dataset.actionFeedbackReady='1';
  document.addEventListener('click',e=>{
    const b=e.target.closest('button,.btn');if(!b)return;
    if(b.classList.contains('tab')){pulse(b,'actionSuccess',250);return}
    if(b.matches('[data-theme-choice]')){const n=b.dataset.themeChoice==='light'?langText('Hell','Light','Hell wie dr Dag'):b.dataset.themeChoice==='dark'?langText('Dunkel','Dark','Dunkel wie dr Keller'):langText('System','System','Wie s Gerät');msg('themeFeedback',langText('Darstellung: ','Appearance: ','Ausseha: ')+n,'ok');pulse(b,'actionSuccess');return}
    if(b.id==='addBtn'){msg('countFeedback',langText('Unterbrechung wird gespeichert …','Saving interruption …','Kommt nei …'),'');pulse(b,'actionRunning',600)}
    if(b.id==='undoBtn'){msg('countFeedback',langText('Letzte Unterbrechung wird gelöscht …','Deleting last interruption …','Letschtes fliegt wieder raus …'),'');pulse(b,'actionRunning',600)}
    if(b.matches('a.btn')){pulse(b,'actionSuccess');}
  },true)
}

function hookFetch(){
  const original=window.fetch.bind(window);window.fetch=async function(){const args=arguments,url=String(args[0]||'');let response;try{response=await original.apply(null,args)}catch(err){handleActionResult(url,false);throw err}try{if(url.indexOf('/api/status')>=0&&response.ok)response.clone().json().then(updateStableStorage).catch(()=>{});handleActionResult(url,response.ok)}catch(e){}return response}
}
function handleActionResult(url,ok){
  if(url.indexOf('/api/add')>=0){msg('countFeedback',ok?langText('Unterbrechung gespeichert.','Interruption saved.','Basst, isch gspeichert.') : langText('Speichern fehlgeschlagen.','Save failed.','Des hot ned klappt.'),ok?'ok':'bad');pulse($('addBtn'),ok?'actionSuccess':'actionError')}
  else if(url.indexOf('/api/delete-last')>=0){msg('countFeedback',ok?langText('Letzte Unterbrechung gelöscht.','Last interruption deleted.','Letschtes Ding isch wieder weg.') : langText('Löschen fehlgeschlagen.','Delete failed.','Des wollt ned raus.'),ok?'ok':'bad');pulse($('undoBtn'),ok?'actionSuccess':'actionError')}
}

let scheduled=false;function scheduleFix(){if(scheduled)return;scheduled=true;setTimeout(()=>{scheduled=false;fillHourGaps();if(swActive()){translateStaticSw();translateDynamicSw()}},0)}

['devRecent','devArchive','devAutark'].forEach(makeStorageNumberStable);addLanguage();installIcons();ensureMessages();fillHourGaps();setupActionFeedback();hookFetch();
if(swActive())translateStaticSw();
const language=$('languageSelect');if(language)language.addEventListener('change',()=>setTimeout(()=>{addLanguage();installIcons();ensureMessages();if(swActive())translateStaticSw()},0));
new MutationObserver(scheduleFix).observe(document.body,{childList:true,subtree:true,characterData:true});
})();
</script>
)HTML";
