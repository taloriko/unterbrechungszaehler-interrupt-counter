#include "rf433_service.h"

#include "interruption_service.h"
#include "interruption_types.h"
#include "rf433_cc1101.h"
#include "serial_log.h"
#include "source_registry.h"

namespace Rf433Service {
namespace {
bool started = false;

Rf433Cc1101::Protocol driverProtocol(ProjectPreferences::RadioMode mode) {
  return mode == ProjectPreferences::RadioMode::SomfyRts
             ? Rf433Cc1101::Protocol::SomfyRts
             : Rf433Cc1101::Protocol::FixedOok;
}

SourceRegistry::RadioProtocol registryProtocol(Rf433Cc1101::Protocol protocol) {
  return protocol == Rf433Cc1101::Protocol::SomfyRts
             ? SourceRegistry::RadioProtocol::SomfyRts
             : SourceRegistry::RadioProtocol::FixedOok;
}
}  // namespace

bool begin() {
  if (started) return Rf433Cc1101::info().ready;
  started = true;
  SourceRegistry::begin();
  if (!Rf433Cc1101::begin()) return false;
  if (!Rf433Cc1101::setOperatingProtocol(driverProtocol(ProjectPreferences::radioMode()))) {
    SerialLog::error("RF433", "Persisted RF mode could not be applied");
    return false;
  }
  return true;
}

void update() {
  SourceRegistry::update();
  Rf433Cc1101::update();

  Rf433Cc1101::Frame frame;
  while (Rf433Cc1101::pollFrame(frame)) {
    if (frame.diagnostic) {
      SerialLog::infof("RF433", "Diagnostic frame consumed without interruption | protocol=%s | bits=%u | code=0x%08lX",
                       Rf433Cc1101::protocolName(frame.protocol), static_cast<unsigned int>(frame.bitCount),
                       static_cast<unsigned long>(frame.code));
      continue;
    }

    uint8_t sourceId = SourceRegistry::SOURCE_ID_UNKNOWN;
    bool learned = false;
    const SourceRegistry::RadioProtocol protocol = registryProtocol(frame.protocol);
    const bool known = SourceRegistry::consumeFrame(protocol, frame.code, frame.bitCount, frame.pulseBucket, sourceId, learned);
    if (learned) {
      // The learning press is configuration, not an interruption event.
      continue;
    }
    if (!known || sourceId < SourceRegistry::SOURCE_ID_RADIO_FIRST) {
      SerialLog::infof("RF433", "Unassigned radio frame | protocol=%s | bits=%u | code=0x%08lX",
                       SourceRegistry::radioProtocolName(protocol), static_cast<unsigned int>(frame.bitCount),
                       static_cast<unsigned long>(frame.code));
      continue;
    }

    SerialLog::infof("RF433", "Matched button | source=%u | name=%s | protocol=%s | code=0x%08lX",
                     static_cast<unsigned int>(sourceId), SourceRegistry::sourceName(sourceId),
                     SourceRegistry::radioProtocolName(protocol), static_cast<unsigned long>(frame.code));
    InterruptionService::capture(InterruptionTypes::EventSource::Radio, sourceId);
  }
}

bool ready() { return Rf433Cc1101::info().ready; }

bool setOperatingMode(ProjectPreferences::RadioMode mode) {
  if (!ready() || SourceRegistry::learnState().active) return false;
  const ProjectPreferences::RadioMode previous = ProjectPreferences::radioMode();
  if (mode == previous) return true;

  if (Rf433Cc1101::receiveTestActive()) Rf433Cc1101::cancelReceiveTest();
  if (!Rf433Cc1101::setOperatingProtocol(driverProtocol(mode))) return false;

  // Hardware first, NVS second. If persistence fails, restore the previously
  // active receiver mode so runtime and stored configuration cannot diverge.
  if (!ProjectPreferences::setRadioMode(mode)) {
    Rf433Cc1101::setOperatingProtocol(driverProtocol(previous));
    return false;
  }
  return true;
}

bool startLearn(const char *name, uint8_t targetSourceId) {
  if (!ready()) return false;
  if (Rf433Cc1101::receiveTestActive()) Rf433Cc1101::cancelReceiveTest();
  return SourceRegistry::startLearn(name, targetSourceId);
}

void cancelLearn() { SourceRegistry::cancelLearn(); }

bool renameSource(uint8_t sourceId, const char *name) {
  return SourceRegistry::renameSource(sourceId, name);
}

bool unbindSource(uint8_t sourceId) {
  return SourceRegistry::unbindSource(sourceId);
}

}  // namespace Rf433Service
