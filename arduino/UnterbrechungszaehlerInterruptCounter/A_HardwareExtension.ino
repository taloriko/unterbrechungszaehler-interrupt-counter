// A_HardwareExtension.ino
// Optional I2C hardware support for DS3231 RTC and SH1106 128x64 OLED.
// Hardware is detected once per boot. Missing modules do not block startup.

#include <Wire.h>
#include <esp_sntp.h>

static const uint8_t HW_I2C_SDA = 21;
static const uint8_t HW_I2C_SCL = 22;
static const uint8_t HW_RTC_ADDR = 0x68;
static const uint8_t HW_OLED_ADDR_1 = 0x3C;
static const uint8_t HW_OLED_ADDR_2 = 0x3D;
static const uint32_t HW_DISPLAY_BOOT_MS = 15000UL;

static bool hwInitialized = false;
static bool hwRtcDetected = false;
static bool hwRtcTimeValid = false;
static bool hwRtcOsf = false;
static bool hwDisplayDetected = false;
static bool hwDisplayActive = false;
static bool hwBrowserTimeWritten = false;
static uint8_t hwDisplayAddress = 0;
static uint32_t hwRtcLastSyncEpoch = 0;
static int64_t hwDisplayOffAtUs = 0;
static volatile bool hwNtpSyncPending = false;
static String hwRtcTimeText = "-";
static String hwRtcDateText = "-";

static uint8_t hwBcdToDec(uint8_t value) {
  return (uint8_t)((value >> 4) * 10 + (value & 0x0F));
}

static uint8_t hwDecToBcd(uint8_t value) {
  return (uint8_t)(((value / 10) << 4) | (value % 10));
}

static bool hwI2cProbe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

static bool hwRtcReadRegister(uint8_t reg, uint8_t& value) {
  if (!hwRtcDetected) return false;
  Wire.beginTransmission(HW_RTC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(HW_RTC_ADDR, (uint8_t)1) != 1) return false;
  value = Wire.read();
  return true;
}

static bool hwRtcWriteRegister(uint8_t reg, uint8_t value) {
  if (!hwRtcDetected) return false;
  Wire.beginTransmission(HW_RTC_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool hwRtcRead(struct tm& t, bool& osf) {
  if (!hwRtcDetected) return false;

  Wire.beginTransmission(HW_RTC_ADDR);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(HW_RTC_ADDR, (uint8_t)7) != 7) return false;

  uint8_t secReg = Wire.read();
  uint8_t minReg = Wire.read();
  uint8_t hourReg = Wire.read();
  uint8_t dowReg = Wire.read();
  uint8_t dayReg = Wire.read();
  uint8_t monthReg = Wire.read();
  uint8_t yearReg = Wire.read();
  (void)dowReg;

  int second = hwBcdToDec(secReg & 0x7F);
  int minute = hwBcdToDec(minReg & 0x7F);
  int hour = 0;
  if (hourReg & 0x40) {
    hour = hwBcdToDec(hourReg & 0x1F);
    bool pm = hourReg & 0x20;
    if (hour == 12) hour = 0;
    if (pm) hour += 12;
  } else {
    hour = hwBcdToDec(hourReg & 0x3F);
  }

  int day = hwBcdToDec(dayReg & 0x3F);
  int month = hwBcdToDec(monthReg & 0x1F);
  int year = 2000 + hwBcdToDec(yearReg);
  if (monthReg & 0x80) year += 100;

  uint8_t status = 0x80;
  if (!hwRtcReadRegister(0x0F, status)) return false;
  osf = (status & 0x80) != 0;

  if (second > 59 || minute > 59 || hour > 23 || day < 1 || day > 31 ||
      month < 1 || month > 12 || year < 2024 || year > 2199) {
    return false;
  }

  memset(&t, 0, sizeof(t));
  t.tm_sec = second;
  t.tm_min = minute;
  t.tm_hour = hour;
  t.tm_mday = day;
  t.tm_mon = month - 1;
  t.tm_year = year - 1900;
  t.tm_isdst = -1;
  return true;
}

static void hwRtcUpdateText() {
  if (!hwRtcDetected) {
    hwRtcTimeValid = false;
    hwRtcOsf = false;
    hwRtcTimeText = "-";
    hwRtcDateText = "-";
    return;
  }

  struct tm t;
  bool osf = false;
  if (!hwRtcRead(t, osf)) {
    hwRtcTimeValid = false;
    hwRtcOsf = osf;
    hwRtcTimeText = "-";
    hwRtcDateText = "-";
    return;
  }

  hwRtcOsf = osf;
  hwRtcTimeValid = !osf;
  char timeBuf[16];
  char dateBuf[16];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
  hwRtcTimeText = timeBuf;
  hwRtcDateText = dateBuf;
}

static bool hwBackfillCurrentAutarkStartAnchor() {
  if (!autarkMode || !autarkRingOk || !timeIsValid() || autarkHeader.count == 0) return false;

  File f = LittleFS.open(AUTARK_FILE, "r+");
  if (!f) return false;

  for (uint32_t n = autarkHeader.count; n > 0; n--) {
    uint32_t chronological = n - 1;
    AutarkRecord r = {};
    if (!readAutarkChronologicalFromFile(f, chronological, r)) continue;
    if (r.sessionId != autarkSessionId) break;
    if (r.type != AUTARK_START) continue;

    if (r.anchorEpoch != 0) {
      f.close();
      return true;
    }

    uint32_t elapsed = currentAutarkElapsed();
    uint32_t nowEpoch = (uint32_t)time(nullptr);
    r.anchorEpoch = nowEpoch > elapsed ? nowEpoch - elapsed : nowEpoch;
    uint32_t oldest = (autarkHeader.writeIndex + autarkHeader.capacity - autarkHeader.count) % autarkHeader.capacity;
    uint32_t physical = (oldest + chronological) % autarkHeader.capacity;
    if (!f.seek(autarkDataOffset(physical), SeekSet)) {
      f.close();
      return false;
    }
    bool ok = f.write((const uint8_t*)&r, sizeof(r)) == sizeof(r);
    f.flush();
    f.close();
    return ok;
  }

  f.close();
  return false;
}

static bool hwRtcApplyToSystem() {
  if (!hwRtcDetected) return false;

  struct tm rtcTm;
  bool osf = false;
  if (!hwRtcRead(rtcTm, osf) || osf) {
    hwRtcUpdateText();
    return false;
  }

  setenv("TZ", TZ_INFO, 1);
  tzset();
  time_t epoch = mktime(&rtcTm);
  if (epoch <= 1700000000) return false;

  struct timeval tv = {};
  tv.tv_sec = epoch;
  if (settimeofday(&tv, nullptr) != 0) return false;

  timeSource = "rtc";
  hwRtcUpdateText();
  if (autarkMode) hwBackfillCurrentAutarkStartAnchor();
  return timeIsValid();
}

static bool hwRtcWriteSystemTime() {
  if (!hwRtcDetected || !timeIsValid()) return false;

  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);

  Wire.beginTransmission(HW_RTC_ADDR);
  Wire.write((uint8_t)0x00);
  Wire.write(hwDecToBcd((uint8_t)t.tm_sec));
  Wire.write(hwDecToBcd((uint8_t)t.tm_min));
  Wire.write(hwDecToBcd((uint8_t)t.tm_hour));
  uint8_t dow = t.tm_wday == 0 ? 7 : (uint8_t)t.tm_wday;
  Wire.write(hwDecToBcd(dow));
  Wire.write(hwDecToBcd((uint8_t)t.tm_mday));
  Wire.write(hwDecToBcd((uint8_t)(t.tm_mon + 1)));
  Wire.write(hwDecToBcd((uint8_t)((t.tm_year + 1900) % 100)));
  if (Wire.endTransmission() != 0) return false;

  uint8_t status = 0;
  if (hwRtcReadRegister(0x0F, status)) {
    hwRtcWriteRegister(0x0F, status & (uint8_t)~0x80);
  }

  hwRtcLastSyncEpoch = (uint32_t)now;
  hwRtcUpdateText();
  return true;
}

static void hwNtpTimeSyncCallback(struct timeval* tv) {
  (void)tv;
  hwNtpSyncPending = true;
}

static bool hwOledCommand(uint8_t command) {
  if (!hwDisplayDetected || !hwDisplayAddress) return false;
  Wire.beginTransmission(hwDisplayAddress);
  Wire.write((uint8_t)0x00);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}

static bool hwOledInit() {
  if (!hwDisplayDetected) return false;
  const uint8_t initCommands[] = {
    0xAE,
    0xD5, 0x80,
    0xA8, 0x3F,
    0xD3, 0x00,
    0x40,
    0xAD, 0x8B,
    0xA1,
    0xC8,
    0xDA, 0x12,
    0x81, 0x7F,
    0xD9, 0x22,
    0xDB, 0x35,
    0xA4,
    0xA6,
    0xAF
  };

  for (size_t i = 0; i < sizeof(initCommands); i++) {
    if (!hwOledCommand(initCommands[i])) return false;
  }
  delay(20);
  return true;
}

static void hwOledSetPage(uint8_t page) {
  hwOledCommand((uint8_t)(0xB0 | (page & 0x07)));
  hwOledCommand(0x02);
  hwOledCommand(0x10);
}

static void hwOledWriteData(const uint8_t* data, size_t length) {
  size_t pos = 0;
  while (pos < length) {
    size_t chunk = length - pos;
    if (chunk > 16) chunk = 16;
    Wire.beginTransmission(hwDisplayAddress);
    Wire.write((uint8_t)0x40);
    for (size_t i = 0; i < chunk; i++) Wire.write(data[pos + i]);
    Wire.endTransmission();
    pos += chunk;
  }
}

static void hwOledClear() {
  uint8_t zeros[16] = {0};
  for (uint8_t page = 0; page < 8; page++) {
    hwOledSetPage(page);
    for (uint8_t block = 0; block < 8; block++) hwOledWriteData(zeros, sizeof(zeros));
  }
}

static const uint8_t HW_FONT_DIGITS[10][5] = {
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
};

static const uint8_t HW_FONT_UPPER[26][5] = {
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
  {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

static void hwGlyph(char c, uint8_t out[5]) {
  memset(out, 0, 5);
  if (c >= '0' && c <= '9') {
    memcpy(out, HW_FONT_DIGITS[c - '0'], 5);
    return;
  }
  if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
  if (c >= 'A' && c <= 'Z') {
    memcpy(out, HW_FONT_UPPER[c - 'A'], 5);
    return;
  }
  switch (c) {
    case ':': out[1] = 0x36; out[2] = 0x36; break;
    case '-': for (uint8_t i = 0; i < 5; i++) out[i] = 0x08; break;
    case '/': out[0]=0x20; out[1]=0x10; out[2]=0x08; out[3]=0x04; out[4]=0x02; break;
    case '.': out[1]=0x60; out[2]=0x60; break;
    case '_': for (uint8_t i = 0; i < 5; i++) out[i] = 0x40; break;
    default: break;
  }
}

static void hwOledText(uint8_t page, const String& text) {
  if (!hwDisplayDetected || page > 7) return;
  hwOledSetPage(page);
  uint8_t glyph[6];
  size_t maxChars = text.length();
  if (maxChars > 21) maxChars = 21;
  for (size_t i = 0; i < maxChars; i++) {
    hwGlyph(text[i], glyph);
    glyph[5] = 0x00;
    hwOledWriteData(glyph, sizeof(glyph));
  }
  uint8_t blank[6] = {0};
  for (size_t i = maxChars; i < 21; i++) hwOledWriteData(blank, sizeof(blank));
}

static void hwDisplayOff() {
  if (!hwDisplayDetected) return;
  hwOledCommand(0xAE);
  hwDisplayActive = false;
  hwDisplayOffAtUs = 0;
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
}

static void hwDisplayShowStatus(bool forceTest = false) {
  if (!hwDisplayDetected) return;
  if (!hwOledInit()) return;
  hwOledClear();

  if (autarkMode && !forceTest) hwOledText(0, "AUTARKMODUS");
  else hwOledText(0, "DISPLAY TEST");

  if (!hwRtcDetected) hwOledText(2, "RTC: NICHT ERKANNT");
  else if (!hwRtcTimeValid) hwOledText(2, "RTC: ZEIT FEHLT");
  else hwOledText(2, "RTC: ZEIT OK");

  String timeLine = "ZEIT: ";
  timeLine += timeIsValid() ? localTimeString() : String("--:--:--");
  hwOledText(4, timeLine);

  String modeLine = autarkMode ? String("AUTARK BEREIT") : String("SYSTEM BEREIT");
  hwOledText(6, modeLine);

  hwOledCommand(0xAF);
  hwDisplayActive = true;
  hwDisplayOffAtUs = esp_timer_get_time() + (int64_t)HW_DISPLAY_BOOT_MS * 1000LL;

  if (autarkMode) {
    esp_sleep_enable_timer_wakeup((uint64_t)HW_DISPLAY_BOOT_MS * 1000ULL);
  }
}

static void hwHardwareDetectOnce() {
  if (hwInitialized) return;
  hwInitialized = true;

  Wire.begin(HW_I2C_SDA, HW_I2C_SCL);
  Wire.setClock(100000);
  delay(5);

  hwRtcDetected = hwI2cProbe(HW_RTC_ADDR);
  if (hwI2cProbe(HW_OLED_ADDR_1)) {
    hwDisplayDetected = true;
    hwDisplayAddress = HW_OLED_ADDR_1;
  } else if (hwI2cProbe(HW_OLED_ADDR_2)) {
    hwDisplayDetected = true;
    hwDisplayAddress = HW_OLED_ADDR_2;
  }

  if (hwRtcDetected) {
    hwRtcUpdateText();
    if (hwRtcTimeValid) hwRtcApplyToSystem();
  }

  sntp_set_time_sync_notification_cb(hwNtpTimeSyncCallback);

  Serial.printf("[HW] RTC DS3231: %s", hwRtcDetected ? "erkannt" : "nicht erkannt");
  if (hwRtcDetected) Serial.printf(" | Zeit %s | OSF %s", hwRtcTimeText.c_str(), hwRtcOsf ? "gesetzt" : "OK");
  Serial.println();
  Serial.printf("[HW] OLED SH1106: %s", hwDisplayDetected ? "erkannt" : "nicht erkannt");
  if (hwDisplayDetected) Serial.printf(" | Adresse 0x%02X", hwDisplayAddress);
  Serial.println();

  if (autarkMode && hwDisplayDetected) {
    hwDisplayShowStatus(false);
  } else {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  }
}

static String hwHardwareJson() {
  hwRtcUpdateText();
  String json;
  json.reserve(620);
  json = "{\"ok\":true";
  json += ",\"detectedAtBoot\":true";
  json += ",\"rtc\":" + String(hwRtcDetected ? "true" : "false");
  json += ",\"rtcAddress\":\"0x68\"";
  json += ",\"rtcValid\":" + String(hwRtcTimeValid ? "true" : "false");
  json += ",\"rtcOsf\":" + String(hwRtcOsf ? "true" : "false");
  json += ",\"rtcDate\":\"" + hwRtcDateText + "\"";
  json += ",\"rtcTime\":\"" + hwRtcTimeText + "\"";
  json += ",\"rtcLastSync\":" + String(hwRtcLastSyncEpoch);
  json += ",\"display\":" + String(hwDisplayDetected ? "true" : "false");
  json += ",\"displayAddress\":\"";
  if (hwDisplayDetected) {
    char addr[8];
    snprintf(addr, sizeof(addr), "0x%02X", hwDisplayAddress);
    json += addr;
  } else json += "-";
  json += "\"";
  json += ",\"displayActive\":" + String(hwDisplayActive ? "true" : "false");
  json += ",\"displayBootSeconds\":15";
  json += ",\"sda\":21,\"scl\":22";
  json += ",\"autark\":" + String(autarkMode ? "true" : "false");
  json += ",\"timeSource\":\"" + timeSource + "\"";
  json += "}";
  return json;
}

static const char HARDWARE_EXTENSION_JS[] PROGMEM = R"JS(
(function(){
'use strict';
function q(id){return document.getElementById(id)}
function esc(v){return String(v==null?'':v).replace(/[&<>"']/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]})}

function installStyle(){
  if(q('hardwareExtStyle'))return;
  var s=document.createElement('style');s.id='hardwareExtStyle';
  s.textContent='.hwIcons{display:inline-flex;gap:6px;margin-left:4px;align-items:center}.hwIcon{display:inline-flex;width:24px;height:24px;align-items:center;justify-content:center;border:1px solid var(--line);border-radius:7px;color:var(--muted);opacity:.28;font-size:1rem}.hwIcon.on{opacity:1;color:var(--ok);border-color:var(--ok)}.hwStateOk{color:var(--ok)}.hwStateBad{color:var(--danger)}';
  document.head.appendChild(s);
}

function buildIcons(){
  installStyle();
  var title=document.querySelector('.headTitle');if(!title||q('hwIcons'))return;
  var box=document.createElement('span');box.id='hwIcons';box.className='hwIcons';
  box.innerHTML='<span id="hwRtcIcon" class="hwIcon" title="RTC nicht erkannt">&#9719;</span><span id="hwDisplayIcon" class="hwIcon" title="Display nicht erkannt">&#9635;</span>';
  title.appendChild(box);
}

function settingsGrid(){
  var v=q('settings');return v&&v.querySelector('.infoGrid');
}

function ensureRtcCard(data){
  var grid=settingsGrid();if(!grid)return;
  var card=q('hwRtcCard');
  if(!data.rtc){if(card)card.remove();return}
  if(!card){
    card=document.createElement('div');card.className='infoBox';card.id='hwRtcCard';
    card.innerHTML='<h3><span class="infoIcon">&#9719;</span>RTC / DS3231</h3><div class="kv"><span>Status</span><span id="hwRtcStatus">-</span><span>RTC-Datum</span><span id="hwRtcDate">-</span><span>RTC-Zeit</span><span id="hwRtcTime">-</span><span>Zeitquelle</span><span id="hwRtcSource">-</span><span>I2C-Adresse</span><span>0x68</span><span>Bus</span><span>GPIO21 / GPIO22</span></div><div class="actions" style="margin-top:12px"><button id="hwRtcSyncBtn" class="btn" type="button">RTC mit Systemzeit synchronisieren</button><span id="hwRtcAction" class="small"></span></div>';
    grid.appendChild(card);
    q('hwRtcSyncBtn').addEventListener('click',syncRtc);
  }
  q('hwRtcStatus').textContent=data.rtcValid?'Erkannt / Zeit OK':(data.rtcOsf?'Erkannt / Zeit ungueltig (OSF)':'Erkannt / Zeit ungueltig');
  q('hwRtcStatus').className=data.rtcValid?'hwStateOk':'hwStateBad';
  q('hwRtcDate').textContent=data.rtcDate||'-';q('hwRtcTime').textContent=data.rtcTime||'-';q('hwRtcSource').textContent=data.timeSource||'-';
}

function ensureDisplayCard(data){
  var grid=settingsGrid();if(!grid)return;
  var card=q('hwDisplayCard');
  if(!data.display){if(card)card.remove();return}
  if(!card){
    card=document.createElement('div');card.className='infoBox';card.id='hwDisplayCard';
    card.innerHTML='<h3><span class="infoIcon">&#9635;</span>Display / SH1106</h3><div class="kv"><span>Status</span><span class="hwStateOk">Erkannt</span><span>Aufloesung</span><span>128 x 64</span><span>I2C-Adresse</span><span id="hwDisplayAddr">-</span><span>Bus</span><span>GPIO21 / GPIO22</span><span>Autark-Bootanzeige</span><span>15 Sekunden</span></div><div class="actions" style="margin-top:12px"><button id="hwDisplayTestBtn" class="btn" type="button">Display 15 s testen</button><span id="hwDisplayAction" class="small"></span></div>';
    grid.appendChild(card);
    q('hwDisplayTestBtn').addEventListener('click',testDisplay);
  }
  q('hwDisplayAddr').textContent=data.displayAddress||'-';
}

function render(data){
  buildIcons();
  var ri=q('hwRtcIcon'),di=q('hwDisplayIcon');
  if(ri){ri.classList.toggle('on',!!data.rtc);ri.title=data.rtc?(data.rtcValid?'RTC erkannt - Zeit OK':'RTC erkannt - Zeit ungueltig'):'RTC nicht erkannt'}
  if(di){di.classList.toggle('on',!!data.display);di.title=data.display?'SH1106 Display erkannt':'Display nicht erkannt'}
  ensureRtcCard(data);ensureDisplayCard(data);
}

async function refresh(){
  try{var r=await fetch('/api/hardware?x='+Date.now(),{cache:'no-store'});if(!r.ok)return;render(await r.json())}catch(e){}
}

async function syncRtc(){
  var out=q('hwRtcAction');if(out)out.textContent='Synchronisiere...';
  try{var r=await fetch('/api/hardware/rtc-sync',{method:'POST'});var d=await r.json();if(out){out.textContent=d.ok?'RTC aktualisiert':'Keine gueltige Systemzeit';out.style.color=d.ok?'var(--ok)':'var(--danger)'}refresh()}catch(e){if(out){out.textContent='Fehler';out.style.color='var(--danger)'}}
}

async function testDisplay(){
  var out=q('hwDisplayAction');if(out)out.textContent='Testanzeige aktiv...';
  try{var r=await fetch('/api/hardware/display-test',{method:'POST'});var d=await r.json();if(out){out.textContent=d.ok?'Display schaltet nach 15 s aus':'Display nicht verfuegbar';out.style.color=d.ok?'var(--ok)':'var(--danger)'}refresh()}catch(e){if(out){out.textContent='Fehler';out.style.color='var(--danger)'}}
}

buildIcons();
setTimeout(refresh,80);setTimeout(refresh,700);setInterval(refresh,10000);
})();
)JS";

static void hwServeExtensionJs() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "application/javascript; charset=utf-8", HARDWARE_EXTENSION_JS);
}

static void hwServeExtendedIndex() {
  String page = FPSTR(INDEX_HTML);
  const char* scripts = "<script src=\"/heatmap-extension.js?v=19\"></script><script src=\"/hardware-extension.js?v=19\"></script><script src=\"/time-management.js?v=19\"></script></body>";
  page.replace("</body>", scripts);
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "text/html; charset=utf-8", page);
}

static void hwServeStatus() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", hwHardwareJson());
}

static void hwServeRtcSync() {
  bool ok = hwRtcWriteSystemTime();
  server.send(ok ? 200 : 409, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

static void hwServeDisplayTest() {
  if (!hwDisplayDetected) {
    server.send(404, "application/json", "{\"ok\":false}");
    return;
  }
  hwDisplayShowStatus(true);
  server.send(200, "application/json", "{\"ok\":true}");
}

class HardwareExtensionRegistrar {
public:
  HardwareExtensionRegistrar() {
    server.on("/", HTTP_GET, hwServeExtendedIndex);
    server.on("/hardware-extension.js", HTTP_GET, hwServeExtensionJs);
    server.on("/api/hardware", HTTP_GET, hwServeStatus);
    server.on("/api/hardware/rtc-sync", HTTP_POST, hwServeRtcSync);
    server.on("/api/hardware/display-test", HTTP_POST, hwServeDisplayTest);
    esp_sleep_enable_timer_wakeup(750000ULL);
  }
};

HardwareExtensionRegistrar hardwareExtensionRegistrar;

static void hardwareExtensionTick() {
  if (!hwInitialized) hwHardwareDetectOnce();

  if (hwNtpSyncPending) {
    hwNtpSyncPending = false;
    if (timeIsValid()) {
      timeSource = "ntp";
      if (hwRtcDetected) hwRtcWriteSystemTime();
    }
  }

  if (hwRtcDetected && timeSource == "browser" && !hwBrowserTimeWritten && timeIsValid()) {
    hwBrowserTimeWritten = hwRtcWriteSystemTime();
  }

  if (hwDisplayActive && hwDisplayOffAtUs > 0) {
    int64_t remaining = hwDisplayOffAtUs - esp_timer_get_time();
    if (remaining <= 0) {
      hwDisplayOff();
    } else if (autarkMode) {
      esp_sleep_enable_timer_wakeup((uint64_t)remaining);
    }
  }
}
