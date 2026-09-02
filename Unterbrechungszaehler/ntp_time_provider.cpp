#include "ntp_time_provider.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <cstring>

#include "config.h"
#include "serial_log.h"
#include "time_utils.h"
#include "wifi_module.h"

namespace NtpTimeProvider {
namespace {

constexpr char PREF_NAMESPACE[] = "espui-time";
constexpr char PREF_NTP_SERVER[] = "ntp";
constexpr uint16_t NTP_PORT = 123;
constexpr uint64_t NTP_UNIX_EPOCH_OFFSET = 2208988800ULL;
constexpr uint64_t NTP_ERA_SECONDS = 4294967296ULL;
constexpr size_t NTP_PACKET_SIZE = 48;

enum class ProbePhase : uint8_t { Idle, Waiting, Ready };

String configuredServer;
WiFiUDP udp;
ProbePhase phase = ProbePhase::Idle;
Result completedResult;
uint8_t requestNonce[8]{};
uint64_t startedAtMs = 0;

uint32_t readU32(const uint8_t *buffer) {
  return (static_cast<uint32_t>(buffer[0]) << 24) |
         (static_cast<uint32_t>(buffer[1]) << 16) |
         (static_cast<uint32_t>(buffer[2]) << 8) |
         static_cast<uint32_t>(buffer[3]);
}

void finish(Result result) {
  if (phase == ProbePhase::Waiting) udp.stop();
  completedResult = result;
  phase = ProbePhase::Ready;
}

void finishError(Error error) {
  Result result;
  result.ok = false;
  result.error = error;
  // Keep the resolved address for diagnostics when DNS already succeeded but
  // a later UDP/protocol step failed.
  result.resolvedIp = completedResult.resolvedIp;
  finish(result);
}

bool parseResponse(int packetSize) {
  if (packetSize < static_cast<int>(NTP_PACKET_SIZE)) {
    while (udp.available() > 0) udp.read();
    finishError(Error::InvalidResponse);
    return false;
  }

  uint8_t response[NTP_PACKET_SIZE]{};
  const int readLength = udp.read(response, sizeof(response));
  const uint64_t receivedAt = TimeUtils::monotonicMs();
  if (readLength < static_cast<int>(NTP_PACKET_SIZE)) {
    finishError(Error::InvalidResponse);
    return false;
  }

  const uint8_t leap = static_cast<uint8_t>(response[0] >> 6);
  const uint8_t version = static_cast<uint8_t>((response[0] >> 3) & 0x07);
  const uint8_t mode = static_cast<uint8_t>(response[0] & 0x07);
  const uint8_t stratum = response[1];
  if (version < 3 || version > 4 || mode != 4 || stratum == 0 || stratum > 15) {
    finishError(Error::InvalidResponse);
    return false;
  }
  if (leap == 3) {
    finishError(Error::ServerUnsynchronized);
    return false;
  }
  for (uint8_t i = 0; i < 8; ++i) {
    if (response[24 + i] != requestNonce[i]) {
      finishError(Error::InvalidResponse);
      return false;
    }
  }

  const uint32_t seconds32 = readU32(response + 40);
  const uint32_t fraction32 = readU32(response + 44);
  uint64_t ntpSeconds = seconds32;
  if (ntpSeconds < NTP_UNIX_EPOCH_OFFSET) ntpSeconds += NTP_ERA_SECONDS;
  if (ntpSeconds < NTP_UNIX_EPOCH_OFFSET) {
    finishError(Error::ImplausibleTime);
    return false;
  }

  const uint64_t unixSeconds = ntpSeconds - NTP_UNIX_EPOCH_OFFSET;
  const uint64_t fractionMs = (static_cast<uint64_t>(fraction32) * 1000ULL) >> 32;
  const uint32_t rttMs = static_cast<uint32_t>(receivedAt - startedAtMs);
  const int64_t serverTransmitMs = static_cast<int64_t>(unixSeconds * 1000ULL + fractionMs);
  const int64_t arrivalEpochMs = serverTransmitMs + static_cast<int64_t>(rttMs / 2U);
  if (!TimeUtils::isPlausibleEpochMs(arrivalEpochMs, AppConfig::TIME_MIN_VALID_YEAR, AppConfig::TIME_MAX_VALID_YEAR)) {
    finishError(Error::ImplausibleTime);
    return false;
  }

  Result result;
  result.ok = true;
  result.error = Error::None;
  result.rttMs = rttMs;
  result.sample.available = true;
  result.sample.valid = true;
  result.sample.epochMs = arrivalEpochMs;
  result.sample.monotonicMs = receivedAt;
  // resolvedIp is filled when the request is started and retained in
  // completedResult until this successful result replaces it below.
  result.resolvedIp = completedResult.resolvedIp;
  finish(result);
  return true;
}

}  // namespace

void begin() {
  configuredServer = AppConfig::DEFAULT_NTP_SERVER;
  Preferences preferences;
  if (!preferences.begin(PREF_NAMESPACE, true)) {
    SerialLog::warning("TIME", "NTP preference store unavailable; using default server");
    return;
  }
  const String stored = preferences.getString(PREF_NTP_SERVER, "");
  preferences.end();
  if (stored.length() && isValidServerName(stored.c_str())) configuredServer = stored;
}

const String &server() {
  return configuredServer;
}

bool isValidServerName(const char *serverName) {
  if (!serverName) return false;
  const size_t length = std::strlen(serverName);
  if (length == 0 || length > AppConfig::NTP_SERVER_MAX_LENGTH) return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = serverName[i];
    if (c <= 0x20 || c == '/' || c == '\\' || c == ':' || c == '?' || c == '#') return false;
  }
  return true;
}

bool startProbe() {
  return startProbe(configuredServer.c_str());
}

bool startProbe(const char *serverName) {
  if (phase != ProbePhase::Idle) return false;
  completedResult = Result{};

  if (!WifiModule::stationConnected()) {
    finishError(Error::NoNetwork);
    return true;
  }
  if (!isValidServerName(serverName)) {
    finishError(Error::EmptyServer);
    return true;
  }

  IPAddress address;
  // Arduino-ESP32 exposes hostByName() synchronously. This is intentionally
  // isolated here and documented as the remaining core-dependent blocking
  // point; the NTP response wait below never blocks the main loop.
  if (!WiFi.hostByName(serverName, address)) {
    finishError(Error::Dns);
    return true;
  }
  completedResult.resolvedIp = address.toString();

  udp.stop();
  if (!udp.begin(AppConfig::NTP_LOCAL_UDP_PORT)) {
    finishError(Error::Socket);
    return true;
  }

  uint8_t packet[NTP_PACKET_SIZE]{};
  packet[0] = 0x23;  // LI=0, VN=4, Mode=3 (client)
  packet[1] = 0;
  packet[2] = 6;
  packet[3] = 0xEC;

  const uint64_t nonce = (TimeUtils::monotonicMs() << 16) ^ static_cast<uint64_t>(micros());
  for (uint8_t i = 0; i < 8; ++i) {
    requestNonce[i] = static_cast<uint8_t>(nonce >> ((7U - i) * 8U));
    packet[40 + i] = requestNonce[i];
  }

  startedAtMs = TimeUtils::monotonicMs();
  if (!udp.beginPacket(address, NTP_PORT) || udp.write(packet, sizeof(packet)) != sizeof(packet) || !udp.endPacket()) {
    udp.stop();
    finishError(Error::Send);
    return true;
  }

  phase = ProbePhase::Waiting;
  return true;
}

void update() {
  if (phase != ProbePhase::Waiting) return;

  const int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    parseResponse(packetSize);
    return;
  }

  if (TimeUtils::monotonicMs() - startedAtMs >= AppConfig::NTP_RESPONSE_TIMEOUT_MS) {
    finishError(Error::Timeout);
  }
}

bool busy() {
  return phase == ProbePhase::Waiting;
}

bool resultReady() {
  return phase == ProbePhase::Ready;
}

bool takeResult(Result &result) {
  if (phase != ProbePhase::Ready) return false;
  result = completedResult;
  phase = ProbePhase::Idle;
  completedResult = Result{};
  return true;
}

bool saveServer(const char *serverName) {
  if (!isValidServerName(serverName)) return false;
  const String candidate(serverName);
  if (candidate == configuredServer) return true;

  Preferences preferences;
  if (!preferences.begin(PREF_NAMESPACE, false)) return false;
  const size_t written = preferences.putString(PREF_NTP_SERVER, candidate);
  preferences.end();
  if (written == 0) return false;
  configuredServer = candidate;
  return true;
}

const char *errorName(Error error) {
  switch (error) {
    case Error::None: return "none";
    case Error::NoNetwork: return "no_network";
    case Error::EmptyServer: return "invalid_server";
    case Error::Dns: return "dns_error";
    case Error::Socket: return "socket_error";
    case Error::Send: return "send_error";
    case Error::Timeout: return "timeout";
    case Error::InvalidResponse: return "invalid_response";
    case Error::ServerUnsynchronized: return "server_unsynchronized";
    case Error::ImplausibleTime: return "implausible_time";
    default: return "unknown";
  }
}

}  // namespace NtpTimeProvider
