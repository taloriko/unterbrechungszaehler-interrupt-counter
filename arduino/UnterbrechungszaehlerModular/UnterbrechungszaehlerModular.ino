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
#include "StorageService.h"
#include "TimeService.h"
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

bool previousAutarkMode = false;
uint32_t lastDiagnosticAt = 0;

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

void printDiagnostics() {
  if (millis() - lastDiagnosticAt < UicConfig::DIAGNOSTIC_INTERVAL_MS) return;
  lastDiagnosticAt = millis();

  Serial.printf("[STATUS] Modus=%s | WLAN=%s | Zeit=%s/%s | Events=%lu/%lu | Langzeit=%lu/%lu | Autark=%lu/%lu | Heap=%u\n",
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
                ESP.getFreeHeap());
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println();
  Serial.println("============================================================");
  Serial.printf("Unterbrechungszaehler modular %s\n", UicConfig::APP_VERSION);
  Serial.println("============================================================");

  led.begin();
  storage.begin();
  rtc.begin();
  timeService.begin(&rtc);
  counter.begin(&storage, &timeService, &led);
  autark.begin(&storage, &timeService, &led);
  display.begin(&rtc, &timeService);
  input.begin(&counter, &autark, &led, &display);
  analytics.begin(&storage);
  web.begin(&storage, &timeService, &network, &counter, &autark, &rtc, &display, &analytics);

  if (input.autarkSwitchOn() && storage.autarkReady()) {
    autark.enter();
    previousAutarkMode = false;
    applyModeTransition();
  } else {
    previousAutarkMode = false;
    network.begin();
    display.showBootStatus(false);
  }

  if (storage.recentReady()) led.signalStored();
}

void loop() {
  input.tick();
  applyModeTransition();

  timeService.tick();
  autark.tick();
  display.tick();
  led.tick();

  const bool normalMode = !autark.active();
  network.tick(normalMode);
  web.tick(normalMode);

  printDiagnostics();

  if (autark.active() && !display.active() &&
      digitalRead(UicConfig::BUTTON_PIN) == HIGH &&
      digitalRead(UicConfig::AUTARK_PIN) == LOW) {
    esp_light_sleep_start();
  }

  delay(1);
}
