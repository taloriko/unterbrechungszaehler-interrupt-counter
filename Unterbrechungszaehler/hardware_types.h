#pragma once

#include <Arduino.h>

#include "status_registry.h"

namespace HardwareTypes {

enum class FeedbackType : uint8_t {
  None,
  LocalState,
  TransportAck,
  ProtocolResponse,
  ExternalFeedback
};

inline const char *feedbackName(FeedbackType type) {
  switch (type) {
    case FeedbackType::LocalState: return "local_state";
    case FeedbackType::TransportAck: return "transport_ack";
    case FeedbackType::ProtocolResponse: return "protocol_response";
    case FeedbackType::ExternalFeedback: return "external_feedback";
    case FeedbackType::None:
    default: return "none";
  }
}

struct DateTimeValue {
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t weekday = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  bool valid = false;
};

}  // namespace HardwareTypes
