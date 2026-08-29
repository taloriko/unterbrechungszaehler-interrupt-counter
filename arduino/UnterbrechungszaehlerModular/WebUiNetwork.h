#pragma once

#include <Arduino.h>

// Header- und WLAN-Erweiterung fuer die klassische Oberflaeche.
// Zeitdarstellung, Modulstatus und WLAN-Signal bleiben bewusst als kleine
// additive UI-Schicht getrennt von der eigentlichen WebUi.
static const char WEB_UI_NETWORK[] PROGMEM = R"HTML(
<style>
/* Uhrzeit und KW in einer Zeile, Datum sauber darunter. Der Trenner bekommt
   links und rechts exakt denselben Abstand. */
.headTime{display:grid!important;grid-template-columns:max-content max-content;column-gap:0;row-gap:2px;align-items:baseline;text-align:left!important;line-height:1.2}
.headTime #deviceClock{font-size:.86rem}
.headTime .headWeek{display:inline!important;margin:0!important;font-size:.8rem;color:var(--muted);white-space:nowrap}
.headTime .headWeek::before{content:'|';display:inline-block;margin:0 7px;color:var(--muted)}
.headTime .headDate{display:block!important;grid-column:1/-1;margin:0!important;font-size:.74rem;color:var(--muted)}

/* Zusatzmodule und Verbindungsstatus gemeinsam oben rechts. Alle Status-Icons
   haben exakt denselben Abstand. Nur der Text bekommt etwas Luft zum Icon. */
.headWifi{display:flex!important;align-items:center;justify-content:flex-end;gap:4px;white-space:nowrap}
.headWifi .headModules{display:inline-flex;gap:4px;margin:0}
.headWifi .dot{display:none!important}
.wifiStateIcon{color:var(--muted)}
.wifiStateIcon.available{color:var(--ok);border-color:var(--ok)}
.wifiStateIcon.unavailable{color:var(--danger)}
.wifiStateIcon svg{width:18px;height:18px;display:block;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round}
.headerWifiText{font-size:.8rem;color:var(--muted);margin-left:4px}

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

@media(max-width:720px){
  .headerWifiText{display:none}
  .headWifi{gap:3px}
  .headWifi .headModules{gap:3px}
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
    kv.insertAdjacentHTML('beforeend',
      '<span id="deviceWifiSignalLabel"></span><span id="deviceWifiRssi">-</span>'+ 
      '<span id="deviceWifiRatingLabel"></span><span id="deviceWifiQuality">-</span>');
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
ensureDeviceWifi();
hookStatus();

const language=$('languageSelect');
if(language)language.addEventListener('change',()=>setTimeout(()=>{
  ensureDeviceWifi();
  setupHeaderTime();
},0));
})();
</script>
)HTML";
