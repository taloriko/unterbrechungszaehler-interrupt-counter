#include "json_utils.h"

namespace JsonUtils {

static void appendHexByte(String &out, uint8_t value) {
  static const char hex[] = "0123456789abcdef";
  out += F("\\u00");
  out += hex[(value >> 4) & 0x0F];
  out += hex[value & 0x0F];
}

void appendEscapedString(String &out, const char *value) {
  out += '"';
  if (value != nullptr) {
    const uint8_t *p = reinterpret_cast<const uint8_t *>(value);
    while (*p != 0) {
      const uint8_t c = *p++;
      switch (c) {
        case '"': out += F("\\\""); break;
        case '\\': out += F("\\\\"); break;
        case '\b': out += F("\\b"); break;
        case '\f': out += F("\\f"); break;
        case '\n': out += F("\\n"); break;
        case '\r': out += F("\\r"); break;
        case '\t': out += F("\\t"); break;
        default:
          if (c < 0x20) {
            appendHexByte(out, c);
          } else {
            // UTF-8 bytes >= 0x20 are copied verbatim. JSON is served as UTF-8.
            out += static_cast<char>(c);
          }
      }
    }
  }
  out += '"';
}

void appendEscapedString(String &out, const String &value) {
  appendEscapedString(out, value.c_str());
}

void appendKey(String &out, const char *key) {
  appendEscapedString(out, key);
  out += ':';
}

void appendBool(String &out, bool value) {
  out += value ? F("true") : F("false");
}

void appendUInt(String &out, uint32_t value) {
  out += String(value);
}

void appendUInt64(String &out, uint64_t value) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
  out += buffer;
}

void appendInt(String &out, int32_t value) {
  out += String(value);
}

void appendInt64(String &out, int64_t value) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(value));
  out += buffer;
}

}  // namespace JsonUtils
