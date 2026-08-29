#pragma once

#include <Arduino.h>

class RtcService;
class TimeService;

class DisplayService {
public:
  void begin(RtcService* rtc, TimeService* time);
  void tick();
  void showBootStatus(bool autarkMode);
  bool showTest(bool autarkMode);
  void off();

  bool present() const { return present_; }
  bool active() const { return active_; }
  uint8_t address() const { return address_; }

private:
  bool probe(uint8_t address);
  bool command(uint8_t commandValue);
  bool initializeController();
  void clear();
  void setPage(uint8_t page);
  void writeData(const uint8_t* data, size_t length);
  void glyph(char value, uint8_t out[5]);
  void text(uint8_t page, const String& value);
  void render(bool autarkMode, bool testMode);

  RtcService* rtc_ = nullptr;
  TimeService* time_ = nullptr;
  bool present_ = false;
  bool active_ = false;
  uint8_t address_ = 0;
  uint32_t offAt_ = 0;
};
