#pragma once

#include <Arduino.h>

#include "hardware_config.h"
#include "hardware_types.h"

namespace GpioModule {

using InputChangedCallback = void (*)(const char *channelId, bool logicalState);

bool begin();
void update();
void probe();

bool enabled();
StatusRegistry::State health();
uint32_t lastCheckMs();
const char *lastError();
HardwareTypes::FeedbackType feedbackType();

size_t channelCount();
const HardwareConfig::GpioChannelConfig *channelConfig(size_t index);
bool channelState(size_t index, bool &logicalState);
bool readInput(const char *channelId, bool &logicalState);
bool writeOutput(const char *channelId, bool logicalState);
bool readOutput(const char *channelId, bool &logicalState);

bool registerInputChangedCallback(InputChangedCallback callback);

}  // namespace GpioModule
