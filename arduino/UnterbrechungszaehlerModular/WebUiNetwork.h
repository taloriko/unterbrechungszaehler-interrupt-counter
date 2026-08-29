#pragma once

#include <Arduino.h>

// Header- und WLAN-Erweiterung fuer die klassische Oberflaeche.
// Zeitdarstellung, Modulstatus und WLAN-Signal bleiben bewusst als kleine
// additive UI-Schicht getrennt von der eigentlichen WebUi.
static const char WEB_UI_NETWORK[] PROGMEM = R"HTML(
<style>
/* Uhrzeit und KW in einer Zeile, Datum sauber darunter. */
.headTime{display:grid!important;grid-template-columns:max-content max-content;column-gap:7px;row-gap:2px;align-items:baseline;text-align:left!important;line-height:1.2}
.headTime #deviceClock{font-size:.86rem}
.headTime .headWeek{display:inline!important;margin:0!important;font-size:.8rem;color:var(--muted);white-space:nowrap}
.headTime .headDate{display:block!important;grid-column:1/-1;margin:0!important;font-size:.74rem;color:var(--muted)}

/* Zusatzmodule und Verbindungsstatus gemeinsam oben rechts. */
.headWifi{display:flex!important;align-items:center;justify-content:flex-end;gap:5px;white-space:nowrap}
.headWifi .headModules{display:inline-flex;gap:4px;margin:0 3px 0 0}
.headWifi .dot{display:none!important}
.wifiStateIcon{color:var(--muted)}
.wifiStateIcon.available{color:var(--ok);border-color:var(--ok)}
.wifiStateIcon.unavailable{color:var(--danger)}
.wifiStateIcon svg{width:18px;height:18px;display:block;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round}
.headerWifiText{font-size:.8rem;color:var(--muted);margin-left:2px}

/* WLAN-Signalstaerke in den Einstellungen. */
.wifiSignalWrap{margin-top:12px}
.wifiSignalHead{display:flex;justify-content:space-between;gap:12px;align-items:center;font-size:.82rem;color:var(--muted);margin-bottom:5px}
.wifiSignalHead b{color:var(--text);font-weight:600}
.wifiSignalBar{height:12px;background:var(--line);border-radius:7px;overflow:hidden}
.wifiSignalBar>span{display:block;height:100%;width:0;transition:width .25s ease,background .25s ease}
.wifiSignalBar>span.good{background:var(--ok)}
.wifiSignalBar>span.warn{background:#d49a00}
.wifiSignalBar>span.bad{background:var(--danger)}
.wifiSignalBar>span.none{background:var(--line)}

@media(max-width:720px){
  .headerWifiText{display:none}
  .headWifi{gap:3px}
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
  week.textContent=number===null?'':'| '+text('KW ','CW ')+number;
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

function addWifiSettings(){
  if($('settingsWifiCard'))return;
  const grid=document.querySelector('#settings .infoGrid');
  if(!grid)return;

  const card=document.createElement('div');
  card.id='settingsWifiCard';
  card.className='infoBox';
  card.innerHTML='<h3><span class="moduleIcon">📶</span> WLAN</h3>'+
    '<div class="kv">'+
      '<span>'+text('Status','Status')+'</span><span id="settingsWifiState">-</span>'+
      '<span>'+text('IP-Adresse','IP address')+'</span><span id="settingsWifiIp">-</span>'+
      '<span>'+text('Signal','Signal')+'</span><span id="settingsWifiRssi">-</span>'+
    '</div>'+
    '<div class="wifiSignalWrap">'+
      '<div class="wifiSignalHead"><span>'+text('Signalstärke','Signal strength')+'</span><b id="settingsWifiQuality">-</b></div>'+
      '<div class="wifiSignalBar"><span id="settingsWifiBar" class="none"></span></div>'+
    '</div>'+
    '<p class="infoHelp">'+text('Grün ab −60 dBm, gelb bis −75 dBm, darunter rot. Je näher der Wert an 0 dBm liegt, desto stärker ist das WLAN-Signal.','Green from −60 dBm, yellow down to −75 dBm, below that red. The closer the value is to 0 dBm, the stronger the Wi-Fi signal.')+'</p>';

  const rtc=$('rtcCard');
  if(rtc)grid.insertBefore(card,rtc);else grid.appendChild(card);
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
  addWifiSettings();

  const local=!d.wifi&&d.ip&&d.ip!=='-';
  const connected=!!d.wifi;
  const icon=$('headWifiIcon');
  if(icon){
    icon.className='hwIcon wifiStateIcon '+((connected||local)?'available':'unavailable');
    icon.title=connected?(text('WLAN verbunden','Wi-Fi connected')+(d.rssi?' · '+d.rssi+' dBm':'')):(local?text('Lokaler Zugangspunkt aktiv','Local access point active'):text('Offline','Offline'));
  }

  const state=$('settingsWifiState'),ip=$('settingsWifiIp'),rssi=$('settingsWifiRssi'),quality=$('settingsWifiQuality'),bar=$('settingsWifiBar');
  if(state)state.textContent=connected?text('WLAN verbunden','Wi-Fi connected'):(local?text('Lokaler Zugangspunkt','Local access point'):text('Offline','Offline'));
  if(ip)ip.textContent=d.ip||'-';

  const dbm=Number(d.rssi||0);
  if(!connected||!dbm){
    if(rssi)rssi.textContent='-';
    if(quality)quality.textContent='-';
    if(bar){bar.style.width='0%';bar.className='none'}
    return;
  }

  if(rssi)rssi.textContent=dbm+' dBm';
  if(quality)quality.textContent=signalLabel(dbm);
  if(bar){
    bar.style.width=qualityPercent(dbm)+'%';
    bar.className=dbm>=-60?'good':(dbm>=-75?'warn':'bad');
  }
}

function hookStatus(){
  const original=window.fetch.bind(window);
  window.fetch=async function(){
    const response=await original.apply(null,arguments);
    try{
      const url=String(arguments[0]||'');
      if(url.indexOf('/api/status')>=0&&response.ok){
        response.clone().json().then(updateWifi).catch(()=>{});
      }
    }catch(e){}
    return response;
  };
}

setupHeaderTime();
setupHeaderStatus();
addWifiSettings();
hookStatus();

// Beim Sprachwechsel werden die dynamisch erzeugten Texte neu aufgebaut.
const language=$('languageSelect');
if(language)language.addEventListener('change',()=>{
  setTimeout(()=>{
    const card=$('settingsWifiCard');
    if(card)card.remove();
    addWifiSettings();
    setupHeaderTime();
  },0);
});
})();
</script>
)HTML";
