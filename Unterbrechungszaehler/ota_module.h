#pragma once

#include <Arduino.h>

class WebServer;

// Handles browser-based application firmware updates. Only the inactive OTA
// application partition is written; NVS/preferences are intentionally untouched.
namespace OtaModule {

void registerRoutes(WebServer &server);
void update();

bool supported();
uint32_t currentFirmwareBytes();
uint32_t maxFirmwareBytes();
uint32_t projectHeadroomBytes();
void logStorageInfo();

}  // namespace OtaModule
