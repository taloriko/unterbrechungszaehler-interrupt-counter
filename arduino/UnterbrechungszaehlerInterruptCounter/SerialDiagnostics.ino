// SerialDiagnostics.ino
// Separate diagnostics for the Unterbrechungszaehler firmware.
// Kept outside the main sketch file so UI changes cannot accidentally remove it again.

#include <esp_system.h>

static const char* SERIAL_DIAG_VERSION = "2026-08-29-12";

class RuntimeVersionInitializer {
public:
  RuntimeVersionInitializer() {
    APP_VERSION = SERIAL_DIAG_VERSION;
  }
};

RuntimeVersionInitializer runtimeVersionInitializer;

static bool diagInitialized = false;
static bool diagLastWifi = false;
static bool diagLastAp = false;
static bool diagLastTimeValid = false;
static bool diagLastAutark = false;
static bool diagLastFsOk = false;
static bool diagLastRingOk = false;
static bool diagLastAutarkRingOk = false;
static uint32_t diagLastEventCount = 0;
static uint32_t diagLastAutarkCount = 0;
static uint32_t diagLastPulseSeq = 0;
static uint32_t diagLastActionSeq = 0;
static uint32_t diagLastStatusAt = 0;

static const char* diagResetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "Power-on";
    case ESP_RST_EXT: return "External reset";
    case ESP_RST_SW: return "Software reset";
    case ESP_RST_PANIC: return "Panic / crash";
    case ESP_RST_INT_WDT: return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT: return "Watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO: return "SDIO";
    default: return "Unknown";
  }
}

static void diagPrintSeparator() {
  Serial.println("------------------------------------------------------------");
}

static void diagPrintFallbackStatus() {
  if (apActive) {
    Serial.printf("[AP] Fallback-WLAN AKTIV | SSID=%s | IP=%s | URL=http://%s\n",
                  FALLBACK_AP_SSID,
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("[AP] Fallback-WLAN AUS | SSID=%s | feste IP=%s\n",
                  FALLBACK_AP_SSID,
                  FALLBACK_AP_IP.toString().c_str());
  }
}

static void diagPrintStartup() {
  Serial.println();
  diagPrintSeparator();
  Serial.printf("Unterbrechungszaehler / Interrupt Counter %s\n", APP_VERSION);
  Serial.printf("Reset-Grund: %s\n", diagResetReason());
  Serial.printf("Chip: %s Rev.%u | %u Kerne | %u MHz\n",
                ESP.getChipModel(),
                ESP.getChipRevision(),
                ESP.getChipCores(),
                ESP.getCpuFreqMHz());
  Serial.printf("Heap: %u frei / %u gesamt Bytes\n",
                ESP.getFreeHeap(), ESP.getHeapSize());
  Serial.printf("Flash: %u Bytes | Sketch: %u Bytes | Sketch frei: %u Bytes\n",
                ESP.getFlashChipSize(), ESP.getSketchSize(), ESP.getFreeSketchSpace());
  Serial.printf("Primaerer NTP-Server: %s\n", primaryNtp.c_str());
  Serial.printf("LittleFS: %s\n", fsOk ? "OK" : "FEHLER");
  Serial.printf("Ringspeicher: %s | %lu/%lu Eintraege\n",
                ringOk ? "OK" : "FEHLER",
                (unsigned long)ringHeader.count,
                (unsigned long)ringHeader.capacity);
  Serial.printf("Autark-Ringspeicher: %s | %lu/%lu Datensaetze\n",
                autarkRingOk ? "OK" : "FEHLER",
                (unsigned long)autarkHeader.count,
                (unsigned long)autarkHeader.capacity);
  Serial.printf("Startmodus: %s\n", autarkMode ? "AUTARK" : "NORMAL");
  diagPrintFallbackStatus();
  diagPrintSeparator();
}

static void diagPrintPeriodicStatus() {
  bool wifi = WiFi.status() == WL_CONNECTED;
  Serial.printf("[STATUS] Mode=%s | WLAN=%s",
                autarkMode ? "AUTARK" : "NORMAL",
                wifi ? "OK" : "OFFLINE");
  if (wifi) {
    Serial.printf(" | IP=%s | RSSI=%d dBm",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else if (apActive) {
    Serial.printf(" | Fallback-AP=%s | AP-IP=%s",
                  FALLBACK_AP_SSID, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf(" | Fallback-AP=AUS | AP-IP=%s", FALLBACK_AP_IP.toString().c_str());
  }
  Serial.printf(" | Zeit=%s/%s | Events=%lu | Autark=%lu | Heap=%u\n",
                timeIsValid() ? "OK" : "UNGUELTIG",
                timeSource.c_str(),
                (unsigned long)ringHeader.count,
                (unsigned long)autarkHeader.count,
                ESP.getFreeHeap());
}

static void diagCheckTransitions() {
  bool wifi = WiFi.status() == WL_CONNECTED;
  bool validTime = timeIsValid();

  if (wifi != diagLastWifi) {
    if (wifi) {
      Serial.printf("[WLAN] Verbunden: %s | IP %s | RSSI %d dBm\n",
                    WIFI_SSID, WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      Serial.println("[WLAN] Normales WLAN getrennt / nicht erreichbar.");
    }
    diagLastWifi = wifi;
  }

  if (apActive != diagLastAp) {
    diagPrintFallbackStatus();
    diagLastAp = apActive;
  }

  if (validTime != diagLastTimeValid) {
    if (validTime) {
      Serial.printf("[ZEIT] Gueltig: %s %s | Quelle: %s\n",
                    localDateString().c_str(), localTimeString().c_str(), timeSource.c_str());
    } else {
      Serial.println("[ZEIT] Keine gueltige absolute Zeit.");
    }
    diagLastTimeValid = validTime;
  }

  if (autarkMode != diagLastAutark) {
    if (autarkMode) {
      Serial.printf("[AUTARK] EIN | Session %lu | Startanker %s\n",
                    (unsigned long)autarkSessionId,
                    validTime ? localTimeString().c_str() : "ohne absolute Zeit");
    } else {
      Serial.printf("[AUTARK] AUS | letzte Laufzeit %lu s | Netzwerk startet wieder\n",
                    (unsigned long)lastAutarkElapsed);
    }
    diagLastAutark = autarkMode;
  }

  if (fsOk != diagLastFsOk) {
    Serial.printf("[SPEICHER] LittleFS: %s\n", fsOk ? "OK" : "FEHLER");
    diagLastFsOk = fsOk;
  }
  if (ringOk != diagLastRingOk) {
    Serial.printf("[SPEICHER] Ringspeicher: %s\n", ringOk ? "OK" : "FEHLER");
    diagLastRingOk = ringOk;
  }
  if (autarkRingOk != diagLastAutarkRingOk) {
    Serial.printf("[SPEICHER] Autark-Ringspeicher: %s\n", autarkRingOk ? "OK" : "FEHLER");
    diagLastAutarkRingOk = autarkRingOk;
  }

  if (ringHeader.count != diagLastEventCount) {
    if (ringHeader.count > diagLastEventCount) {
      Serial.printf("[EVENT] Gespeichert | %lu/%lu | Unix %lu\n",
                    (unsigned long)ringHeader.count,
                    (unsigned long)ringHeader.capacity,
                    (unsigned long)getLastEvent());
    } else {
      Serial.printf("[EVENT] Letzter Eintrag geloescht | verbleibend %lu\n",
                    (unsigned long)ringHeader.count);
    }
    diagLastEventCount = ringHeader.count;
  }

  if (autarkHeader.count != diagLastAutarkCount) {
    Serial.printf("[AUTARK] Ringspeicher jetzt %lu/%lu Datensaetze | Session %lu | Pulse %lu\n",
                  (unsigned long)autarkHeader.count,
                  (unsigned long)autarkHeader.capacity,
                  (unsigned long)autarkSessionId,
                  (unsigned long)autarkSessionEvents);
    diagLastAutarkCount = autarkHeader.count;
  }

  if (physicalPulseSeq != diagLastPulseSeq) {
    Serial.printf("[TASTER] Physischer Puls erkannt | Zaehler %lu\n",
                  (unsigned long)physicalPulseSeq);
    diagLastPulseSeq = physicalPulseSeq;
  }

  if (uiActionSeq != diagLastActionSeq) {
    Serial.printf("[AKTION] %s | Sequenz %lu\n",
                  uiActionKind == 2 ? "Loeschen" : "Hinzufuegen",
                  (unsigned long)uiActionSeq);
    diagLastActionSeq = uiActionSeq;
  }
}

void serialEventRun(void) {
  if (!diagInitialized) {
    diagInitialized = true;
    diagLastWifi = WiFi.status() == WL_CONNECTED;
    diagLastAp = apActive;
    diagLastTimeValid = timeIsValid();
    diagLastAutark = autarkMode;
    diagLastFsOk = fsOk;
    diagLastRingOk = ringOk;
    diagLastAutarkRingOk = autarkRingOk;
    diagLastEventCount = ringHeader.count;
    diagLastAutarkCount = autarkHeader.count;
    diagLastPulseSeq = physicalPulseSeq;
    diagLastActionSeq = uiActionSeq;
    diagLastStatusAt = millis();
    diagPrintStartup();
    return;
  }

  diagCheckTransitions();

  if (millis() - diagLastStatusAt >= 60000UL) {
    diagLastStatusAt = millis();
    diagPrintPeriodicStatus();
  }
}
