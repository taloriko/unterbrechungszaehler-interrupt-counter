#include <Arduino.h>
#include <cstring>

#include "config.h"
#include "hardware_registry.h"
#include "audio_dy_sv17f.h"
#include "display_sh1106.h"
#include "project_preferences.h"
#include "interruption_service.h"
#include "work_cycle.h"
#include "ota_module.h"
#include "serial_log.h"
#include "time_service.h"
#include "web_server.h"
#include "wifi_module.h"


namespace {
const char *bootStatusForLanguage(const char *language) {
  if (!language) return "STARTING";
  if (strcmp(language, "de") == 0) return "START";
  if (strcmp(language, "fr") == 0) return "DEMARRAGE";
  if (strcmp(language, "it") == 0) return "AVVIO";
  if (strncmp(language, "swg", 3) == 0) return "START";
  return "STARTING";
}
}  // namespace

void setup() {
  SerialLog::begin(AppConfig::SERIAL_BAUD_RATE);
  SerialLog::infof("SYSTEM", "Boot start | project=%s | version=%s",
                   AppConfig::PROJECT_NAME, AppConfig::SOFTWARE_VERSION);

  ProjectPreferences::begin();
  DisplaySh1106::setRotation180(ProjectPreferences::displayRotation180());
  DisplaySh1106::setBootStatusText(bootStatusForLanguage(ProjectPreferences::language()));
  AudioDySv17f::configureVolumePercent(ProjectPreferences::soundVolumePercent());

  HardwareRegistry::begin();

  const bool networkReadyImmediately = WifiModule::begin();
  TimeService::begin();

  // The interruption service remains the single owner of interruption storage
  // and analytics. WorkCycle sits in front of the physical DI and classifies
  // start / pending last press / explicit or automatic end.
  InterruptionService::begin();
  WorkCycle::begin();

  beginWebServer();
  OtaModule::logStorageInfo();

  if (networkReadyImmediately) {
    SerialLog::success("SYSTEM", "Startup services running | network interface ready | work-cycle capture active");
  } else {
    SerialLog::info("SYSTEM", "Startup services running | network negotiation continues | work-cycle capture active offline");
  }

  WifiModule::logStatusNow();
}

void loop() {
  // GPIO still runs first. During the explicit 10-second goodbye overlay the
  // work-cycle manager exclusively owns the local display/input path, matching
  // the existing anti-spam lock semantics. Normal interruption UI servicing is
  // resumed immediately after the overlay ends.
  HardwareRegistry::update();
  if (!WorkCycle::exclusiveGoodbyeActive()) {
    InterruptionService::update();
  }
  WorkCycle::update();
  WifiModule::update();
  handleWebServer();
  TimeService::update();
  OtaModule::update();

  delay(2);
}
