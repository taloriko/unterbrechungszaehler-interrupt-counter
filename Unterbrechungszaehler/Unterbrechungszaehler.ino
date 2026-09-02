#include <Arduino.h>

#include "config.h"
#include "hardware_registry.h"
#include "interruption_service.h"
#include "ota_module.h"
#include "serial_log.h"
#include "time_service.h"
#include "web_server.h"
#include "wifi_module.h"

void setup() {
  SerialLog::begin(AppConfig::SERIAL_BAUD_RATE);
  SerialLog::infof("SYSTEM", "Boot start | project=%s | version=%s",
                   AppConfig::PROJECT_NAME, AppConfig::SOFTWARE_VERSION);

  HardwareRegistry::begin();

  const bool networkReadyImmediately = WifiModule::begin();
  TimeService::begin();

  // Project layer starts only after base hardware/time services exist. It may
  // use the provisional RTC source immediately while NTP continues cooperatively.
  InterruptionService::begin();

  beginWebServer();
  OtaModule::logStorageInfo();

  if (networkReadyImmediately) {
    SerialLog::success("SYSTEM", "Startup services running | network interface ready | interruption capture active");
  } else {
    SerialLog::info("SYSTEM", "Startup services running | network negotiation continues | interruption capture active offline");
  }

  WifiModule::logStatusNow();
}

void loop() {
  // Physical DI capture runs first. The project service then provides immediate
  // feedback and queues persistence before less urgent network/UI work.
  HardwareRegistry::update();
  InterruptionService::update();
  WifiModule::update();
  handleWebServer();
  TimeService::update();
  OtaModule::update();

  delay(2);
}
