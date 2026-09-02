#include "ota_module.h"

#include <Update.h>
#include <WebServer.h>
#include <cstdlib>

#include "config.h"
#include "json_utils.h"
#include "serial_log.h"

namespace OtaModule {
namespace {

WebServer *webServer = nullptr;
bool uploadStarted = false;
bool uploadFailed = false;
bool uploadSucceeded = false;
bool restartPending = false;
uint32_t restartAtMs = 0;
size_t expectedBytes = 0;
size_t receivedBytes = 0;
uint8_t updateErrorCode = UPDATE_ERROR_OK;
const char *failureStage = "none";
const char *failureReason = "none";
String updateErrorText;

bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(now - since) >= interval;
}

const char *reasonForUpdateError(uint8_t code) {
  switch (code) {
    case UPDATE_ERROR_WRITE: return "write";
    case UPDATE_ERROR_ERASE: return "erase";
    case UPDATE_ERROR_READ: return "read";
    case UPDATE_ERROR_SPACE: return "space";
    case UPDATE_ERROR_SIZE: return "size";
    case UPDATE_ERROR_STREAM: return "stream";
    case UPDATE_ERROR_MD5: return "checksum";
    case UPDATE_ERROR_MAGIC_BYTE: return "image";
    case UPDATE_ERROR_ACTIVATE: return "activate";
    case UPDATE_ERROR_NO_PARTITION: return "partition";
    case UPDATE_ERROR_BAD_ARGUMENT: return "argument";
    case UPDATE_ERROR_ABORT: return "aborted";
#ifdef UPDATE_ERROR_DECRYPT
    case UPDATE_ERROR_DECRYPT: return "verify";
#endif
#ifdef UPDATE_ERROR_SIGN
    case UPDATE_ERROR_SIGN: return "verify";
#endif
#ifdef UPDATE_ERROR_SHA256
    case UPDATE_ERROR_SHA256: return "checksum";
#endif
    default: return "unknown";
  }
}

void resetUploadState() {
  uploadStarted = false;
  uploadFailed = false;
  uploadSucceeded = false;
  expectedBytes = 0;
  receivedBytes = 0;
  updateErrorCode = UPDATE_ERROR_OK;
  failureStage = "none";
  failureReason = "none";
  updateErrorText = "";
}

void failUpload(const char *stage, const char *reason, uint8_t code = UPDATE_ERROR_OK, const char *detail = nullptr) {
  uploadFailed = true;
  uploadSucceeded = false;
  failureStage = stage ? stage : "unknown";
  failureReason = reason ? reason : "unknown";
  updateErrorCode = code;
  if (detail && detail[0]) updateErrorText = detail;
  else if (code != UPDATE_ERROR_OK) updateErrorText = Update.errorString();
  else updateErrorText = "Update failed";
}

size_t requestedFirmwareSize() {
  if (!webServer) return 0;
  const String raw = webServer->header("X-Firmware-Size");
  if (!raw.length()) return 0;
  char *end = nullptr;
  const unsigned long parsed = std::strtoul(raw.c_str(), &end, 10);
  if (end == raw.c_str() || (end && *end != '\0')) return 0;
  return static_cast<size_t>(parsed);
}

bool filenameLooksLikeFirmware(const String &filename) {
  if (!filename.length()) return false;
  String lower = filename;
  lower.toLowerCase();
  return lower.endsWith(".bin");
}

String buildResultJson(bool ok) {
  String out;
  out.reserve(420);
  out += '{';
  JsonUtils::appendKey(out, "ok"); JsonUtils::appendBool(out, ok); out += ',';
  JsonUtils::appendKey(out, "stage"); JsonUtils::appendEscapedString(out, ok ? "complete" : failureStage); out += ',';
  JsonUtils::appendKey(out, "reason"); JsonUtils::appendEscapedString(out, ok ? "none" : failureReason); out += ',';
  JsonUtils::appendKey(out, "errorCode"); JsonUtils::appendUInt(out, ok ? 0U : updateErrorCode); out += ',';
  JsonUtils::appendKey(out, "error"); JsonUtils::appendEscapedString(out, ok ? "" : updateErrorText); out += ',';
  JsonUtils::appendKey(out, "receivedBytes"); JsonUtils::appendUInt(out, static_cast<uint32_t>(receivedBytes)); out += ',';
  JsonUtils::appendKey(out, "expectedBytes"); JsonUtils::appendUInt(out, static_cast<uint32_t>(expectedBytes)); out += ',';
  JsonUtils::appendKey(out, "maxBytes"); JsonUtils::appendUInt(out, maxFirmwareBytes()); out += ',';
  JsonUtils::appendKey(out, "restarting"); JsonUtils::appendBool(out, ok);
  out += '}';
  return out;
}

void handleUpload() {
  if (!webServer) return;
  HTTPUpload &upload = webServer->upload();

  switch (upload.status) {
    case UPLOAD_FILE_START: {
      resetUploadState();
      expectedBytes = requestedFirmwareSize();

      if (!supported()) {
        failUpload("start", "partition", UPDATE_ERROR_NO_PARTITION, "No OTA partition available");
        SerialLog::error("OTA", "Update rejected | no OTA partition available");
        return;
      }
      if (!filenameLooksLikeFirmware(upload.filename)) {
        failUpload("start", "image", UPDATE_ERROR_BAD_ARGUMENT, "Firmware file must be a .bin image");
        SerialLog::error("OTA", "Update rejected | file is not a .bin firmware image");
        return;
      }
      if (expectedBytes == 0) {
        failUpload("start", "size", UPDATE_ERROR_SIZE, "Firmware size header is missing or invalid");
        SerialLog::error("OTA", "Update rejected | firmware size is missing or invalid");
        return;
      }
      if (expectedBytes > maxFirmwareBytes()) {
        failUpload("start", "space", UPDATE_ERROR_SPACE, "Firmware image is larger than the OTA partition");
        SerialLog::errorf("OTA", "Update rejected | image=%lu bytes | OTA slot=%lu bytes",
                          static_cast<unsigned long>(expectedBytes),
                          static_cast<unsigned long>(maxFirmwareBytes()));
        return;
      }

      if (!Update.begin(expectedBytes, U_FLASH)) {
        const uint8_t code = Update.getError();
        failUpload("start", reasonForUpdateError(code), code);
        SerialLog::errorf("OTA", "Update could not start | code=%u | %s",
                          static_cast<unsigned int>(code), updateErrorText.c_str());
        return;
      }

      uploadStarted = true;
      SerialLog::infof("OTA", "Firmware upload started | file=%s | size=%lu bytes",
                       upload.filename.c_str(), static_cast<unsigned long>(expectedBytes));
      break;
    }

    case UPLOAD_FILE_WRITE: {
      if (!uploadStarted || uploadFailed) return;
      receivedBytes += upload.currentSize;
      const size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        const uint8_t code = Update.getError();
        failUpload("write", reasonForUpdateError(code), code);
        Update.abort();
        SerialLog::errorf("OTA", "Flash write failed | received=%lu | code=%u | %s",
                          static_cast<unsigned long>(receivedBytes),
                          static_cast<unsigned int>(code), updateErrorText.c_str());
      }
      break;
    }

    case UPLOAD_FILE_END: {
      if (!uploadStarted || uploadFailed) return;
      if (receivedBytes != expectedBytes) {
        Update.abort();
        failUpload("finish", "size", UPDATE_ERROR_SIZE, "Received firmware size does not match the selected file");
        SerialLog::errorf("OTA", "Upload size mismatch | expected=%lu | received=%lu",
                          static_cast<unsigned long>(expectedBytes),
                          static_cast<unsigned long>(receivedBytes));
        return;
      }
      if (!Update.end()) {
        const uint8_t code = Update.getError();
        failUpload("finish", reasonForUpdateError(code), code);
        SerialLog::errorf("OTA", "Firmware verification/activation failed | code=%u | %s",
                          static_cast<unsigned int>(code), updateErrorText.c_str());
        return;
      }

      uploadSucceeded = true;
      uploadFailed = false;
      SerialLog::successf("OTA", "Firmware update successful | %lu bytes written | restart pending",
                          static_cast<unsigned long>(receivedBytes));
      break;
    }

    case UPLOAD_FILE_ABORTED:
      if (Update.isRunning()) Update.abort();
      failUpload("aborted", "aborted", UPDATE_ERROR_ABORT, "Firmware upload was aborted by the client or network");
      SerialLog::warningf("OTA", "Firmware upload aborted | received=%lu bytes",
                          static_cast<unsigned long>(receivedBytes));
      break;
  }
}

void handleResult() {
  if (!webServer) return;
  webServer->sendHeader("Cache-Control", "no-store");

  if (uploadSucceeded && !uploadFailed) {
    webServer->send(200, "application/json; charset=utf-8", buildResultJson(true));
    restartPending = true;
    restartAtMs = millis();
    return;
  }

  if (!uploadFailed) {
    failUpload("request", "unknown", UPDATE_ERROR_BAD_ARGUMENT, "No firmware upload was received");
  }
  webServer->send(400, "application/json; charset=utf-8", buildResultJson(false));
}

}  // namespace

void registerRoutes(WebServer &server) {
  webServer = &server;
  server.on("/api/ota", HTTP_POST, handleResult, handleUpload);
}

void update() {
  if (restartPending && elapsed(millis(), restartAtMs, AppConfig::OTA_RESTART_DELAY_MS)) {
    restartPending = false;
    SerialLog::info("OTA", "Restarting into updated firmware");
    delay(20);
    ESP.restart();
  }
}

bool supported() {
  return ESP.getFreeSketchSpace() > 0;
}

uint32_t currentFirmwareBytes() {
  return ESP.getSketchSize();
}

uint32_t maxFirmwareBytes() {
  return ESP.getFreeSketchSpace();
}

uint32_t projectHeadroomBytes() {
  const uint32_t maximum = maxFirmwareBytes();
  const uint32_t current = currentFirmwareBytes();
  return maximum > current ? maximum - current : 0;
}

void logStorageInfo() {
  const uint32_t current = currentFirmwareBytes();
  const uint32_t maximum = maxFirmwareBytes();
  const uint32_t reserve = projectHeadroomBytes();
  if (maximum == 0) {
    SerialLog::warningf("OTA", "OTA unavailable | firmware=%lu bytes | no secondary OTA app partition",
                        static_cast<unsigned long>(current));
    return;
  }
  SerialLog::infof("OTA", "Program storage | firmware=%lu bytes | OTA slot=%lu bytes | project reserve=%lu bytes",
                   static_cast<unsigned long>(current),
                   static_cast<unsigned long>(maximum),
                   static_cast<unsigned long>(reserve));
}

}  // namespace OtaModule
