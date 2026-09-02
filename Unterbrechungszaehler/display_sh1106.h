#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace DisplaySh1106 {

bool begin();
void probe();

bool enabled();
bool detected();
bool initialized();
bool lastTransferOk();
StatusRegistry::State health();
uint32_t lastCheckMs();
const char *lastError();
HardwareTypes::FeedbackType feedbackType();

// Small capability surface. The module keeps a tiny 1 KiB monochrome frame
// buffer so project modules can later build on the same transport without an
// external display library.
bool initialize();
bool clear();
bool setPower(bool on);
bool setContrast(uint8_t contrast);
bool showBootScreen();
bool showTestScreen();
bool bootScreenActive();
bool manualTestActive();

// Lightweight drawing surface used by project-level display templates. The
// SH1106 transport/framebuffer stays owned by this module; project code never
// talks I2C directly.
void frameClear();
void drawPixel(int16_t x, int16_t y, bool on = true);
void drawHLine(int16_t x0, int16_t x1, int16_t y);
void drawVLine(int16_t x, int16_t y0, int16_t y1);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h);
void drawText(int16_t x, int16_t y, const char *value);
void drawTextScaled(int16_t x, int16_t y, const char *value, uint8_t scale);
void drawCenteredText(int16_t y, const char *value);
bool present();
bool setInverted(bool inverted);

}  // namespace DisplaySh1106
