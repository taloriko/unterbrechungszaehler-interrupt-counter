#pragma once

#include <Arduino.h>

// Zentrale Sprachregistrierung der Weboberflaeche.
static const char WEB_UI_I18N[] PROGMEM = R"HTML(
<script>
(function(){'use strict';
const BASE_EXTRA={
  de:{
    'action.add.running':'Unterbrechung wird gespeichert …','action.add.success':'Unterbrechung gespeichert.','action.add.error':'Speichern fehlgeschlagen.',
    'action.delete.running':'Letzte Unterbrechung wird gelöscht …','action.delete.success':'Letzte Unterbrechung gelöscht.','action.delete.error':'Löschen fehlgeschlagen.',
    'action.ntp.running':'NTP-Server wird geprüft …','action.ntp.success':'NTP-Server geprüft und gespeichert.','action.ntp.error':'NTP-Prüfung fehlgeschlagen.',
    'action.rtc.running':'RTC wird synchronisiert …','action.rtc.success':'RTC synchronisiert.','action.rtc.error':'RTC-Synchronisation fehlgeschlagen.',
    'action.display_save.running':'Display-Einstellungen werden gespeichert …','action.display_save.success':'Display-Einstellungen gespeichert.','action.display_save.error':'Display-Einstellungen konnten nicht gespeichert werden.',
    'action.display_test.running':'Display-Test wird gestartet …','action.display_test.success':'Display-Test gestartet.','action.display_test.error':'Display-Test konnte nicht gestartet werden.',
    'action.download.started':'Download gestartet.','action.theme.changed':'Darstellung geändert.'
  },
  en:{
    'action.add.running':'Saving interruption …','action.add.success':'Interruption saved.','action.add.error':'Save failed.',
    'action.delete.running':'Deleting last interruption …','action.delete.success':'Last interruption deleted.','action.delete.error':'Delete failed.',
    'action.ntp.running':'Checking NTP server …','action.ntp.success':'NTP server checked and saved.','action.ntp.error':'NTP check failed.',
    'action.rtc.running':'Synchronizing RTC …','action.rtc.success':'RTC synchronized.','action.rtc.error':'RTC synchronization failed.',
    'action.display_save.running':'Saving display settings …','action.display_save.success':'Display settings saved.','action.display_save.error':'Display settings could not be saved.',
    'action.display_test.running':'Starting display test …','action.display_test.success':'Display test started.','action.display_test.error':'Display test could not be started.',
    'action.download.started':'Download started.','action.theme.changed':'Appearance changed.'
  }
};

const UIC_LANGUAGE_PACKS={
  "swg":{
    "label":"Schwäbisch",
    "locale":"de-DE",
    "strings":{
      "app.title":"Unterbrechungszählerle","connection.connecting":"I verbind mi grad …","connection.wifi":"WLAN isch do","connection.local":"Lokal verbunda","connection.offline":"Grad koi Netz",
      "tab.today":"Heit","tab.history":"Verlauf","tab.heatmap":"Hitzekärtle","tab.details":"Genauer","tab.export":"Raustraga","tab.device":"Kistle","tab.settings":"Eistellunga","tab.autark":"Ohne Netz",
      "today.count":"Unterbrechunga heit","today.avg_gap":"Ø Ruah dazwischa","today.max_gap":"längschte Ruah","today.last":"letschte","today.by_time":"Unterbrechunga nach Uhrzeit","today.latest":"Was zletscht war",
      "common.time":"Zeit","common.gap":"Abstand","common.date":"Datum","common.count":"Azahl","common.status":"Zustand","common.total":"Alles zamma","common.used":"Belegt","common.free":"Frei","common.ok":"Basst","common.error":"Des isch nix","common.not_present":"Isch ned do",
      "history.daily":"Tagesverlauf","history.overview":"Überblick","history.limit":"Damit s Kistle ned ins Schnaufa kommt, zeiget mir höchstens {days} Täg und {limit} rohe Ereignisse. Export und Langzeitspeicher bleibet komplett.",
      "heat.week":"Hitzekärtle – Wochadag / Uhrzeit","heat.monthweek":"Hitzekärtle – Monat / Kalenderwoch","heat.yearmonth":"Hitzekärtle – Johr / Monat","heat.year":"Johr","heat.week_number":"Kalenderwoch","heat.start_hour":"Von Stund","heat.end_hour":"Bis Stund","heat.loading":"I kram grad im Langzeitspeicher …","heat.week_prefix":"KW",
      "details.title":"Tagesdetaille","export.title":"Raustraga & Sichera","export.description":"Kompletter CSV-Export vom normale Ringspeicher. Nix wird verschlampert.","export.download":"CSV raustraga","export.autark_title":"Ohne-Netz-Dada","export.autark_description":"CSV mit Session, Typ, relativer Zeit und Zeitanker.","export.autark_download":"Ohne-Netz-CSV raustraga",
      "device.system":"Kistle","device.firmware":"Firmware","device.uptime":"Läuft scho","device.ram_free":"RAM no frei","device.network":"WLAN","device.ip":"IP-Adress","device.storage":"Speicher","device.recent":"Letschte Ereignisse","device.archive":"Langzeitspeicher","device.autark_store":"Ohne-Netz-Speicher","device.rtc_sync":"RTC mit Systemzeit gradziaga","device.display_test":"Display ausprobiera",
      "settings.language":"Sproch","settings.language_help":"Die Sproch gilt bloß auf dem Gerät hier und wird im Browser gmerkt.","settings.appearance":"Ausseha","settings.theme_system":"Wie s Gerät","settings.theme_help":"Wie s Gerät nimmt automatisch hell oder dunkel. Muss mer nix macha.","settings.rtc_help":"Wenn koi RTC dran isch, bleibt se sichtbar, aber grau. Wenn se do isch, kann se beim Start d Uhrzeit bringa und wird nach NTP wieder gradgricht.",
      "display.brightness":"Helligkeit","display.dim_after":"Dunkler nach Sekunda","display.save":"Display speichera","display.help":"Nach der eingstellte Zeit wird s Display dunkler. Bei ere neue Unterbrechung wird s sofort wieder hell. Ohne Netz geht s nach 15 Sekunda ganz aus.","display.saved":"Display-Eistellunga send gspeichert. Basst.","display.error":"Display-Eistellunga send ned gspeichert worda.",
      "time.title":"Zeitverwaltung","time.ntp_title":"1. NTP","time.ntp_role":"Des gibt dr Takt vor","time.rtc_title":"2. RTC","time.rtc_role":"Start / ohne Netz","time.browser_title":"3. Browser","time.browser_role":"Wenn sonscht nix goht","time.none_title":"4. Ohne Uhrzeit","time.none_role":"bloß relativ","time.active_source":"Wo d Zeit herkommt","time.system_time":"Systemzeit","time.rtc_time":"RTC-Zeit","time.rtc_difference":"Unterschied RTC / System","time.ntp_section":"NTP","time.ntp_save":"Prüfa & speichera","time.ntp_help":"Dr eingtragene NTP-Server wird erscht geprüft und dann dauerhaft gspeichert. Beim Tippen funkt koaner dazwischa.","time.browser_section":"Browser als Notnagel","time.browser_help":"Bloß wenn weder NTP no RTC a gültige Zeit hend, darf dr Browser einmal sei Uhrzeit rüberschieba.","time.difference_title":"Wenn d Uhra andersch laufet","time.difference_text":"NTP isch dr Chef bei dr Zeit. Nach em Abgleich wird d RTC gradgricht; alte gspeicherte Dada werdet ned heimlich verboga.","time.source_ntp":"NTP","time.source_rtc":"RTC","time.source_browser":"Händy / Browser","time.source_none":"Koi absolute Uhrzeit","time.rtc_ok":"Gfunda / Zeit basst","time.rtc_invalid":"Gfunda / Zeit isch krumm","time.diff_seconds":"{value} s","time.ntp_enter":"Trag erscht en NTP-Server ei.","time.ntp_checking":"I guck, ob dr NTP-Server do isch …","time.ntp_saved":"NTP-Server isch gspeichert. Basst.","time.ntp_error":"Dr NTP-Server antwortet ned oder dr Name taugt nix.","time.rtc_sync_ok":"RTC isch wieder gradgricht.","time.rtc_sync_error":"RTC hot sich ned gradziaga lassa.",
      "autark.title":"Ohne-Netz-Modus","autark.session":"Runde","autark.runtime":"Laufzeit","autark.current_pulses":"Impulse","autark.power":"Strom spare","autark.description":"Ohne Netz send WLAN und Webserver aus; CPU 80 MHz und Light-Sleep bleibet aktiv. Des Kistle soll schließlich ned bloß d Powerbank warm macha.","autark.last_entries":"Letschte Ohne-Netz-Einträg","autark.session_type":"Runde / Art","autark.time_reference":"Zeitbezug","autark.type_start":"Akku läuft","autark.type_event":"Impuls","autark.type_end":"Ende / Netz","autark.no_data":"No nix do. Isch au a Ergebnis.","autark.active":"LÄUFT","autark.normal":"Netz/WLAN",
      "footer.made":"Mit Liebe ❤️ gmacht – weil Ruah offenbar Luxus isch","footer.project":"🛠️ Projekt bei GitHub",
      "action.add.running":"Kommt nei …","action.add.success":"Basst, isch gspeichert.","action.add.error":"Des hot ned klappt.",
      "action.delete.running":"Letschtes fliegt wieder raus …","action.delete.success":"Letschtes Ding isch wieder weg.","action.delete.error":"Des wollt ned raus.",
      "action.ntp.running":"I guck, ob dr NTP-Server do isch …","action.ntp.success":"NTP-Server basst und isch gspeichert.","action.ntp.error":"Dr NTP-Server will grad ned.",
      "action.rtc.running":"I richt d RTC grad …","action.rtc.success":"RTC läuft wieder grad.","action.rtc.error":"D RTC hot sich ned gradziaga lassa.",
      "action.display_save.running":"I speicher s Display …","action.display_save.success":"Display-Eistellunga send gspeichert.","action.display_save.error":"Display-Eistellunga send ned gspeichert worda.",
      "action.display_test.running":"I probier s Display aus …","action.display_test.success":"Display-Test läuft.","action.display_test.error":"Display-Test hot ned klappt.",
      "action.download.started":"Download lauft. D Dada send unterwegs.","action.theme.changed":"Ausseha isch umgstellt."
    }
  }
};

const EXACT_TEXT={
  "display.simulation":{"de":"Display-Simulation","en":"Display simulation","swg":"Display bloß zum Gugga"},
  "display.simulation_off":{"de":"Simulation aus","en":"Simulation off","swg":"Simulation aus"},
  "display.simulation_on":{"de":"Simulation an","en":"Simulation on","swg":"Simulation läuft"},
  "display.simulate":{"de":"Simulieren","en":"Simulate","swg":"So tua als ob"},
  "display.preview":{"de":"OLED Live-Vorschau","en":"OLED live preview","swg":"OLED zum Gugga"},
  "display.dimmed":{"de":"Helligkeit gedimmt","en":"Dimmed brightness","swg":"Helligkeit gedimmt"},
  "display.off_after":{"de":"Ganz ausschalten nach","en":"Switch off after","swg":"Ganz aus nach"},
  "display.off_help":{"de":"0 = im Normalbetrieb nie ganz ausschalten; Autark bleibt fest bei 15 s.","en":"0 = never switch off in normal mode; standalone remains fixed at 15 s.","swg":"0 = normal bleibt s an; ohne Netz isch nach 15 s Feierabend."},
  "display.layout":{"de":"Layout","en":"Layout","swg":"Layout"},
  "display.standard":{"de":"Standard","en":"Standard","swg":"Normal"},
  "display.compact":{"de":"Kompakt","en":"Compact","swg":"Kompakt"},
  "display.large_clock":{"de":"Große Uhr","en":"Large clock","swg":"Große Uhr"},
  "display.wake":{"de":"Bei Ereignis aufwecken","en":"Wake on event","swg":"Bei Ereignis wieder aufwecka"},
  "display.invert":{"de":"Anzeige invertieren","en":"Invert display","swg":"Anzeige umdreha"},
  "display.rotate":{"de":"180° drehen","en":"Rotate 180°","swg":"180° dreha"},
  "storage.used":{"de":"Belegt: ","en":"Stored: ","swg":"Drin: "},
  "storage.purpose":{"de":"Zweck:","en":"Purpose:","swg":"Wofür s gut isch:"},
  "wifi.rating":{"de":"Bewertung","en":"Rating","swg":"Wie guat"},
  "wifi.very_good":{"de":"sehr gut","en":"very good","swg":"sauguat"},
  "wifi.good":{"de":"gut","en":"good","swg":"guat"},
  "wifi.fair":{"de":"ausreichend","en":"fair","swg":"goht no"},
  "wifi.weak":{"de":"schwach","en":"weak","swg":"a bissle mau"},
  "wifi.ap":{"de":"Lokaler Zugangspunkt aktiv","en":"Local access point active","swg":"Lokals WLAN isch an"},
  "status.active":{"de":"AKTIV","en":"ACTIVE","swg":"LÄUFT"}
};

const META={de:{label:'Deutsch',locale:'de-DE'},en:{label:'English',locale:'en-US'}};
let applying=false;
function code(){const wanted=localStorage.getItem('uic-lang')||'de';return META[wanted]||UIC_LANGUAGE_PACKS[wanted]?wanted:'de'}
function fmt(text,vars){let out=String(text===undefined?'':text);if(vars)Object.keys(vars).forEach(k=>out=out.replaceAll('{'+k+'}',vars[k]));return out}
function tr(key,vars){const c=code(),pack=UIC_LANGUAGE_PACKS[c];let value=pack&&pack.strings&&pack.strings[key];if(value===undefined)value=(BASE_EXTRA[c]&&BASE_EXTRA[c][key]);if(value===undefined)value=BASE_EXTRA.de[key];return fmt(value===undefined?key:value,vars)}
function locale(){const c=code(),pack=UIC_LANGUAGE_PACKS[c];return pack&&pack.locale?pack.locale:(META[c]&&META[c].locale)||'de-DE'}
function registerLanguage(c,definition){if(!c||!definition)return;UIC_LANGUAGE_PACKS[c]=definition;installOptions();scheduleApply()}
function installOptions(){const select=document.getElementById('languageSelect');if(!select)return;Object.entries(UIC_LANGUAGE_PACKS).forEach(([c,p])=>{let option=select.querySelector('option[value="'+c+'"]');if(!option){option=document.createElement('option');option.value=c;select.appendChild(option)}option.textContent=p.label||c});select.value=code()}
function exactValue(def,c){return def[c]!==undefined?def[c]:(def.de!==undefined?def.de:'')}
function identifyExact(text){for(const [id,def] of Object.entries(EXACT_TEXT)){for(const value of Object.values(def)){if(value===text)return id}}return null}
function applyExact(){const c=code();document.querySelectorAll('body *').forEach(el=>{if(el.children.length)return;const raw=(el.textContent||'').trim();if(!raw)return;let id=el.dataset.uicExact;if(!id||!EXACT_TEXT[id])id=identifyExact(raw);else{const values=Object.values(EXACT_TEXT[id]);if(!values.includes(raw)){const next=identifyExact(raw);if(next)id=next}}if(!id)return;el.dataset.uicExact=id;const value=exactValue(EXACT_TEXT[id],c);if(el.textContent!==value)el.textContent=value})}
function applyHeatmapAxes(){const c=code(),days=c==='en'?['Mon','Tue','Wed','Thu','Fri','Sat','Sun']:['Mo','Di','Mi','Do','Fr','Sa','So'];const months=c==='en'?['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec']:['Jan','Feb','Mär','Apr','Mai','Jun','Jul','Aug','Sep','Okt','Nov','Dez'];const week=document.querySelector('#weekHeat table.heat');if(week){for(let i=1;i<week.rows.length&&i<=7;i++)if(week.rows[i].cells[0])week.rows[i].cells[0].textContent=days[i-1]}document.querySelectorAll('#monthWeekHeat table.heat,#yearMonthHeat table.heat').forEach(table=>{if(table.id===''){}for(let i=1;i<table.rows.length;i++){const cell=table.rows[i].cells[0];if(cell&&i<=12&&!/^\d{4}$/.test(cell.textContent.trim()))cell.textContent=months[i-1]}})}
function apply(){if(applying)return;applying=true;try{installOptions();const c=code(),pack=UIC_LANGUAGE_PACKS[c];document.documentElement.lang=c;if(pack&&pack.strings){document.querySelectorAll('[data-i18n]').forEach(el=>{const value=pack.strings[el.dataset.i18n];if(value!==undefined){const next=fmt(value);if(el.textContent!==next)el.textContent=next}})}applyExact();applyHeatmapAxes()}finally{applying=false}}
let timer=0;function scheduleApply(){clearTimeout(timer);timer=setTimeout(apply,0)}
window.UicI18n={registerLanguage:registerLanguage,t:tr,apply:apply,current:code,locale:locale,languages:UIC_LANGUAGE_PACKS};
window.uicTr=tr;
installOptions();apply();
const select=document.getElementById('languageSelect');if(select)select.addEventListener('change',scheduleApply);
new MutationObserver(scheduleApply).observe(document.body,{childList:true,subtree:true,characterData:true});
})();
</script>
)HTML";
