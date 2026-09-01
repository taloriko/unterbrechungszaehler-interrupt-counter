#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include "esp32-hal-cpu.h"

#include "AnalyticsService.h"
#include "AutarkService.h"
#include "Config.h"
#include "CounterService.h"
#include "DisplayService.h"
#include "InputService.h"
#include "LedService.h"
#include "NetworkService.h"
#include "RtcService.h"
#include "SoundService.h"
#include "StorageService.h"
#include "TimeService.h"
#include "WatchdogService.h"
#include "WebService.h"

// ============================================================
// Unterbrechungszaehler / Interrupt Counter - modularer Reboot
// ============================================================
// Diese Datei enthaelt absichtlich nur den Programmablauf.
// Fachlogik, Hardwarezugriff, Speicherung und Weboberflaeche liegen
// in eigenen Modulen. Neue Funktionen sollen nicht hier eingebaut,
// sondern als klar abgegrenztes Modul ergaenzt werden.
// ============================================================

StorageService storage;
RtcService rtc;
TimeService timeService;
LedService led;
CounterService counter;
AutarkService autark;
InputService input;
NetworkService network;
DisplayService display;
AnalyticsService analytics;
WebService web;
SoundService sound;
WatchdogService watchdog;

bool previousAutarkMode = false;
uint32_t lastDiagnosticAt = 0;
bool analyticsReadyAtBoot = false;
char rtcDiagnosticDetail[128] = "-";

void configureAutarkWakeup() {
  gpio_wakeup_enable(static_cast<gpio_num_t>(UicConfig::BUTTON_PIN), GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable(static_cast<gpio_num_t>(UicConfig::AUTARK_PIN), GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
}

void disableAutarkWakeup() {
  gpio_wakeup_disable(static_cast<gpio_num_t>(UicConfig::BUTTON_PIN));
  gpio_wakeup_disable(static_cast<gpio_num_t>(UicConfig::AUTARK_PIN));
}

void applyModeTransition() {
  const bool active = autark.active();
  if (active == previousAutarkMode) return;

  previousAutarkMode = active;
  if (active) {
    network.stop();
    web.stop();
    setCpuFrequencyMhz(80);
    configureAutarkWakeup();
    display.showBootStatus(true);
    Serial.printf("[MODUS] Autark EIN | Session %lu\n", static_cast<unsigned long>(autark.sessionId()));
  } else {
    disableAutarkWakeup();
    setCpuFrequencyMhz(240);
    network.begin();
    display.showBootStatus(false);
    Serial.println("[MODUS] Autark AUS | Netzwerk wird gestartet");
  }
}

void updateModuleStatuses() {
  watchdog.setStatus(WatchdogService::MainLoop, ModuleState::Ready, "loop_cycle");
  watchdog.setStatus(WatchdogService::Input, ModuleState::Ready, "gpio");
  watchdog.setStatus(WatchdogService::Mode,
                     autark.active() ? ModuleState::Busy : ModuleState::Ready,
                     autark.active() ? "autark" : "normal");

  if (!storage.recentReady()) {
    watchdog.setStatus(WatchdogService::Storage, ModuleState::Error, "recent_unavailable");
  } else if (!storage.archiveReady() || !storage.autarkReady() || !storage.archiveSynchronized()) {
    watchdog.setStatus(WatchdogService::Storage, ModuleState::Degraded, "partial");
  } else {
    watchdog.setStatus(WatchdogService::Storage, ModuleState::Ready, "rings_ok");
  }

  watchdog.setStatus(WatchdogService::Time,
                     timeService.valid() ? ModuleState::Ready : ModuleState::Degraded,
                     timeService.valid() ? timeSourceName(timeService.source()) : "invalid");

  if (!rtc.present()) {
    watchdog.setStatus(WatchdogService::Rtc, ModuleState::NotDetected, "optional");
  } else {
    const float temp = rtc.temperatureC();
    if (isnan(temp)) {
      snprintf(rtcDiagnosticDetail, sizeof(rtcDiagnosticDetail),
               "comm=%s;valid=%s;osf=%s;age=%lu",
               rtc.communicationOk() ? "ok" : "error",
               rtc.timeValid() ? "yes" : "no",
               rtc.oscillatorStopFlag() ? "set" : "clear",
               static_cast<unsigned long>(rtc.lastCheckAgeMs()));
    } else {
      snprintf(rtcDiagnosticDetail, sizeof(rtcDiagnosticDetail),
               "comm=%s;valid=%s;osf=%s;temp=%.2f;age=%lu",
               rtc.communicationOk() ? "ok" : "error",
               rtc.timeValid() ? "yes" : "no",
               rtc.oscillatorStopFlag() ? "set" : "clear",
               temp,
               static_cast<unsigned long>(rtc.lastCheckAgeMs()));
    }
    watchdog.setStatus(WatchdogService::Rtc,
                       rtc.communicationOk() && rtc.timeValid() ? ModuleState::Ready : ModuleState::Degraded,
                       rtcDiagnosticDetail);
  }

  watchdog.setStatus(WatchdogService::Autark,
                     autark.active() ? ModuleState::Busy : ModuleState::Ready,
                     autark.active() ? "session_active" : "standby");

  if (display.present()) {
    watchdog.setStatus(WatchdogService::Display,
                       display.active() ? ModuleState::Busy : ModuleState::Ready,
                       display.active() ? "active" : "idle");
  } else if (display.simulationEnabled()) {
    watchdog.setStatus(WatchdogService::Display, ModuleState::Degraded, "simulation");
  } else {
    watchdog.setStatus(WatchdogService::Display, ModuleState::NotDetected, "optional");
  }

  watchdog.setStatus(WatchdogService::Led, ModuleState::Ready, "gpio");

  if (autark.active()) {
    watchdog.setStatus(WatchdogService::Network, ModuleState::Disabled, "autark");
    watchdog.setStatus(WatchdogService::Web, ModuleState::Disabled, "autark");
  } else {
    if (network.connected()) {
      watchdog.setStatus(WatchdogService::Network, ModuleState::Ready, "wifi");
    } else if (network.accessPointActive()) {
      watchdog.setStatus(WatchdogService::Network, ModuleState::Degraded, "fallback_ap");
    } else {
      watchdog.setStatus(WatchdogService::Network, ModuleState::Initializing, "connecting");
    }
    watchdog.setStatus(WatchdogService::Web,
                       web.started() ? ModuleState::Ready : ModuleState::Initializing,
                       web.started() ? "http_80" : "starting");
  }

  watchdog.setStatus(WatchdogService::Analytics,
                     analyticsReadyAtBoot ? ModuleState::Ready : ModuleState::Degraded,
                     analyticsReadyAtBoot ? "cache_ready" : "cache_rebuild");
  watchdog.setStatus(WatchdogService::Sound, sound.moduleState(), sound.statusDetail());
  watchdog.setStatus(WatchdogService::Diagnostics, ModuleState::Ready, "serial");
}

void printDiagnostics() {
  if (millis() - lastDiagnosticAt < UicConfig::DIAGNOSTIC_INTERVAL_MS) return;
  lastDiagnosticAt = millis();

  Serial.printf("[STATUS] Modus=%s | WLAN=%s | Zeit=%s/%s | Events=%lu/%lu | Langzeit=%lu/%lu | Autark=%lu/%lu | Heap=%u | Sound=%s sent=%lu done=%lu\n",
                autark.active() ? "AUTARK" : "NORMAL",
                network.connected() ? "OK" : "OFFLINE",
                timeService.valid() ? "OK" : "UNGUELTIG",
                timeSourceName(timeService.source()),
                static_cast<unsigned long>(storage.recentCount()),
                static_cast<unsigned long>(storage.recentCapacity()),
                static_cast<unsigned long>(storage.archiveCount()),
                static_cast<unsigned long>(storage.archiveCapacity()),
                static_cast<unsigned long>(storage.autarkCount()),
                static_cast<unsigned long>(storage.autarkCapacity()),
                ESP.getFreeHeap(),
                sound.present() ? "OK" : "FEHLT",
                static_cast<unsigned long>(sound.sentCount()),
                static_cast<unsigned long>(sound.completedCount()));

  Serial.print("[WATCHDOG]");
  for (uint8_t i = 0; i < WatchdogService::Count; i++) {
    const auto module = static_cast<WatchdogService::Module>(i);
    const auto& state = watchdog.state(module);
    Serial.printf(" %s=%s/%luus(max%lu)/err%lu",
                  WatchdogService::name(module),
                  watchdog.healthy(module) ? moduleStateName(state.status.state) : "TIMEOUT/FEHLER",
                  static_cast<unsigned long>(state.lastDurationUs),
                  static_cast<unsigned long>(state.maxDurationUs),
                  static_cast<unsigned long>(state.errorCount));
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println();
  Serial.println("============================================================");
  Serial.printf("Unterbrechungszaehler modular %s\n", UicConfig::APP_VERSION);
  Serial.println("============================================================");

  watchdog.begin();
  led.begin();
  storage.begin();
  rtc.begin();
  timeService.begin(&rtc);
  sound.begin();
  counter.begin(&storage, &timeService, &led, &sound);
  autark.begin(&storage, &timeService, &led, &sound);
  display.begin(&rtc, &timeService, &storage, &network);
  input.begin(&counter, &autark, &led, &display);

  analytics.begin(&storage);
  const uint32_t analyticsStarted = millis();
  const bool analyticsReady = analytics.warmCurrent();
  analyticsReadyAtBoot = analyticsReady;
  Serial.printf("[ANALYTIK] Vorbereitet=%s in %lu ms\n",
                analyticsReady ? "JA" : "NEIN",
                static_cast<unsigned long>(millis() - analyticsStarted));

  web.begin(&storage, &timeService, &network, &counter, &autark, &rtc, &display, &analytics, &sound, &watchdog);

  if (input.autarkSwitchOn() && storage.autarkReady()) {
    autark.enter();
    previousAutarkMode = false;
    applyModeTransition();
  } else {
    previousAutarkMode = false;
    network.begin();
    display.showBootStatus(false);
  }

  watchdog.heartbeat(WatchdogService::Storage, storage.recentReady());
  if (rtc.present()) watchdog.heartbeat(WatchdogService::Rtc, rtc.communicationOk());
  watchdog.heartbeat(WatchdogService::Analytics, analyticsReadyAtBoot);
  updateModuleStatuses();

  if (storage.recentReady()) led.signalStored();
}

void loop() {
  watchdog.beginModule(WatchdogService::MainLoop);

  watchdog.beginModule(WatchdogService::Input);
  input.tick();
  watchdog.endModule(WatchdogService::Input);

  watchdog.beginModule(WatchdogService::Mode);
  applyModeTransition();
  watchdog.endModule(WatchdogService::Mode);

  watchdog.beginModule(WatchdogService::Time);
  timeService.tick();
  watchdog.endModule(WatchdogService::Time);

  if (rtc.checkDue()) {
    watchdog.beginModule(WatchdogService::Rtc);
    const bool rtcOk = rtc.tick();
    watchdog.endModule(WatchdogService::Rtc, rtcOk);
  }

  watchdog.beginModule(WatchdogService::Autark);
  autark.tick();
  watchdog.endModule(WatchdogService::Autark);

  // Sound wird bewusst vor Display und Web bedient. Ein gespeichertes Ereignis
  // startet den Ton bereits im Counter-/Autark-Service; tick() verarbeitet nur
  // noch UART-Antworten und Wiedergabestatus.
  watchdog.beginModule(WatchdogService::Sound);
  sound.tick();
  watchdog.endModule(WatchdogService::Sound);

  watchdog.beginModule(WatchdogService::Display);
  display.tick();
  watchdog.endModule(WatchdogService::Display);

  watchdog.beginModule(WatchdogService::Led);
  led.tick();
  watchdog.endModule(WatchdogService::Led);

  const bool normalMode = !autark.active();

  watchdog.beginModule(WatchdogService::Network);
  network.tick(normalMode);
  watchdog.endModule(WatchdogService::Network);

  watchdog.beginModule(WatchdogService::Web);
  web.tick(normalMode);
  watchdog.endModule(WatchdogService::Web);

  watchdog.beginModule(WatchdogService::Diagnostics);
  updateModuleStatuses();
  printDiagnostics();
  watchdog.endModule(WatchdogService::Diagnostics);

  watchdog.endModule(WatchdogService::MainLoop);
  updateModuleStatuses();
  watchdog.feed();

  if (autark.active() && !display.active() &&
      digitalRead(UicConfig::BUTTON_PIN) == HIGH &&
      digitalRead(UicConfig::AUTARK_PIN) == LOW) {
    esp_light_sleep_start();
  }

  delay(1);
}
