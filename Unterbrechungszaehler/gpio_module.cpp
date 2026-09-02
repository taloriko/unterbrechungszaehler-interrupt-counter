#include "gpio_module.h"

#include <cstring>

#include "serial_log.h"
#include "status_registry.h"

namespace GpioModule {
namespace {

struct RuntimeChannel {
  bool logicalState = false;
  bool candidateState = false;
  uint32_t candidateSinceMs = 0;
  bool initialized = false;
  bool feedbackPending = false;
  bool feedbackExpected = false;
  uint32_t feedbackDueMs = 0;
  bool feedbackMismatch = false;
  volatile bool interruptLatched = false;
  uint32_t lastInterruptAcceptedMs = 0;
};

RuntimeChannel runtime[HardwareConfig::GPIO_CHANNEL_COUNT];
InputChangedCallback callbacks[HardwareConfig::GPIO_EVENT_CALLBACK_CAPACITY]{};
StatusRegistry::State moduleHealth = StatusRegistry::State::Unknown;
uint32_t checkedAtMs = 0;
uint32_t lastScanMs = 0;
const char *errorText = "";

bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(now - since) >= interval;
}

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

#ifndef ARDUINO_ISR_ATTR
#define ARDUINO_ISR_ATTR
#endif

void ARDUINO_ISR_ATTR latchInputEdge(void *arg) {
  auto *channel = static_cast<RuntimeChannel *>(arg);
  if (channel) channel->interruptLatched = true;
}

bool physicalToLogical(const HardwareConfig::GpioChannelConfig &config, int physical) {
  const bool high = physical == HIGH;
  return config.activeHigh ? high : !high;
}

bool feedbackPhysicalToLogical(const HardwareConfig::GpioChannelConfig &config, int physical) {
  const bool high = physical == HIGH;
  return config.feedbackActiveHigh ? high : !high;
}

uint8_t gpioNumber(int8_t pin) {
  return static_cast<uint8_t>(pin);
}

uint8_t logicalToPhysical(const HardwareConfig::GpioChannelConfig &config, bool logical) {
  const bool high = config.activeHigh ? logical : !logical;
  return static_cast<uint8_t>(high ? HIGH : LOW);
}

uint8_t pinModeForPull(HardwareConfig::PullMode pull) {
  switch (pull) {
    case HardwareConfig::PullMode::Up: return INPUT_PULLUP;
    case HardwareConfig::PullMode::Down: return INPUT_PULLDOWN;
    case HardwareConfig::PullMode::None:
    default: return INPUT;
  }
}

uint8_t pinModeFor(const HardwareConfig::GpioChannelConfig &config) {
  if (config.direction == HardwareConfig::GpioDirection::Output) return OUTPUT;
  return pinModeForPull(config.pull);
}

int findChannel(const char *id, HardwareConfig::GpioDirection direction) {
  if (!id) return -1;
  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    const auto &config = HardwareConfig::GPIO_CHANNELS[i];
    if (!config.enabled || config.direction != direction) continue;
    if (std::strcmp(config.id, id) == 0) return static_cast<int>(i);
  }
  return -1;
}

void notifyInputChanged(const char *channelId, bool state) {
  for (auto callback : callbacks) {
    if (callback) callback(channelId, state);
  }
}

void setHealth(StatusRegistry::State state, const char *message = "") {
  moduleHealth = state;
  errorText = message ? message : "";
  StatusRegistry::setState("gpio", state);
}

void refreshOutputHealth() {
  bool anyPending = false;
  bool anyMismatch = false;
  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    const auto &config = HardwareConfig::GPIO_CHANNELS[i];
    if (!config.enabled || config.direction != HardwareConfig::GpioDirection::Output || config.feedbackPin < 0) continue;
    anyPending = anyPending || runtime[i].feedbackPending;
    anyMismatch = anyMismatch || runtime[i].feedbackMismatch;
  }

  if (anyMismatch) setHealth(StatusRegistry::State::Warning, "output feedback mismatch");
  else if (anyPending) setHealth(StatusRegistry::State::Busy);
  else setHealth(StatusRegistry::State::Ok);
}

bool verifyFeedback(size_t index) {
  const auto &config = HardwareConfig::GPIO_CHANNELS[index];
  RuntimeChannel &channel = runtime[index];
  if (config.feedbackPin < 0) return true;

  const bool feedback = feedbackPhysicalToLogical(config, digitalRead(gpioNumber(config.feedbackPin)));
  channel.feedbackMismatch = feedback != channel.feedbackExpected;
  channel.feedbackPending = false;
  if (channel.feedbackMismatch) {
    SerialLog::warningf("GPIO", "Output feedback mismatch | %s | GPIO%d | expected=%s | feedback=%s",
                        config.id, config.pin, channel.feedbackExpected ? "ON" : "OFF", feedback ? "ON" : "OFF");
  } else {
    SerialLog::infof("GPIO", "Output feedback confirmed | %s | state=%s",
                     config.id, feedback ? "ON" : "OFF");
  }
  refreshOutputHealth();
  return !channel.feedbackMismatch;
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("gpio", "status.gpio", "io", HardwareConfig::ENABLE_GPIO);
  if (!HardwareConfig::ENABLE_GPIO) {
    setHealth(StatusRegistry::State::Disabled);
    StatusRegistry::setVisible("gpio", false);
    return false;
  }

  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    const auto &config = HardwareConfig::GPIO_CHANNELS[i];
    if (!config.enabled) continue;

    if (config.direction == HardwareConfig::GpioDirection::Output) {
      // Set the output latch before enabling output mode to minimize boot glitches.
      digitalWrite(gpioNumber(config.pin), logicalToPhysical(config, config.safeBootState));
      pinMode(gpioNumber(config.pin), OUTPUT);
      runtime[i].logicalState = config.safeBootState;
      runtime[i].feedbackExpected = config.safeBootState;
      if (config.feedbackPin >= 0) pinMode(gpioNumber(config.feedbackPin), pinModeForPull(config.feedbackPull));
    } else {
      pinMode(gpioNumber(config.pin), pinModeFor(config));
      const bool initial = physicalToLogical(config, digitalRead(gpioNumber(config.pin)));
      runtime[i].logicalState = initial;
      runtime[i].candidateState = initial;
      runtime[i].candidateSinceMs = millis();
      if (config.interruptLatch) {
        attachInterruptArg(gpioNumber(config.pin), latchInputEdge, &runtime[i],
                           config.activeHigh ? RISING : FALLING);
      }
    }
    runtime[i].initialized = true;
  }

  probe();
  size_t enabledChannels = 0;
  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    if (HardwareConfig::GPIO_CHANNELS[i].enabled) ++enabledChannels;
  }
  SerialLog::successf("GPIO", "Module ready | active-channels=%u", static_cast<unsigned int>(enabledChannels));
  return true;
}

void update() {
  if (!HardwareConfig::ENABLE_GPIO) return;
  const uint32_t now = millis();

  // Important human inputs can latch their active edge in an ISR while a
  // synchronous HTTP operation temporarily owns the main loop. The ISR does
  // nothing except set one bool. All callback/project work remains here.
  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    const auto &config = HardwareConfig::GPIO_CHANNELS[i];
    RuntimeChannel &channel = runtime[i];
    if (!config.enabled || !channel.initialized ||
        config.direction != HardwareConfig::GpioDirection::Input || !config.interruptLatch) continue;

    noInterrupts();
    const bool latched = channel.interruptLatched;
    channel.interruptLatched = false;
    interrupts();
    if (!latched) continue;

    const bool outsideDebounce = channel.lastInterruptAcceptedMs == 0 ||
                                 elapsed(now, channel.lastInterruptAcceptedMs, config.debounceMs);
    if (!outsideDebounce || channel.logicalState) continue;

    channel.lastInterruptAcceptedMs = now;
    channel.logicalState = true;
    channel.candidateState = true;
    channel.candidateSinceMs = now;
    notifyInputChanged(config.id, true);
  }

  if (elapsed(now, lastScanMs, HardwareConfig::GPIO_SCAN_INTERVAL_MS)) {
    lastScanMs = now;
    for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
      const auto &config = HardwareConfig::GPIO_CHANNELS[i];
      RuntimeChannel &channel = runtime[i];
      if (!config.enabled || !channel.initialized || config.direction != HardwareConfig::GpioDirection::Input) continue;

      const bool sample = physicalToLogical(config, digitalRead(gpioNumber(config.pin)));
      if (sample != channel.candidateState) {
        channel.candidateState = sample;
        channel.candidateSinceMs = now;
        continue;
      }
      if (sample == channel.logicalState) continue;
      if (!elapsed(now, channel.candidateSinceMs, config.debounceMs)) continue;

      channel.logicalState = sample;
      SerialLog::infof("GPIO", "Input changed | %s | GPIO%d | state=%s",
                       config.id, config.pin, sample ? "ON" : "OFF");
      notifyInputChanged(config.id, sample);
    }
  }

  // Output verification is also cooperative. A project can request a physical
  // feedback delay without introducing delay() into the GPIO write path.
  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    RuntimeChannel &channel = runtime[i];
    if (!channel.feedbackPending || !deadlineReached(now, channel.feedbackDueMs)) continue;
    verifyFeedback(i);
  }
}

void probe() {
  if (!HardwareConfig::ENABLE_GPIO) {
    setHealth(StatusRegistry::State::Disabled);
    return;
  }
  checkedAtMs = millis();

  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    const auto &config = HardwareConfig::GPIO_CHANNELS[i];
    if (!config.enabled || !runtime[i].initialized || config.direction != HardwareConfig::GpioDirection::Output || config.feedbackPin < 0) continue;
    runtime[i].feedbackExpected = runtime[i].logicalState;
    runtime[i].feedbackPending = false;
    const bool feedback = feedbackPhysicalToLogical(config, digitalRead(gpioNumber(config.feedbackPin)));
    runtime[i].feedbackMismatch = feedback != runtime[i].feedbackExpected;
  }
  refreshOutputHealth();
  SerialLog::infof("GPIO", "Health check: %s | GPIO configuration active",
                   moduleHealth == StatusRegistry::State::Ok ? "OK" : StatusRegistry::stateName(moduleHealth));
}

bool enabled() { return HardwareConfig::ENABLE_GPIO; }
StatusRegistry::State health() { return moduleHealth; }
uint32_t lastCheckMs() { return checkedAtMs; }
const char *lastError() { return errorText; }
HardwareTypes::FeedbackType feedbackType() { return HardwareTypes::FeedbackType::LocalState; }

size_t channelCount() { return HardwareConfig::GPIO_CHANNEL_COUNT; }

const HardwareConfig::GpioChannelConfig *channelConfig(size_t index) {
  return index < HardwareConfig::GPIO_CHANNEL_COUNT ? &HardwareConfig::GPIO_CHANNELS[index] : nullptr;
}

bool channelState(size_t index, bool &logicalState) {
  if (index >= HardwareConfig::GPIO_CHANNEL_COUNT) return false;
  const auto &config = HardwareConfig::GPIO_CHANNELS[index];
  if (!config.enabled || !runtime[index].initialized) return false;
  logicalState = runtime[index].logicalState;
  return true;
}

bool readInput(const char *channelId, bool &logicalState) {
  const int index = findChannel(channelId, HardwareConfig::GpioDirection::Input);
  if (index < 0) return false;
  logicalState = runtime[static_cast<size_t>(index)].logicalState;
  return true;
}

bool writeOutput(const char *channelId, bool logicalState) {
  const int index = findChannel(channelId, HardwareConfig::GpioDirection::Output);
  if (index < 0) return false;
  const size_t i = static_cast<size_t>(index);
  const auto &config = HardwareConfig::GPIO_CHANNELS[i];
  RuntimeChannel &channel = runtime[i];

  digitalWrite(gpioNumber(config.pin), logicalToPhysical(config, logicalState));
  channel.logicalState = logicalState;
  channel.feedbackExpected = logicalState;
  channel.feedbackMismatch = false;

  // With no separate feedback input, success only confirms that the ESP32 latch
  // was written. Projects can configure a feedback input for physical checking.
  if (config.feedbackPin < 0) {
    setHealth(StatusRegistry::State::Ok);
    return true;
  }

  if (config.feedbackDelayMs == 0) return verifyFeedback(i);

  channel.feedbackPending = true;
  channel.feedbackDueMs = millis() + config.feedbackDelayMs;
  refreshOutputHealth();
  return true;  // command accepted; physical verification completes in update().
}

bool readOutput(const char *channelId, bool &logicalState) {
  const int index = findChannel(channelId, HardwareConfig::GpioDirection::Output);
  if (index < 0) return false;
  logicalState = runtime[static_cast<size_t>(index)].logicalState;
  return true;
}

bool registerInputChangedCallback(InputChangedCallback callback) {
  if (!callback) return false;
  for (auto &slot : callbacks) {
    if (slot == callback) return true;
    if (!slot) {
      slot = callback;
      return true;
    }
  }
  return false;
}

}  // namespace GpioModule
