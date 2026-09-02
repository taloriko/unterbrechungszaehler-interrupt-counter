#pragma once

#include <Arduino.h>

namespace TimeTypes {

enum class Source : uint8_t {
  None,
  Ntp,
  Rtc,
  Browser,
  Relative
};

enum class Quality : uint8_t {
  None,
  Reference,
  Valid,
  Fallback,
  Relative
};

struct Sample {
  bool available = false;
  bool valid = false;
  int64_t epochMs = 0;
  uint64_t monotonicMs = 0;
};

struct Snapshot {
  bool valid = false;
  int64_t epochMs = 0;
  uint64_t monotonicMs = 0;
  Source source = Source::None;
  Quality quality = Quality::None;
};

inline const char *sourceName(Source source) {
  switch (source) {
    case Source::Ntp: return "ntp";
    case Source::Rtc: return "rtc";
    case Source::Browser: return "browser";
    case Source::Relative: return "relative";
    case Source::None:
    default: return "none";
  }
}

inline const char *qualityName(Quality quality) {
  switch (quality) {
    case Quality::Reference: return "reference";
    case Quality::Valid: return "valid";
    case Quality::Fallback: return "fallback";
    case Quality::Relative: return "relative";
    case Quality::None:
    default: return "none";
  }
}

}  // namespace TimeTypes
