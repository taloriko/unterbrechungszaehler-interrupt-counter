#pragma once

#include <Arduino.h>

namespace Rf433Cc1101 {

struct Frame {
  uint32_t code = 0;
  uint8_t bitCount = 0;
  uint8_t pulseBucket = 0;  // average short pulse in ~25 us units
  uint8_t repeats = 0;
};

struct Info {
  bool initialized = false;
  bool ready = false;
  uint8_t partNumber = 0xFF;
  uint8_t version = 0xFF;
  uint32_t decodedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint32_t overflowFrames = 0;
  Frame lastFrame{};
  const char *error = "none";
};

bool begin();
void update();
bool pollFrame(Frame &frameOut);
const Info &info();

}  // namespace Rf433Cc1101
