#pragma once

#include <Arduino.h>

// RTC-Diagnose und mobile Anpassungen fuer 2.1.2.
static const char WEB_UI_RTC_MOBILE[] PROGMEM = R"HTML(
<style>
html,body{max-width:100%;overflow-x:hidden}
.rtcDiag{margin-top:12px;padding-top:12px;border-top:1px solid var(--line)}
.rtcDiagTitle{font-weight:700;margin-bottom:9px}.rtcBatteryHint{margin-top:9px;font-size:.8rem;color:var(--muted);line-height:1.4}
@media(max-width:640px){
 .wrap{padding:8px;max-width:100%}
 header{grid-template-columns:auto 1fr auto;gap:7px;min-height:48px}
 .headTitle{min-width:0;gap:4px}.headTitle h1{font-size:.9rem;overflow:hidden;text-overflow:ellipsis;max-width:38vw}.headTime,.headWifi{font-size:.72rem}.headDate{font-size:.66rem}.headModules{gap:2px;margin-left:1px}.hwIcon{width:24px;height:24px;border-radius:7px}
 .tabs{flex-wrap:nowrap;overflow-x:auto;overflow-y:hidden;padding:0 2px;margin-top:8px;gap:3px;-webkit-overflow-scrolling:touch;scrollbar-width:thin}
 .tab{flex:0 0 auto;min-width:43px;padding:8px 10px;justify-content:center}.tab .label{display:none}.ico{font-size:1rem}
 .tabpanel{padding:8px;border-radius:0 9px 9px 9px}
 .grid,.infoGrid{gap:8px}.card,.infoBox{padding:11px;border-radius:9px;min-width:0}
 .kv{grid-template-columns:minmax(0,1fr);gap:2px 0}.kv span:nth-child(odd){margin-top:7px;font-size:.78rem}.kv span:nth-child(even){text-align:left;font-size:.92rem;overflow-wrap:anywhere}
 .settingsRow{grid-template-columns:minmax(0,1fr);gap:5px;margin:8px 0}.fieldRow{flex-direction:column;align-items:stretch}.fieldRow input,.fieldRow button{width:100%}
 .actions{display:grid;grid-template-columns:minmax(0,1fr);gap:7px}.actions button,.actions .btn{width:100%;justify-content:center;text-align:center}
 .rangeRow,.v2Range{grid-template-columns:minmax(0,1fr) 68px}
 .tmHierarchy{grid-template-columns:minmax(0,1fr);gap:6px}.tmStep{padding:8px}
 .metrics{grid-template-columns:1fr;gap:5px}.metric{padding-top:7px}.big{padding:14px 6px}.big .num{font-size:3.1rem}.countRow{grid-template-columns:50px minmax(0,1fr) 50px}.countAction{width:46px;height:46px}
 .barrow{grid-template-columns:48px minmax(0,1fr) 30px;gap:5px}
 table.list{display:block;max-width:100%;overflow-x:auto;-webkit-overflow-scrolling:touch}.list th,.list td{white-space:nowrap;padding:7px 6px}
 .heatControls{display:grid;grid-template-columns:1fr 1fr;gap:7px}.heatControl{min-width:0}.heatControl select,.heatControl input{width:100%}
 .footerWrap{grid-template-columns:1fr;gap:6px;text-align:center;margin-top:12px}.footerRight{text-align:center}
 .v2WdWrap{margin-left:-3px;margin-right:-3px}.v2WdTable{font-size:.78rem;min-width:650px}.v2WdTable th,.v2WdTable td{padding:6px}
 .v2TrackTable input{min-width:105px}
}
@media(max-width:390px){.headTitle h1{display:none}.headWifi #conn{display:none}.heatControls{grid-template-columns:1fr}.tabpanel{padding:6px}.card,.infoBox{padding:9px}}
</style>
<script>
(function(){'use strict';
const R={
 de:{title:'RTC-Diagnose',comm:'Kommunikation',valid:'Zeit gueltig',osf:'Oszillator-Stop-Flag',temp:'RTC-Temperatur',last:'Letzter Hardwarecheck',diff:'Abweichung RTC / System',battery:'Batteriestand',batteryText:'Nicht messbar',yes:'Ja',no:'Nein',ok:'OK',error:'Fehler',clear:'nicht gesetzt',set:'gesetzt',now:'gerade eben',ago:'vor {s} s',tempMissing:'-',hint:'Der DS3231 kann den Batteriestand nicht messen. OSF meldet dagegen, ob die RTC ihre sichere Zeitbasis irgendwann verloren hat.'},
 en:{title:'RTC diagnostics',comm:'Communication',valid:'Time valid',osf:'Oscillator stop flag',temp:'RTC temperature',last:'Last hardware check',diff:'RTC / system offset',battery:'Battery level',batteryText:'Not measurable',yes:'Yes',no:'No',ok:'OK',error:'Error',clear:'clear',set:'set',now:'just now',ago:'{s} s ago',tempMissing:'-',hint:'The DS3231 cannot measure battery level. OSF instead reports whether the RTC lost its reliable time base at some point.'},
 swg:{title:'RTC-Diagnose',comm:'Verbindung',valid:'Zeit basst',osf:'Oszillator-Stop-Flag',temp:'RTC-Temperatur',last:'Letschter Hardwarecheck',diff:'RTC / System Unterschied',battery:'Batteriestand',batteryText:'Kann se ned missa',yes:'Jo',no:'Noi',ok:'Basst',error:'Fehler',clear:'ned gsetzt',set:'gsetzt',now:'grad eben',ago:'vor {s} s',tempMissing:'-',hint:'Dr DS3231 kann dr Batteriestand ned missa. S OSF zeigt aber, ob d RTC irgendwann ihre sichere Zeitbasis verlora hot.'}
};
function lang(){let s=document.getElementById('languageSelect'),l=s&&s.value?s.value:(document.documentElement.lang||'de');return R[l]?l:'de'}
function rt(k){return R[lang()][k]||R.de[k]||k}
function parseDetail(text){let o={};String(text||'').split(';').forEach(p=>{let i=p.indexOf('=');if(i>0)o[p.slice(0,i)]=p.slice(i+1)});return o}
function val(id,text){let e=document.getElementById(id);if(e)e.textContent=text}
function ensure(){let card=document.getElementById('rtcCard');if(!card||document.getElementById('rtcDiag'))return;let d=document.createElement('div');d.id='rtcDiag';d.className='rtcDiag';d.innerHTML='<div class="rtcDiagTitle" id="rtcDiagTitle"></div><div class="kv"><span id="rtcCommLabel"></span><span id="rtcComm">-</span><span id="rtcValidLabel"></span><span id="rtcValidDetail">-</span><span id="rtcOsfLabel"></span><span id="rtcOsf">-</span><span id="rtcTempLabel"></span><span id="rtcTemp">-</span><span id="rtcLastLabel"></span><span id="rtcLastCheck">-</span><span id="rtcDiffLabel"></span><span id="rtcDiffDetail">-</span><span id="rtcBatteryLabel"></span><span id="rtcBattery">-</span></div><div class="rtcBatteryHint" id="rtcBatteryHint"></div>';card.appendChild(d);labels()}
function labels(){ensure();val('rtcDiagTitle',rt('title'));val('rtcCommLabel',rt('comm'));val('rtcValidLabel',rt('valid'));val('rtcOsfLabel',rt('osf'));val('rtcTempLabel',rt('temp'));val('rtcLastLabel',rt('last'));val('rtcDiffLabel',rt('diff'));val('rtcBatteryLabel',rt('battery'));val('rtcBattery',rt('batteryText'));val('rtcBatteryHint',rt('hint'))}
function age(ms){ms=Number(ms);if(!Number.isFinite(ms)||ms>4000000000)return '-';let s=Math.max(0,Math.round(ms/1000));return s<2?rt('now'):rt('ago').replace('{s}',s)}
function update(d){ensure();labels();let m=(d.modules||[]).find(x=>x.name==='RTC'),p=parseDetail(m&&m.detail);if(!d.rtcPresent){val('rtcComm','-');val('rtcValidDetail','-');val('rtcOsf','-');val('rtcTemp','-');val('rtcLastCheck','-');val('rtcDiffDetail','-');return}val('rtcComm',p.comm==='ok'?rt('ok'):rt('error'));val('rtcValidDetail',p.valid==='yes'?rt('yes'):rt('no'));val('rtcOsf',p.osf==='set'?rt('set'):rt('clear'));let t=Number(p.temp);val('rtcTemp',Number.isFinite(t)&&t>-100?t.toFixed(2)+' °C':rt('tempMissing'));val('rtcLastCheck',age(p.age!==undefined?p.age:(m?m.ageMs:NaN)));let diff=Number(p.diff);val('rtcDiffDetail',Number.isFinite(diff)?((diff>0?'+':'')+diff+' s'):'-')}
const oldFetch=window.fetch.bind(window);window.fetch=async function(){let r=await oldFetch.apply(null,arguments);try{let u=String(arguments[0]||'');if(u.indexOf('/api/status')>=0&&r.ok)r.clone().json().then(update).catch(()=>{})}catch(e){}return r};
document.addEventListener('change',e=>{if(e.target&&e.target.id==='languageSelect')labels()});
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>{ensure();labels()});else{ensure();labels()}
})();
</script>
)HTML";
