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
uint32_t lastCounterActionSequence = 0;
uint32_t lastAutarkSession = 0;
uint32_t lastAutarkEvents = 0;
bool analyticsReadyAtBoot = false;
char rtcStatusDetail[96] = "rtc_unknown";

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
    lastAutarkSession = autark.sessionId();
    lastAutarkEvents = autark.sessionEvents();
    Serial.printf("[MODUS] Autark EIN | Session %lu\n", static_cast<unsigned long>(autark.sessionId()));
  } else {
    disableAutarkWakeup();
    setCpuFrequencyMhz(240);
    network.begin();
    display.showBootStatus(false);
    Serial.println("[MODUS] Autark AUS | Netzwerk wird gestartet");
  }
}

void handleSoundTrigger() {
  if (autark.active()) {
    if (lastAutarkSession != autark.sessionId()) {
      lastAutarkSession = autark.sessionId();
      lastAutarkEvents = autark.sessionEvents();
    }
    const uint32_t nowEvents = autark.sessionEvents();
    if (nowEvents > lastAutarkEvents) sound.requestPlay();
    lastAutarkEvents = nowEvents;
    return;
  }

  if (counter.actionSequence() == lastCounterActionSequence) return;
  lastCounterActionSequence = counter.actionSequence();
  if (counter.actionKind() == 1) sound.requestPlay();
}

void updateRtcStatusDetail() {
  if (!rtc.present()) {
    snprintf(rtcStatusDetail, sizeof(rtcStatusDetail), "optional");
    return;
  }
  const float temp = rtc.temperatureC();
  snprintf(rtcStatusDetail,
           sizeof(rtcStatusDetail),
           "comm=%s;valid=%s;osf=%s;temp=%.2f;diff=%ld;age=%lu",
           rtc.communicationOk() ? "ok" : "error",
           rtc.timeValid() ? "yes" : "no",
           rtc.oscillatorStopFlag() ? "set" : "clear",
           isnan(temp) ? -999.0f : temp,
           static_cast<long>(rtc.systemDifferenceSeconds()),
           static_cast<unsigned long>(rtc.lastCheckAgeMs()));
}

void updateModuleStatuses() {
  watchdog.setStatus(WatchdogService::MainLoop, ModuleState::Ready, "loop_ok");
  watchdog.setStatus(WatchdogService::Input, ModuleState::Ready, "gpio_ok");
  watchdog.setStatus(WatchdogService::Mode,
                     autark.active() ? ModuleState::Busy : ModuleState::Ready,
                     autark.active() ? "autark" : "normal");

  if (!storage.recentReady()) {
    watchdog.setStatus(WatchdogService::Storage, ModuleState::Error, "recent_unavailable");
  } else if (!storage.archiveReady() || !storage.autarkReady() || !storage.archiveSynchronized()) {
    watchdog.setStatus(WatchdogService::Storage, ModuleState::Degraded, "partial_storage");
  } else {
    watchdog.setStatus(WatchdogService::Storage, ModuleState::Ready, "rings_ok");
  }

  watchdog.setStatus(WatchdogService::Time,
                     timeService.valid() ? ModuleState::Ready : ModuleState::Degraded,
                     timeService.valid() ? timeSourceName(timeService.source()) : "time_invalid");

  updateRtcStatusDetail();
  if (!rtc.present()) {
    watchdog.setStatus(WatchdogService::Rtc, ModuleState::NotDetected, rtcStatusDetail);
  } else if (!rtc.communicationOk()) {
    watchdog.setStatus(WatchdogService::Rtc, ModuleState::Error, rtcStatusDetail);
  } else {
    watchdog.setStatus(WatchdogService::Rtc,
                       rtc.timeValid() ? ModuleState::Ready : ModuleState::Degraded,
                       rtcStatusDetail);
  }

  watchdog.setStatus(WatchdogService::Autark,
                     autark.active() ? ModuleState::Busy : ModuleState::Ready,
                     autark.active() ? "session_active" : "standby");

  if (display.present()) {
    watchdog.setStatus(WatchdogService::Display,
                       display.active() ? ModuleState::Busy : ModuleState::Ready,
                       display.active() ? "display_active" : "display_idle");
  } else if (display.simulationEnabled()) {
    watchdog.setStatus(WatchdogService::Display, ModuleState::Degraded, "simulation");
  } else {
    watchdog.setStatus(WatchdogService::Display, ModuleState::NotDetected, "optional");
  }

  watchdog.setStatus(WatchdogService::Led, ModuleState::Ready, "gpio_ok");

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
  watchdog.setStatus(WatchdogService::Diagnostics, ModuleState::Ready, "serial_ok");
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
  counter.begin(&storage, &timeService, &led);
  autark.begin(&storage, &timeService, &led);
  display.begin(&rtc, &timeService, &storage, &network);
  input.begin(&counter, &autark, &led, &display);
  sound.begin();

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

  lastCounterActionSequence = counter.actionSequence();
  lastAutarkSession = autark.sessionId();
  lastAutarkEvents = autark.sessionEvents();

  watchdog.heartbeat(WatchdogService::Storage, true);
  if (rtc.present()) watchdog.heartbeat(WatchdogService::Rtc, rtc.communicationOk());
  watchdog.heartbeat(WatchdogService::Analytics, true);
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

  watchdog.heartbeat(WatchdogService::Storage, true);

  watchdog.beginModule(WatchdogService::Time);
  timeService.tick();
  watchdog.endModule(WatchdogService::Time);

  if (rtc.present() && (rtc.lastCheckAt() == 0 || rtc.lastCheckAgeMs() >= UicConfig::RTC_DIAGNOSTIC_INTERVAL_MS)) {
    watchdog.beginModule(WatchdogService::Rtc);
    rtc.tick();
    watchdog.endModule(WatchdogService::Rtc, rtc.communicationOk());
  }

  watchdog.beginModule(WatchdogService::Autark);
  autark.tick();
  watchdog.endModule(WatchdogService::Autark);

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

  watchdog.heartbeat(WatchdogService::Analytics, true);

  watchdog.beginModule(WatchdogService::Sound);
  handleSoundTrigger();
  sound.tick();
  watchdog.endModule(WatchdogService::Sound);

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
