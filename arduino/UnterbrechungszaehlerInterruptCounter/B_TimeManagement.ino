// B_TimeManagement.ino
// Time hierarchy/status UI. Loaded after A_HardwareExtension.ino so RTC
// variables and helper functions are already declared for the Arduino build.

static int32_t tmCurrentRtcDifferenceSec() {
  if (!hwRtcDetected || !hwRtcTimeValid || !timeIsValid()) return 0;
  struct tm rtcTm;
  bool osf = false;
  if (!hwRtcRead(rtcTm, osf) || osf) return 0;
  setenv("TZ", TZ_INFO, 1);
  tzset();
  time_t rtcEpoch = mktime(&rtcTm);
  if (rtcEpoch <= 1700000000) return 0;
  return (int32_t)((int64_t)rtcEpoch - (int64_t)time(nullptr));
}

static String tmTimeJson() {
  hwRtcUpdateText();
  int32_t diff = tmCurrentRtcDifferenceSec();
  String json;
  json.reserve(760);
  json = "{\"ok\":true";
  json += ",\"source\":\"" + timeSource + "\"";
  json += ",\"systemValid\":" + String(timeIsValid() ? "true" : "false");
  json += ",\"systemDate\":\"" + localDateString() + "\"";
  json += ",\"systemTime\":\"" + localTimeString() + "\"";
  json += ",\"ntpServer\":\"" + primaryNtp + "\"";
  json += ",\"wifi\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  json += ",\"rtcPresent\":" + String(hwRtcDetected ? "true" : "false");
  json += ",\"rtcValid\":" + String(hwRtcTimeValid ? "true" : "false");
  json += ",\"rtcOsf\":" + String(hwRtcOsf ? "true" : "false");
  json += ",\"rtcDate\":\"" + hwRtcDateText + "\"";
  json += ",\"rtcTime\":\"" + hwRtcTimeText + "\"";
  json += ",\"rtcDifferenceSec\":" + String(diff);
  json += ",\"rtcLastSync\":" + String(hwRtcLastSyncEpoch);
  json += ",\"hierarchy\":[\"NTP\",\"RTC\",\"Browser\",\"keine absolute Zeit\"]";
  json += ",\"correctionPolicy\":\"current_provisional_only\"";
  json += "}";
  return json;
}

static const char TIME_MANAGEMENT_JS[] PROGMEM = R"JS(
(function(){
'use strict';
function q(id){return document.getElementById(id)}
function signed(v){v=Number(v||0);return (v>0?'+':'')+v+' s'}
function sourceName(v){return v==='ntp'?'NTP':(v==='rtc'?'RTC':(v==='browser'?'Browser':'Keine absolute Zeit'))}
function getGrid(){var v=q('settings');return v&&v.querySelector('.infoGrid')}

function installStyle(){
  if(q('tmStyle'))return;
  var s=document.createElement('style');s.id='tmStyle';
  s.textContent='#hwRtcCard{display:none!important}.tmHierarchy{display:grid;grid-template-columns:repeat(4,minmax(120px,1fr));gap:8px;margin-bottom:14px}.tmStep{border:1px solid var(--line);border-radius:9px;padding:10px}.tmStep b{display:block;margin-bottom:3px}.tmSection{border-top:1px solid var(--line);margin-top:14px;padding-top:12px}@media(max-width:700px){.tmHierarchy{grid-template-columns:1fr 1fr}}';
  document.head.appendChild(s);
}

function buildCard(){
  installStyle();
  var grid=getGrid();if(!grid||q('tmTimeCard'))return;
  var c=document.createElement('div');c.className='infoBox';c.id='tmTimeCard';c.style.gridColumn='1 / -1';
  c.innerHTML='<h3><span class="infoIcon">&#128337;</span>Zeitverwaltung</h3>'+
    '<div class="tmHierarchy">'+
      '<div class="tmStep"><b>1. NTP</b><div class="small">Referenz / hoechste Prioritaet</div></div>'+
      '<div class="tmStep"><b>2. RTC</b><div class="small">Startzeit und Autarkbetrieb</div></div>'+
      '<div class="tmStep"><b>3. Browser</b><div class="small">Fallback ohne NTP und RTC</div></div>'+
      '<div class="tmStep"><b>4. Ohne Zeit</b><div class="small">nur relative Autarkzeit</div></div>'+
    '</div>'+
    '<div class="kv"><span>Aktive Zeitquelle</span><span id="tmSource">-</span><span>Systemzeit</span><span id="tmSystem">-</span><span>RTC</span><span id="tmRtcState">-</span><span>RTC-Zeit</span><span id="tmRtcTime">-</span><span>Differenz RTC / System</span><span id="tmDiff">-</span><span>Letzter RTC-Abgleich</span><span id="tmLastSync">-</span></div>'+
    '<div id="tmNtpSlot" class="tmSection"><b>NTP</b><div class="small" style="margin:5px 0 9px">NTP ist die verbindliche Referenz. Der primaere Server kann hier geprueft und gespeichert werden.</div></div>'+
    '<div class="tmSection"><b>Was passiert bei einer Zeitdifferenz?</b><div class="small" style="line-height:1.55;margin-top:6px">Beim Start wird zuerst eine gueltige RTC verwendet. Sobald NTP verfuegbar ist, wird die Systemzeit auf NTP gesetzt und die RTC danach auf NTP nachgefuehrt. Bereits dauerhaft gespeicherte historische Daten werden niemals stillschweigend veraendert. Automatisch korrigiert werden duerfen nur eindeutig als vorlaeufig erkannte Eintraege des aktuellen Start-/Synchronisationszeitraums. Autark-Ereignisse bleiben relativ zur Session; bei einer spaeteren Referenzzeit wird der Session-Anker korrigiert statt jedes Ereignis einzeln umzuschreiben.</div></div>'+
    '<div class="tmSection"><b>Datenstrategie</b><div class="small" style="line-height:1.55;margin-top:6px"><b>Aktuelle vorlaeufige Daten:</b> mit der erkannten Differenz verschieben. <b>Aktuelle Autark-Session:</b> Zeitanker korrigieren. <b>Aeltere Daten:</b> unveraendert lassen und eine erkannte Abweichung nur protokollieren.</div></div>';
  var anchor=q('extOriginalSettings');
  if(anchor&&anchor.parentNode===grid){if(anchor.nextSibling)grid.insertBefore(c,anchor.nextSibling);else grid.appendChild(c)}else grid.insertBefore(c,grid.firstChild);
}

function hideKvPair(id){
  var value=q(id);if(!value)return;var label=value.previousElementSibling;
  value.style.display='none';if(label)label.style.display='none';
}

function reorganizeDevice(){
  var date=q('devDate'),deviceBox=date&&date.closest('.infoBox');
  if(deviceBox){
    var h=deviceBox.querySelector('h3');if(h)h.innerHTML='<span class="infoIcon">&#128421;</span>Geraet';
    hideKvPair('devDate');hideKvPair('devTime');hideKvPair('devTimeSource');
  }

  var wifi=q('devWifi'),wifiBox=wifi&&wifi.closest('.infoBox');
  if(wifiBox){var wh=wifiBox.querySelector('h3');if(wh)wh.innerHTML='<span class="infoIcon">&#128246;</span>WLAN';}

  var slot=q('tmNtpSlot'),input=q('ntpServer');
  if(slot&&input){
    var label=document.querySelector('label[for="ntpServer"]'),field=input.closest('.fieldRow'),result=q('ntpResult');
    if(label&&label.parentNode!==slot)slot.appendChild(label);
    if(field&&field.parentNode!==slot)slot.appendChild(field);
    if(result&&result.parentNode!==slot)slot.appendChild(result);
  }

  var rtcCard=q('hwRtcCard');if(rtcCard)rtcCard.style.display='none';
}

function fmtEpoch(v){if(!v)return'-';var d=new Date(Number(v)*1000);return d.toLocaleString()}
function render(d){
  buildCard();reorganizeDevice();if(!q('tmTimeCard'))return;
  q('tmSource').textContent=sourceName(d.source);
  q('tmSystem').textContent=(d.systemDate||'-')+' '+(d.systemTime||'-');
  q('tmRtcState').textContent=!d.rtcPresent?'nicht vorhanden':(d.rtcValid?'erkannt / Zeit OK':(d.rtcOsf?'erkannt / OSF gesetzt':'erkannt / Zeit ungueltig'));
  q('tmRtcTime').textContent=d.rtcPresent?((d.rtcDate||'-')+' '+(d.rtcTime||'-')):'-';
  q('tmDiff').textContent=(d.rtcPresent&&d.rtcValid&&d.systemValid)?signed(d.rtcDifferenceSec):'-';
  q('tmLastSync').textContent=fmtEpoch(d.rtcLastSync);
}
async function refresh(){try{buildCard();reorganizeDevice();var r=await fetch('/api/time-management?x='+Date.now(),{cache:'no-store'});if(!r.ok)return;render(await r.json())}catch(e){}}

setTimeout(refresh,140);setTimeout(refresh,950);setInterval(refresh,10000);
})();
)JS";

static void tmServeJs() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "application/javascript; charset=utf-8", TIME_MANAGEMENT_JS);
}

class TimeManagementRegistrar {
public:
  TimeManagementRegistrar() {
    server.on("/time-management.js", HTTP_GET, tmServeJs);
    server.on("/api/time-management", HTTP_GET, []() {
      server.sendHeader("Cache-Control", "no-store");
      server.send(200, "application/json", tmTimeJson());
    });
  }
};

TimeManagementRegistrar timeManagementRegistrar;
