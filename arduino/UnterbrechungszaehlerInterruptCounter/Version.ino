// Version.ino
// Single runtime version source for all UI/API/serial output.

static const char* FIRMWARE_VERSION = "2026-08-29-14";

class FirmwareVersionInitializer {
public:
  FirmwareVersionInitializer() {
    APP_VERSION = FIRMWARE_VERSION;
  }
};

FirmwareVersionInitializer firmwareVersionInitializer;
