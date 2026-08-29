#pragma once

#include <Arduino.h>
#include <Preferences.h>

class RtcService;
class TimeService;
class StorageService;
class NetworkService;

enum class DisplayLayout : uint8_t {
  Standard = 0,
  Compact = 1,
  Clock = 2
};

class DisplayService {
public:
  static constexpr uint16_t WIDTH = 128;
  static constexpr uint16_t HEIGHT = 64;
  static constexpr size_t FRAMEBUFFER_SIZE = WIDTH * HEIGHT / 8;

  void begin(RtcService* rtc, TimeService* time, StorageService* storage, NetworkService* network);
  void tick();
  void showBootStatus(bool autarkMode);
  bool showTest(bool autarkMode);
  void notifyActivity(bool autarkMode);
  void notifyEvent(bool autarkMode);
  void off();

  // present() bedeutet weiterhin: echte Hardware am I2C-Bus erkannt.
  // available() ist wahr, wenn echte Hardware vorhanden ODER die Simulation
  // bewusst aktiviert wurde.
  bool present() const { return present_; }
  bool simulationEnabled() const { return simulationEnabled_; }
  bool available() const { return present_ || simulationEnabled_; }
  bool active() const { return active_; }
  bool dimmed() const { return dimmed_; }
  uint8_t address() const { return address_; }
  uint8_t brightness() const { return brightness_; }
  uint8_t dimBrightness() const { return dimBrightness_; }
  uint16_t dimAfterSeconds() const { return dimAfterSeconds_; }
  uint32_t offAfterSeconds() const { return offAfterSeconds_; }
  bool wakeOnEvent() const { return wakeOnEvent_; }
  bool inverted() const { return inverted_; }
  bool rotation180() const { return rotation180_; }
  DisplayLayout layout() const { return layout_; }
  uint8_t effectiveBrightness() const { return dimmed_ ? dimBrightness_ : brightness_; }
  const uint8_t* framebuffer() const { return framebuffer_; }
  uint32_t frameRevision() const { return frameRevision_; }

  bool setSimulationEnabled(bool enabled);
  bool setSettings(uint8_t brightness,
                   uint8_t dimBrightness,
                   uint16_t dimAfterSeconds,
                   uint32_t offAfterSeconds,
                   bool wakeOnEvent,
                   bool inverted,
                   bool rotation180,
                   DisplayLayout layout);

private:
  enum class ScreenMode : uint8_t {
    Boot,
    Live,
    Test
  };

  bool probe(uint8_t address);
  bool command(uint8_t commandValue);
  bool initializeController();
  bool setContrast(uint8_t value);
  bool applyOrientation();
  void loadSettings();
  void flush();
  void setPage(uint8_t page);
  void writeData(const uint8_t* data, size_t length);
  void glyph(char value, uint8_t out[5]);
  void clearBuffer();
  void pixel(int16_t x, int16_t y, bool on = true);
  void drawChar(int16_t x, int16_t y, char value, uint8_t scale = 1);
  void drawText(int16_t x, int16_t y, const String& value, uint8_t scale = 1);
  void drawHLine(int16_t x, int16_t y, int16_t width);
  void renderFrame(bool autarkMode, ScreenMode mode);
  void renderBoot(bool autarkMode);
  void renderTest(bool autarkMode);
  void renderLive(bool autarkMode);
  void renderStandard(bool autarkMode);
  void renderCompact(bool autarkMode);
  void renderClock(bool autarkMode);
  void armActivityTimers(bool autarkMode);
  void wake(bool autarkMode, bool resetTimers);
  String shortDate() const;
  String timeText() const;
  String rtcText() const;
  String networkText() const;

  RtcService* rtc_ = nullptr;
  TimeService* time_ = nullptr;
  StorageService* storage_ = nullptr;
  NetworkService* network_ = nullptr;
  Preferences preferences_;

  bool present_ = false;
  bool simulationEnabled_ = false;
  bool active_ = false;
  bool dimmed_ = false;
  bool currentAutarkMode_ = false;
  bool wakeOnEvent_ = true;
  bool inverted_ = false;
  bool rotation180_ = false;
  uint8_t address_ = 0;
  uint8_t brightness_ = 127;
  uint8_t dimBrightness_ = 32;
  uint16_t dimAfterSeconds_ = 60;
  uint32_t offAfterSeconds_ = 0;
  DisplayLayout layout_ = DisplayLayout::Standard;
  ScreenMode screenMode_ = ScreenMode::Boot;

  uint32_t dimAt_ = 0;
  uint32_t offAt_ = 0;
  uint32_t bootUntil_ = 0;
  uint32_t lastFrameAt_ = 0;
  uint32_t frameRevision_ = 0;
  uint8_t framebuffer_[FRAMEBUFFER_SIZE] = {};
};
