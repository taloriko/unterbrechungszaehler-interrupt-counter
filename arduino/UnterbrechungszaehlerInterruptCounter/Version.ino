// Version.ino
// Runtime version source for all UI/API/serial output.
// TODO: remove after APP_VERSION is consolidated directly in the main sketch.

static const char* FIRMWARE_VERSION = "2026-08-29-20";

class FirmwareVersionInitializer {
public:
  FirmwareVersionInitializer() {
    APP_VERSION = FIRMWARE_VERSION;
  }
};

FirmwareVersionInitializer firmwareVersionInitializer;
