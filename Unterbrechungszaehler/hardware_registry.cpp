#include "hardware_registry.h"

#include <cstring>

#include "audio_dy_sv17f.h"
#include "display_sh1106.h"
#include "gpio_module.h"
#include "hardware_config.h"
#include "hardware_types.h"
#include "json_utils.h"
#include "rtc_ds3231.h"
#include "rf433_cc1101.h"
#include "source_registry.h"
#include "serial_log.h"
#include "status_registry.h"

namespace HardwareRegistry {
namespace {

struct PinClaim {
  int8_t pin;
  const char *owner;
  bool output;
  bool internalPull;
};

bool pinMapValid = true;
uint32_t pinMapCheckedAtMs = 0;

bool isKnownEsp32Pin(int pin) {
  // Target is the classic ESP32 Dev Module / ESP32-WROOM-32 header layout.
  // GPIO37/38 exist in the silicon but are not exposed on the standard DevKitC
  // headers, so accepting them here would create a configuration that cannot
  // actually be wired on the declared target board.
  if (pin >= 0 && pin <= 19) return true;
  if (pin >= 21 && pin <= 23) return true;
  if (pin >= 25 && pin <= 27) return true;
  return pin == 32 || pin == 33 || pin == 34 || pin == 35 || pin == 36 || pin == 39;
}
bool isInputOnlyPin(int pin) { return pin == 34 || pin == 35 || pin == 36 || pin == 39; }
bool isFlashPin(int pin) { return pin >= 6 && pin <= 11; }
bool isSerialConsolePin(int pin) { return pin == 1 || pin == 3; }
bool isStrappingPin(int pin) { return pin == 0 || pin == 2 || pin == 4 || pin == 5 || pin == 12 || pin == 15; }

bool validatePinConfiguration() {
  PinClaim claims[24]{};
  size_t count = 0;
  bool overflow = false;
  auto add = [&](int8_t pin, const char *owner, bool output, bool internalPull = false) {
    if (pin < 0) return;
    if (count >= (sizeof(claims) / sizeof(claims[0]))) {
      overflow = true;
      return;
    }
    claims[count++] = PinClaim{pin, owner, output, internalPull};
  };

  if (HardwareConfig::ENABLE_RTC_DS3231 || HardwareConfig::ENABLE_DISPLAY_SH1106) {
    add(HardwareConfig::I2C_SDA_PIN, "I2C SDA", true);
    add(HardwareConfig::I2C_SCL_PIN, "I2C SCL", true);
  }
  if (HardwareConfig::ENABLE_AUDIO_DY_SV17F) {
    add(HardwareConfig::AUDIO_RX_PIN, "Audio RX", false);
    add(HardwareConfig::AUDIO_TX_PIN, "Audio TX", true);
    add(HardwareConfig::AUDIO_BUSY_PIN, "Audio BUSY", false);
  }
  if (HardwareConfig::ENABLE_RF433_CC1101) {
    add(HardwareConfig::RF433_SCK_PIN, "RF433 SCK", true);
    add(HardwareConfig::RF433_MISO_PIN, "RF433 MISO", false);
    add(HardwareConfig::RF433_MOSI_PIN, "RF433 MOSI", true);
    add(HardwareConfig::RF433_CS_PIN, "RF433 CS", true);
    add(HardwareConfig::RF433_GDO0_PIN, "RF433 GDO0", false);
    add(HardwareConfig::RF433_GDO2_PIN, "RF433 GDO2", false);
  }
  if (HardwareConfig::ENABLE_GPIO) {
    for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
      const auto &channel = HardwareConfig::GPIO_CHANNELS[i];
      if (!channel.enabled) continue;
      const bool input = channel.direction == HardwareConfig::GpioDirection::Input;
      add(channel.pin, channel.id, !input, input && channel.pull != HardwareConfig::PullMode::None);
      if (channel.feedbackPin >= 0) {
        add(channel.feedbackPin, "GPIO feedback", false, channel.feedbackPull != HardwareConfig::PullMode::None);
      }
    }
  }

  bool valid = !overflow;
  if (overflow) SerialLog::error("HARDWARE", "Pin-map validation overflow; configuration rejected");
  for (size_t i = 0; i < count; ++i) {
    const PinClaim &claim = claims[i];
    if (!isKnownEsp32Pin(claim.pin)) {
      SerialLog::errorf("HARDWARE", "Invalid ESP32 pin assignment | GPIO%d | owner=%s", claim.pin, claim.owner);
      valid = false;
      continue;
    }
    if (isFlashPin(claim.pin) || isSerialConsolePin(claim.pin)) {
      SerialLog::errorf("HARDWARE", "Unsafe pin assignment | GPIO%d | owner=%s", claim.pin, claim.owner);
      valid = false;
    }
    if (claim.output && isInputOnlyPin(claim.pin)) {
      SerialLog::errorf("HARDWARE", "Output assigned to input-only pin | GPIO%d | owner=%s", claim.pin, claim.owner);
      valid = false;
    }
    if (claim.internalPull && isInputOnlyPin(claim.pin)) {
      SerialLog::errorf("HARDWARE", "Internal pull-up/down requested on GPIO without internal pulls | GPIO%d | owner=%s",
                        claim.pin, claim.owner);
      valid = false;
    }
    if (isStrappingPin(claim.pin)) {
      SerialLog::warningf("HARDWARE", "Boot strapping pin in use | GPIO%d | owner=%s", claim.pin, claim.owner);
    }
    for (size_t j = i + 1; j < count; ++j) {
      if (claim.pin != claims[j].pin) continue;
      SerialLog::errorf("HARDWARE", "GPIO conflict | GPIO%d | %s <-> %s", claim.pin, claim.owner, claims[j].owner);
      valid = false;
    }
  }
  if (valid) SerialLog::success("HARDWARE", "Pin map validation: OK");
  else SerialLog::error("HARDWARE", "Pin map validation failed; optional hardware initialization is blocked for safety");
  return valid;
}

void registerConfigurationErrorProviders() {
  if (HardwareConfig::ENABLE_GPIO) {
    StatusRegistry::registerProvider("gpio", "status.gpio", "io", true);
    StatusRegistry::setState("gpio", StatusRegistry::State::Error);
  }
  if (HardwareConfig::ENABLE_RTC_DS3231) {
    StatusRegistry::registerProvider("rtc", "status.rtc", "clock", true);
    StatusRegistry::setState("rtc", StatusRegistry::State::Error);
  }
  if (HardwareConfig::ENABLE_DISPLAY_SH1106) {
    StatusRegistry::registerProvider("display", "status.display", "display", true);
    StatusRegistry::setState("display", StatusRegistry::State::Error);
  }
  if (HardwareConfig::ENABLE_AUDIO_DY_SV17F) {
    StatusRegistry::registerProvider("audio", "status.audio", "audio", true);
    StatusRegistry::setState("audio", StatusRegistry::State::Error);
  }
  if (HardwareConfig::ENABLE_RF433_CC1101) {
    StatusRegistry::registerProvider("rf433", "status.rf433", "hardware", true);
    StatusRegistry::setState("rf433", StatusRegistry::State::Error);
  }
}

StatusRegistry::State effectiveHealth(StatusRegistry::State actual) {
  return pinMapValid ? actual : StatusRegistry::State::Error;
}

const char *effectiveError(const char *actual) {
  return pinMapValid ? actual : "pin configuration invalid";
}

uint32_t effectiveCheckedAt(uint32_t actual) {
  return pinMapValid ? actual : pinMapCheckedAtMs;
}

void appendInfoString(String &out, bool &first, const char *labelKey, const String &value, const char *format = nullptr) {
  if (!first) out += ',';
  first = false;
  out += '{';
  JsonUtils::appendKey(out, "labelKey"); JsonUtils::appendEscapedString(out, labelKey); out += ',';
  JsonUtils::appendKey(out, "value"); JsonUtils::appendEscapedString(out, value);
  if (format) { out += ','; JsonUtils::appendKey(out, "format"); JsonUtils::appendEscapedString(out, format); }
  out += '}';
}

void appendInfoBool(String &out, bool &first, const char *labelKey, bool value, const char *format = "bool") {
  if (!first) out += ',';
  first = false;
  out += '{';
  JsonUtils::appendKey(out, "labelKey"); JsonUtils::appendEscapedString(out, labelKey); out += ',';
  JsonUtils::appendKey(out, "value"); JsonUtils::appendBool(out, value); out += ',';
  JsonUtils::appendKey(out, "format"); JsonUtils::appendEscapedString(out, format);
  out += '}';
}

void appendInfoUInt(String &out, bool &first, const char *labelKey, uint32_t value, const char *format = nullptr) {
  if (!first) out += ',';
  first = false;
  out += '{';
  JsonUtils::appendKey(out, "labelKey"); JsonUtils::appendEscapedString(out, labelKey); out += ',';
  JsonUtils::appendKey(out, "value"); JsonUtils::appendUInt(out, value);
  if (format) { out += ','; JsonUtils::appendKey(out, "format"); JsonUtils::appendEscapedString(out, format); }
  out += '}';
}

void appendInfoFloat(String &out, bool &first, const char *labelKey, float value, const char *format) {
  if (!first) out += ',';
  first = false;
  out += '{';
  JsonUtils::appendKey(out, "labelKey"); JsonUtils::appendEscapedString(out, labelKey); out += ',';
  JsonUtils::appendKey(out, "value"); out += String(value, 2); out += ',';
  JsonUtils::appendKey(out, "format"); JsonUtils::appendEscapedString(out, format);
  out += '}';
}

void beginModule(String &out, bool &first, const char *id, const char *nameKey, const char *icon,
                 bool enabled, StatusRegistry::State health, HardwareTypes::FeedbackType feedback,
                 uint32_t checkedAt, const char *error) {
  if (!enabled) return;
  if (!first) out += ',';
  first = false;
  out += '{';
  JsonUtils::appendKey(out, "id"); JsonUtils::appendEscapedString(out, id); out += ',';
  JsonUtils::appendKey(out, "nameKey"); JsonUtils::appendEscapedString(out, nameKey); out += ',';
  JsonUtils::appendKey(out, "icon"); JsonUtils::appendEscapedString(out, icon); out += ',';
  JsonUtils::appendKey(out, "health"); JsonUtils::appendEscapedString(out, StatusRegistry::stateName(health)); out += ',';
  JsonUtils::appendKey(out, "feedback"); JsonUtils::appendEscapedString(out, HardwareTypes::feedbackName(feedback)); out += ',';
  JsonUtils::appendKey(out, "lastCheckMs"); JsonUtils::appendUInt(out, checkedAt); out += ',';
  JsonUtils::appendKey(out, "error"); JsonUtils::appendEscapedString(out, error ? error : ""); out += ',';
  JsonUtils::appendKey(out, "info"); out += '[';
}

void endModule(String &out, const char *actionId = nullptr, const char *labelKey = nullptr, const char *icon = nullptr) {
  out += ']';
  if (actionId && labelKey) {
    out += ',';
    JsonUtils::appendKey(out, "actions");
    out += "[{";
    JsonUtils::appendKey(out, "id"); JsonUtils::appendEscapedString(out, actionId); out += ',';
    JsonUtils::appendKey(out, "labelKey"); JsonUtils::appendEscapedString(out, labelKey);
    if (icon) { out += ','; JsonUtils::appendKey(out, "icon"); JsonUtils::appendEscapedString(out, icon); }
    out += "}]";
  }
  out += '}';
}

String pinList(HardwareConfig::GpioDirection direction) {
  String result;
  for (size_t i = 0; i < HardwareConfig::GPIO_CHANNEL_COUNT; ++i) {
    const auto &channel = HardwareConfig::GPIO_CHANNELS[i];
    if (!channel.enabled || channel.direction != direction) continue;
    if (result.length()) result += ", ";
    result += "GPIO";
    result += String(channel.pin);
  }
  return result;
}

String rtcTimeText() {
  const auto &dt = RtcDs3231::dateTime();
  if (!dt.valid) return String();
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u",
           dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  return String(buffer);
}

String hexByte(uint8_t value) {
  char buffer[5];
  snprintf(buffer, sizeof(buffer), "0x%02X", value);
  return String(buffer);
}

String hexDword(uint32_t value) {
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(value));
  return String(buffer);
}

}  // namespace

void begin() {
  SerialLog::info("HARDWARE", "Boot hardware check started");
  pinMapCheckedAtMs = millis();
  pinMapValid = validatePinConfiguration();
  if (!pinMapValid) {
    registerConfigurationErrorProviders();
    return;
  }
  GpioModule::begin();
  RtcDs3231::begin();
  DisplaySh1106::begin();
  AudioDySv17f::begin();
  Rf433Cc1101::begin();
  SerialLog::info("HARDWARE", "Boot hardware checks dispatched | asynchronous modules finish in loop()");
}

void update() {
  if (!pinMapValid) return;
  GpioModule::update();
  AudioDySv17f::update();
}

void probeAll() {
  if (!pinMapValid) {
    SerialLog::error("HARDWARE", "Manual check rejected | pin configuration invalid");
    return;
  }
  SerialLog::info("HARDWARE", "Manual check: all enabled modules");
  GpioModule::probe();
  RtcDs3231::probe();
  DisplaySh1106::probe();
  AudioDySv17f::probe();
  Rf433Cc1101::probe();
}

bool hasModule(const char *moduleId) {
  if (!moduleId) return false;
  if (std::strcmp(moduleId, "gpio") == 0) return GpioModule::enabled();
  if (std::strcmp(moduleId, "rtc") == 0) return RtcDs3231::enabled();
  if (std::strcmp(moduleId, "display") == 0) return DisplaySh1106::enabled();
  if (std::strcmp(moduleId, "audio") == 0) return AudioDySv17f::enabled();
  if (std::strcmp(moduleId, "rf433") == 0) return Rf433Cc1101::enabled();
  return false;
}

bool probe(const char *moduleId) {
  if (!pinMapValid || !moduleId) return false;
  if (std::strcmp(moduleId, "gpio") == 0 && GpioModule::enabled()) { GpioModule::probe(); return true; }
  if (std::strcmp(moduleId, "rtc") == 0 && RtcDs3231::enabled()) { RtcDs3231::probe(); return true; }
  if (std::strcmp(moduleId, "display") == 0 && DisplaySh1106::enabled()) { DisplaySh1106::probe(); return true; }
  if (std::strcmp(moduleId, "audio") == 0 && AudioDySv17f::enabled()) return AudioDySv17f::probe();
  if (std::strcmp(moduleId, "rf433") == 0 && Rf433Cc1101::enabled()) return Rf433Cc1101::probe();
  return false;
}

bool action(const char *moduleId, const char *actionId) {
  if (!pinMapValid || !moduleId || !actionId) return false;
  if (std::strcmp(moduleId, "display") == 0 && std::strcmp(actionId, "test") == 0 && DisplaySh1106::enabled()) {
    SerialLog::info("HARDWARE", "Manual action | display test");
    return DisplaySh1106::showTestScreen();
  }
  if (std::strcmp(moduleId, "audio") == 0 && std::strcmp(actionId, "test") == 0 && AudioDySv17f::enabled()) {
    SerialLog::info("HARDWARE", "Manual action | audio test tone");
    return AudioDySv17f::playTestTone();
  }
  if (std::strcmp(moduleId, "rf433") == 0 && std::strcmp(actionId, "test") == 0 && Rf433Cc1101::enabled()) {
    if (SourceRegistry::learnState().active) return false;
    SerialLog::info("HARDWARE", "Manual action | RF433 receive test");
    return Rf433Cc1101::startReceiveTest();
  }
  return false;
}

bool anyChecking() {
  if (!pinMapValid) return false;
  return GpioModule::health() == StatusRegistry::State::Checking ||
         RtcDs3231::health() == StatusRegistry::State::Checking ||
         DisplaySh1106::health() == StatusRegistry::State::Checking ||
         AudioDySv17f::checking() ||
         Rf433Cc1101::health() == StatusRegistry::State::Checking ||
         Rf433Cc1101::receiveTestActive();
}

void appendJson(String &out) {
  out += '{';
  JsonUtils::appendKey(out, "checking"); JsonUtils::appendBool(out, anyChecking()); out += ',';
  JsonUtils::appendKey(out, "modules"); out += '[';
  bool firstModule = true;

  if (GpioModule::enabled()) {
    beginModule(out, firstModule, "gpio", "hardware.gpio", "io", true, effectiveHealth(GpioModule::health()),
                GpioModule::feedbackType(), effectiveCheckedAt(GpioModule::lastCheckMs()), effectiveError(GpioModule::lastError()));
    bool first = true;
    appendInfoString(out, first, "hardware.info.inputs", pinList(HardwareConfig::GpioDirection::Input));
    appendInfoString(out, first, "hardware.info.outputs", pinList(HardwareConfig::GpioDirection::Output));
    endModule(out);
  }

  if (RtcDs3231::enabled()) {
    beginModule(out, firstModule, "rtc", "hardware.rtc", "clock", true, effectiveHealth(RtcDs3231::health()),
                RtcDs3231::feedbackType(), effectiveCheckedAt(RtcDs3231::lastCheckMs()), effectiveError(RtcDs3231::lastError()));
    bool first = true;
    appendInfoString(out, first, "hardware.info.model", "DS3231");
    appendInfoString(out, first, "hardware.info.transport", "I2C");
    appendInfoString(out, first, "hardware.info.address", hexByte(HardwareConfig::RTC_DS3231_ADDRESS));
    appendInfoString(out, first, "hardware.info.pins",
                     String("SDA GPIO") + String(static_cast<int>(HardwareConfig::I2C_SDA_PIN)) + ", SCL GPIO" + String(static_cast<int>(HardwareConfig::I2C_SCL_PIN)));
    const String timeText = rtcTimeText();
    if (timeText.length()) appendInfoString(out, first, "hardware.info.rtcTime", timeText);
    if (RtcDs3231::detected()) appendInfoFloat(out, first, "hardware.info.temperature", RtcDs3231::temperatureC(), "celsius");
    if (RtcDs3231::detected()) appendInfoBool(out, first, "hardware.info.osf", RtcDs3231::oscillatorStopFlag());
    endModule(out);
  }

  if (DisplaySh1106::enabled()) {
    beginModule(out, firstModule, "display", "hardware.display", "display", true, effectiveHealth(DisplaySh1106::health()),
                DisplaySh1106::feedbackType(), effectiveCheckedAt(DisplaySh1106::lastCheckMs()), effectiveError(DisplaySh1106::lastError()));
    bool first = true;
    appendInfoString(out, first, "hardware.info.model", "SH1106");
    appendInfoString(out, first, "hardware.info.transport", "I2C");
    appendInfoString(out, first, "hardware.info.address", hexByte(HardwareConfig::DISPLAY_SH1106_ADDRESS));
    appendInfoString(out, first, "hardware.info.resolution", String(HardwareConfig::DISPLAY_WIDTH) + " x " + String(HardwareConfig::DISPLAY_HEIGHT));
    appendInfoString(out, first, "hardware.info.pins",
                     String("SDA GPIO") + String(static_cast<int>(HardwareConfig::I2C_SDA_PIN)) + ", SCL GPIO" + String(static_cast<int>(HardwareConfig::I2C_SCL_PIN)));
    appendInfoBool(out, first, "hardware.info.initialized", DisplaySh1106::initialized());
    endModule(out, "test", "action.displayTest", "display");
  }

  if (AudioDySv17f::enabled()) {
    beginModule(out, firstModule, "audio", "hardware.audio", "audio", true, effectiveHealth(AudioDySv17f::health()),
                AudioDySv17f::feedbackType(), effectiveCheckedAt(AudioDySv17f::lastCheckMs()), effectiveError(AudioDySv17f::lastError()));
    bool first = true;
    appendInfoString(out, first, "hardware.info.model", "DY-SV17F");
    appendInfoString(out, first, "hardware.info.transport", "UART2 / 9600 8N1");
    appendInfoString(out, first, "hardware.info.pins",
                     String("DY TX -> GPIO") + String(static_cast<int>(HardwareConfig::AUDIO_RX_PIN)) + " (ESP RX), DY RX <- GPIO" + String(static_cast<int>(HardwareConfig::AUDIO_TX_PIN)) + " (ESP TX), BUSY -> GPIO" + String(static_cast<int>(HardwareConfig::AUDIO_BUSY_PIN)) + "/VN");
    if (AudioDySv17f::detected()) {
      appendInfoString(out, first, "hardware.info.playState", String(AudioDySv17f::playStateName()), "stateKey");
      appendInfoString(out, first, "hardware.info.onlineDevices", hexByte(AudioDySv17f::onlineDevices()));
      appendInfoUInt(out, first, "hardware.info.fileCount", AudioDySv17f::musicCount());
    }
    if (AudioDySv17f::busyKnown()) appendInfoBool(out, first, "hardware.info.busy", AudioDySv17f::busy());
    appendInfoUInt(out, first, "hardware.info.testTrack", HardwareConfig::AUDIO_TEST_TRACK);
    endModule(out, "test", "action.audioTest", "audio");
  }

  if (Rf433Cc1101::enabled()) {
    beginModule(out, firstModule, "rf433", "hardware.rf433", "hardware", true, effectiveHealth(Rf433Cc1101::health()),
                Rf433Cc1101::feedbackType(), effectiveCheckedAt(Rf433Cc1101::lastCheckMs()), effectiveError(Rf433Cc1101::lastError()));
    bool first = true;
    const auto &rf = Rf433Cc1101::info();
    appendInfoString(out, first, "hardware.info.model", "CC1101 / RF1100SE");
    appendInfoString(out, first, "hardware.info.transport", "SPI / OOK: 433.92 MHz + Somfy RTS 433.42 MHz Test");
    appendInfoString(out, first, "hardware.info.pins",
                     String("SCK GPIO") + String(static_cast<int>(HardwareConfig::RF433_SCK_PIN)) +
                     ", MISO GPIO" + String(static_cast<int>(HardwareConfig::RF433_MISO_PIN)) +
                     ", MOSI GPIO" + String(static_cast<int>(HardwareConfig::RF433_MOSI_PIN)) +
                     ", CS GPIO" + String(static_cast<int>(HardwareConfig::RF433_CS_PIN)) +
                     ", GDO0 GPIO" + String(static_cast<int>(HardwareConfig::RF433_GDO0_PIN)) +
                     ", GDO2 GPIO" + String(static_cast<int>(HardwareConfig::RF433_GDO2_PIN)));
    appendInfoUInt(out, first, "hardware.info.frequency", rf.activeFrequencyHz ? rf.activeFrequencyHz : HardwareConfig::RF433_FREQUENCY_HZ);
    appendInfoBool(out, first, "hardware.info.initialized", rf.initialized);
    appendInfoBool(out, first, "hardware.info.configVerified", rf.configVerified);
    if (rf.partNumber != 0xFFU) appendInfoString(out, first, "hardware.info.partNumber", hexByte(rf.partNumber));
    if (rf.version != 0xFFU) appendInfoString(out, first, "hardware.info.chipVersion", hexByte(rf.version));
    appendInfoUInt(out, first, "hardware.info.decodedFrames", rf.decodedFrames);
    appendInfoUInt(out, first, "hardware.info.rejectedFrames", rf.rejectedFrames);
    appendInfoUInt(out, first, "hardware.info.overflowFrames", rf.overflowFrames);
    appendInfoString(out, first, "hardware.info.rfTestResult", Rf433Cc1101::receiveTestResult(), "rfTest");
    const auto &testFrame = Rf433Cc1101::lastTestFrame();
    if (testFrame.code != 0U) {
      String frameText = String(Rf433Cc1101::protocolName(testFrame.protocol)) + " / ";
      if (testFrame.protocol == Rf433Cc1101::Protocol::SomfyRts) {
        frameText += "Addr ";
        frameText += hexDword(testFrame.code);
        frameText += " / ";
        frameText += Rf433Cc1101::somfyCommandName(testFrame.command);
        frameText += " / RC ";
        frameText += String(testFrame.rollingCode);
      } else {
        frameText += hexDword(testFrame.code) + " / " + String(testFrame.bitCount) + " bit";
      }
      appendInfoString(out, first, "hardware.info.rfTestFrame", frameText);
    }
    endModule(out, "test", "action.rf433Test", "hardware");
  }

  out += ']';
  out += '}';
}

}  // namespace HardwareRegistry
