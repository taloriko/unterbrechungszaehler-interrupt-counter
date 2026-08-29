#pragma once

#include <Arduino.h>
#include <Preferences.h>

class RtcService;
class TimeService;

class DisplayService {
public:
  void begin(RtcService* rtc, TimeService* time);
  void tick();
  void showBootStatus(bool autarkMode);
  bool showTest(bool autarkMode);
  void notifyActivity(bool autarkMode);
  void off();

  bool present() const { return present_; }
  bool active() const { return active_; }
  bool dimmed() const { return dimmed_; }
  uint8_t address() const { return address_; }
  uint8_t brightness() const { return brightness_; }
  uint16_t dimAfterSeconds() const { return dimAfterSeconds_; }

  bool setSettings(uint8_t brightness, uint16_t dimAfterSeconds);

private:
  bool probe(uint8_t address);
  bool command(uint8_t commandValue);
  bool initializeController();
  bool setContrast(uint8_t value);
  void loadSettings();
  void clear();
  void setPage(uint8_t page);
  void writeData(const uint8_t* data, size_t length);
  void glyph(char value, uint8_t out[5]);
  void text(uint8_t page, const String& value);
  void render(bool autarkMode, bool testMode);
  void armActivityTimers(bool autarkMode);

  RtcService* rtc_ = nullptr;
  TimeService* time_ = nullptr;
  Preferences preferences_;
  bool present_ = false;
  bool active_ = false;
  bool dimmed_ = false;
  bool currentAutarkMode_ = false;
  uint8_t address_ = 0;
  uint8_t brightness_ = 127;
  uint16_t dimAfterSeconds_ = 60;
  uint32_t dimAt_ = 0;
  uint32_t offAt_ = 0;
};
