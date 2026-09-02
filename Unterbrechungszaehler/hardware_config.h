#pragma once

#include <Arduino.h>

// Central pin/module configuration for the reusable hardware layer.
// Defaults target the classic ESP32 Dev Module / ESP32-WROOM-32 family.
namespace HardwareConfig {

constexpr bool ENABLE_GPIO = true;
constexpr bool ENABLE_RTC_DS3231 = true;
constexpr bool ENABLE_DISPLAY_SH1106 = true;
constexpr bool ENABLE_AUDIO_DY_SV17F = true;

// Shared I2C bus: DS3231 + SH1106.
constexpr int8_t I2C_SDA_PIN = 21;
constexpr int8_t I2C_SCL_PIN = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 400000;

// DS3231 has a fixed 7-bit I2C address.
constexpr uint8_t RTC_DS3231_ADDRESS = 0x68;

// Current display target: common 128x64 I2C SH1106 module.
// 0x3C is the usual address; change to 0x3D if the module is strapped that way.
constexpr uint8_t DISPLAY_SH1106_ADDRESS = 0x3C;
constexpr uint16_t DISPLAY_WIDTH = 128;
constexpr uint16_t DISPLAY_HEIGHT = 64;
constexpr uint8_t DISPLAY_COLUMN_OFFSET = 2;
constexpr bool DISPLAY_BOOT_SCREEN_ENABLED = true;

// DY-SV17F uses UART control mode at 9600 8N1.
// ESP32 UART signals are routed through the GPIO matrix, so UART2 is placed on
// GPIO18/19 instead of 16/17 to keep possible PSRAM pins free.
constexpr uint8_t AUDIO_UART_PORT = 2;
constexpr int8_t AUDIO_RX_PIN = 18;  // ESP32 RX <- DY-SV17F TXD/IO0
constexpr int8_t AUDIO_TX_PIN = 19;  // ESP32 TX -> DY-SV17F RXD/IO1
constexpr int8_t AUDIO_BUSY_PIN = 39; // DY-SV17F CON3/BUSY, active LOW while playing
constexpr uint32_t AUDIO_BAUD_RATE = 9600;
constexpr uint32_t AUDIO_RESPONSE_TIMEOUT_MS = 1000;
constexpr uint32_t AUDIO_COMMAND_VERIFY_DELAY_MS = 220;
constexpr uint32_t AUDIO_INTER_COMMAND_DELAY_MS = 120;
constexpr uint32_t AUDIO_BOOT_GRACE_MS = 1200;
constexpr bool AUDIO_BOOT_TONE_ENABLED = true;
constexpr uint16_t AUDIO_BOOT_TONE_TRACK = 1;
constexpr uint32_t AUDIO_BOOT_TONE_DELAY_MS = 350;
constexpr uint16_t AUDIO_TEST_TRACK = 1;

// CON3/BUSY is also a mode selection input during roughly the first 30 ms of
// DY-SV17F power-up. Hardware wiring must hold it HIGH during that interval;
// GPIO39 cannot provide an internal pull-up. Use an external ~10 kOhm pull-up
// to the DY-SV17F V33 pin and tie CON1 + CON2 directly LOW for UART mode.

// Generic GPIO channels. INPUT_PULLUP + activeLow makes an unconnected/open
// contact read inactive and a switch to GND read active. Projects can change
// these definitions without touching the GPIO module implementation.
enum class GpioDirection : uint8_t { Input, Output };
enum class PullMode : uint8_t { None, Up, Down };

struct GpioChannelConfig {
  const char *id;
  GpioDirection direction;
  int8_t pin;
  PullMode pull;
  bool activeHigh;
  bool safeBootState;
  uint16_t debounceMs;
  // Optional edge latch for important human inputs. The ISR only marks that
  // an active edge occurred; debounce/callback logic stays in the normal loop.
  bool interruptLatch;
  int8_t feedbackPin;
  PullMode feedbackPull;
  bool feedbackActiveHigh;
  uint16_t feedbackDelayMs;
  bool enabled;
};

// Project profile: only DI1 is currently consumed by the interruption button.
// The remaining generic base channels stay defined but disabled, so their pins
// are free for later project extensions and do not create unnecessary runtime
// scanning or output configuration.
constexpr GpioChannelConfig GPIO_CHANNELS[] = {
    {"di1", GpioDirection::Input,  13, PullMode::Up, false, false, 25, true,  -1, PullMode::None, true, 0, true},
    {"di2", GpioDirection::Input,  14, PullMode::Up, false, false, 25, false, -1, PullMode::None, true, 0, false},
    {"di3", GpioDirection::Input,  32, PullMode::Up, false, false, 25, false, -1, PullMode::None, true, 0, false},
    {"di4", GpioDirection::Input,  33, PullMode::Up, false, false, 25, false, -1, PullMode::None, true, 0, false},
    {"do1", GpioDirection::Output, 25, PullMode::None, true, false, 0, false, -1, PullMode::None, true, 0, false},
    {"do2", GpioDirection::Output, 26, PullMode::None, true, false, 0, false, -1, PullMode::None, true, 0, false},
    {"do3", GpioDirection::Output, 27, PullMode::None, true, false, 0, false, -1, PullMode::None, true, 0, false},
    {"do4", GpioDirection::Output, 23, PullMode::None, true, false, 0, false, -1, PullMode::None, true, 0, false},
};

constexpr size_t GPIO_CHANNEL_COUNT = sizeof(GPIO_CHANNELS) / sizeof(GPIO_CHANNELS[0]);
constexpr size_t GPIO_EVENT_CALLBACK_CAPACITY = 4;
constexpr uint32_t GPIO_SCAN_INTERVAL_MS = 5;

}  // namespace HardwareConfig
