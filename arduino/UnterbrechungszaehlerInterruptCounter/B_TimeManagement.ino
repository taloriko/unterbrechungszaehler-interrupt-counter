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
  json.reserve(720);
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

function buildCard(){
  var grid=getGrid();if(!grid||q('tmTimeCard'))return;
  var c=document.createElement('div');c.className='infoBox';c.id='tmTimeCard';
  c.style.gridColumn='1 / -1';
  c.innerHTML='<h3><span class="infoIcon">&#128337;</span>Zeitverwaltung</h3>'+
    '<div style="display:grid;grid-template-columns:repeat(4,minmax(120px,1fr));gap:8px;margin-bottom:14px">'+
      '<div class="infoBox" style="padding:10px"><b>1. NTP</b><div class="small">Referenz / hoechste Prioritaet</div></div>'+
      '<div class="infoBox" style="padding:10px"><b>2. RTC</b><div class="small">Startzeit und Autarkbetrieb</div></div>'+
      '<div class="infoBox" style="padding:10px"><b>3. Browser</b><div class="small">Fallback ohne NTP/RTC</div></div>'+
      '<div class="infoBox" style="padding:10px"><b>4. Ohne Zeit</b><div class="small">nur relative Autarkzeit</div></div>'+
    '</div>'+
    '<div class="kv"><span>Aktive Zeitquelle</span><span id="tmSource">-</span><span>Systemzeit</span><span id="tmSystem">-</span><span>NTP-Server</span><span id="tmNtp">-</span><span>RTC</span><span id="tmRtcState">-</span><span>RTC-Zeit</span><span id="tmRtcTime">-</span><span>Aktuelle Differenz RTC / System</span><span id="tmDiff">-</span><span>Letzter RTC-Abgleich</span><span id="tmLastSync">-</span></div>'+
    '<div style="border-top:1px solid var(--line);margin-top:14px;padding-top:12px"><b>Was passiert bei einer Zeitdifferenz?</b><div class="small" style="line-height:1.55;margin-top:6px">NTP ist die Referenz. Beim Start wird zuerst eine gueltige RTC verwendet. Sobald NTP verfuegbar ist, wird die Systemzeit auf NTP gesetzt und die RTC danach auf NTP nachgefuehrt. Bereits dauerhaft gespeicherte historische Daten werden niemals stillschweigend veraendert. Automatisch korrigiert werden duerfen nur eindeutig als vorlaeufig erkannte Eintraege des aktuellen Start-/Synchronisationszeitraums. Autark-Ereignisse bleiben relativ zur Session; bei einer spaeteren Referenzzeit wird der Session-Anker korrigiert statt jedes Ereignis einzeln umzuschreiben.</div></div>'+
    '<div style="border-top:1px solid var(--line);margin-top:12px;padding-top:12px"><b>Datenstrategie</b><div class="small" style="line-height:1.55;margin-top:6px"><b>Aktuelle vorlaeufige Daten:</b> mit der erkannten Differenz verschieben. <b>Aktuelle Autark-Session:</b> Zeitanker korrigieren. <b>Aeltere Daten:</b> unveraendert lassen und eine erkannte Abweichung nur protokollieren. Damit kann eine falsche RTC niemals unbemerkt Monate oder Jahre Historie verschieben.</div></div>';
  var first=grid.firstElementChild;if(first)grid.insertBefore(c,first);else grid.appendChild(c);
}

function fmtEpoch(v){if(!v)return'-';var d=new Date(Number(v)*1000);return d.toLocaleString()}
function render(d){
  buildCard();if(!q('tmTimeCard'))return;
  q('tmSource').textContent=sourceName(d.source);
  q('tmSystem').textContent=(d.systemDate||'-')+' '+(d.systemTime||'-');
  q('tmNtp').textContent=d.ntpServer||'-';
  q('tmRtcState').textContent=!d.rtcPresent?'nicht vorhanden':(d.rtcValid?'erkannt / Zeit OK':(d.rtcOsf?'erkannt / OSF gesetzt':'erkannt / Zeit ungueltig'));
  q('tmRtcTime').textContent=d.rtcPresent?((d.rtcDate||'-')+' '+(d.rtcTime||'-')):'-';
  q('tmDiff').textContent=(d.rtcPresent&&d.rtcValid&&d.systemValid)?signed(d.rtcDifferenceSec):'-';
  q('tmLastSync').textContent=fmtEpoch(d.rtcLastSync);
}
async function refresh(){try{var r=await fetch('/api/time-management?x='+Date.now(),{cache:'no-store'});if(!r.ok)return;render(await r.json())}catch(e){}}
setTimeout(refresh,120);setTimeout(refresh,900);setInterval(refresh,10000);
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
