#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
constexpr char BRIDGE_VERSION[] = "2.1.2-bridge";
constexpr char AP_SSID[] = "Unterbrechungszaehler-OTA";

WebServer server(80);
Preferences prefs;
bool restartPending = false;
uint32_t restartAt = 0;
bool updateOk = false;

String page() {
  return String(
    "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>OTA Bridge</title><style>body{font-family:Arial,sans-serif;max-width:620px;margin:40px auto;padding:0 16px}"
    "button,input{font:inherit;padding:12px;margin:8px 0;max-width:100%}button{cursor:pointer}#s{margin-top:12px}</style></head><body>"
    "<h2>Unterbrechungszaehler OTA Bridge</h2><p>Zwischen-Firmware fuer ein robustes OTA-Update.</p>"
    "<p>Version: ") + BRIDGE_VERSION +
    "</p><form id='f'><input id='b' type='file' accept='.bin,application/octet-stream' required><br>"
    "<button type='submit'>Firmware installieren</button></form><div id='s'></div>"
    "<script>f.onsubmit=async e=>{e.preventDefault();let x=b.files[0];if(!x)return;s.textContent='Upload laeuft ...';"
    "let d=new FormData();d.append('update',x,x.name);try{let r=await fetch('/update',{method:'POST',body:d});"
    "let t=await r.text();s.textContent=r.ok?'Update erfolgreich - Neustart ...':'Fehler: '+t}catch(e){s.textContent='Verbindung abgebrochen. Wenn das Geraet neu startet, bitte erneut verbinden.'}}</script>"
    "</body></html>";
}

void startNetwork() {
  String ssid;
  String pass;
  if (prefs.begin("uic-network", true)) {
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("unterbrechungen");
  WiFi.setAutoReconnect(true);

  if (ssid.length()) WiFi.begin(ssid.c_str(), pass.c_str());
  else WiFi.begin();

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 12000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[BRIDGE] WLAN: %s\n", WiFi.localIP().toString().c_str());
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);
  Serial.printf("[BRIDGE] Fallback-AP: %s | %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void startWeb() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", page());
  });

  server.on("/status", HTTP_GET, []() {
    String json = String("{\"version\":\"") + BRIDGE_VERSION +
                  "\",\"freeSketch\":" + String(ESP.getFreeSketchSpace()) +
                  ",\"sketchSize\":" + String(ESP.getSketchSize()) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/update", HTTP_POST,
    []() {
      updateOk = updateOk && !Update.hasError();
      if (!updateOk) Update.printError(Serial);
      server.send(updateOk ? 200 : 500,
                  "text/plain; charset=utf-8",
                  updateOk ? "OK" : "OTA fehlgeschlagen - siehe serielle Ausgabe");
      if (updateOk) {
        restartPending = true;
        restartAt = millis() + 1200;
      }
    },
    []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[BRIDGE] OTA Start: %s | freier OTA-Platz: %u\n",
                      upload.filename.c_str(), ESP.getFreeSketchSpace());
        updateOk = Update.begin(UPDATE_SIZE_UNKNOWN);
        if (!updateOk) Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (updateOk && Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          updateOk = false;
          Update.printError(Serial);
        }
        yield();
      } else if (upload.status == UPLOAD_FILE_END) {
        if (updateOk) updateOk = Update.end(true);
        if (!updateOk) Update.printError(Serial);
        Serial.printf("[BRIDGE] OTA Ende: %u Bytes | %s\n", upload.totalSize, updateOk ? "OK" : "FEHLER");
      } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
        updateOk = false;
        Serial.println("[BRIDGE] OTA abgebrochen");
      }
    });

  server.begin();
}
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n[BRIDGE] Unterbrechungszaehler %s\n", BRIDGE_VERSION);
  Serial.printf("[BRIDGE] Sketch=%u | freier OTA-Platz=%u\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());
  startNetwork();
  startWeb();
}

void loop() {
  server.handleClient();
  if (restartPending && static_cast<int32_t>(millis() - restartAt) >= 0) ESP.restart();
  delay(1);
}
