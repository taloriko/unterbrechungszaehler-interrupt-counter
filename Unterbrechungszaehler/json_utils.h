#pragma once

#include <Arduino.h>

namespace JsonUtils {

void appendEscapedString(String &out, const char *value);
void appendEscapedString(String &out, const String &value);
void appendKey(String &out, const char *key);
void appendBool(String &out, bool value);
void appendUInt(String &out, uint32_t value);
void appendUInt64(String &out, uint64_t value);
void appendInt(String &out, int32_t value);
void appendInt64(String &out, int64_t value);

}  // namespace JsonUtils
