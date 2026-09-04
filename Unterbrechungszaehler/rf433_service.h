#pragma once

#include <Arduino.h>

#include "project_preferences.h"

namespace Rf433Service {

bool begin();
void update();
bool ready();

bool setOperatingMode(ProjectPreferences::RadioMode mode);

bool startLearn(const char *name, uint8_t targetSourceId = 0);
void cancelLearn();
bool renameSource(uint8_t sourceId, const char *name);
bool unbindSource(uint8_t sourceId);

}  // namespace Rf433Service
