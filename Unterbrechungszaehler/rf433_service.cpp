#include "rf433_service.h"

#include "interruption_service.h"
#include "interruption_types.h"
#include "rf433_cc1101.h"
#include "serial_log.h"
#include "source_registry.h"

namespace Rf433Service {
namespace {
bool started = false;
}

bool begin() {
  if (started) return Rf433Cc1101::info().ready;
  started = true;
  SourceRegistry::begin();
  return Rf433Cc1101::begin();
}

void update() {
  SourceRegistry::update();
  Rf433Cc1101::update();

  Rf433Cc1101::Frame frame;
  while (Rf433Cc1101::pollFrame(frame)) {
    uint8_t sourceId = SourceRegistry::SOURCE_ID_UNKNOWN;
    bool learned = false;
    const bool known = SourceRegistry::consumeFrame(frame.code, frame.bitCount, frame.pulseBucket, sourceId, learned);
    if (learned) {
      // The learning press is configuration, not an interruption event.
      continue;
    }
    if (!known || sourceId < SourceRegistry::SOURCE_ID_RADIO_FIRST) {
      SerialLog::infof("RF433", "Unassigned fixed-code frame | bits=%u | code=0x%08lX | pulse=%u",
                       static_cast<unsigned int>(frame.bitCount), static_cast<unsigned long>(frame.code),
                       static_cast<unsigned int>(frame.pulseBucket));
      continue;
    }

    SerialLog::infof("RF433", "Matched button | source=%u | name=%s | code=0x%08lX",
                     static_cast<unsigned int>(sourceId), SourceRegistry::sourceName(sourceId),
                     static_cast<unsigned long>(frame.code));
    InterruptionService::capture(InterruptionTypes::EventSource::Radio, sourceId);
  }
}

bool ready() { return Rf433Cc1101::info().ready; }

bool startLearn(const char *name, uint8_t targetSourceId) {
  if (!ready()) return false;
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
