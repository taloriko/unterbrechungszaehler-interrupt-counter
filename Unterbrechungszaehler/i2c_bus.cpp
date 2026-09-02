#include "i2c_bus.h"

#include <Wire.h>

#include "hardware_config.h"
#include "serial_log.h"

namespace I2cBus {
namespace {
bool ready = false;
}

bool begin() {
  if (ready) return true;
  if (!Wire.begin(HardwareConfig::I2C_SDA_PIN, HardwareConfig::I2C_SCL_PIN, HardwareConfig::I2C_FREQUENCY_HZ)) {
    SerialLog::errorf("I2C", "Bus start failed | SDA=%d | SCL=%d", HardwareConfig::I2C_SDA_PIN, HardwareConfig::I2C_SCL_PIN);
    return false;
  }
  Wire.setTimeOut(50);
  ready = true;
  SerialLog::infof("I2C", "Shared bus ready | SDA=%d | SCL=%d | %lu Hz",
                   HardwareConfig::I2C_SDA_PIN, HardwareConfig::I2C_SCL_PIN,
                   static_cast<unsigned long>(HardwareConfig::I2C_FREQUENCY_HZ));
  return true;
}

bool initialized() { return ready; }

bool probe(uint8_t address) {
  if (!ready && !begin()) return false;
  Wire.beginTransmission(address);
  return Wire.endTransmission(true) == 0;
}

bool write(uint8_t address, const uint8_t *data, size_t length) {
  if (!ready && !begin()) return false;
  Wire.beginTransmission(address);
  if (data && length) Wire.write(data, length);
  return Wire.endTransmission(true) == 0;
}

bool writeRegister(uint8_t address, uint8_t reg, const uint8_t *data, size_t length) {
  if (!ready && !begin()) return false;
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (data && length) Wire.write(data, length);
  return Wire.endTransmission(true) == 0;
}

bool readRegisters(uint8_t address, uint8_t startReg, uint8_t *data, size_t length) {
  if (!data || length == 0) return false;
  if (!ready && !begin()) return false;

  Wire.beginTransmission(address);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) return false;

  const size_t received = Wire.requestFrom(static_cast<uint16_t>(address), static_cast<uint8_t>(length), true);
  if (received != length) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < length; ++i) data[i] = static_cast<uint8_t>(Wire.read());
  return true;
}

}  // namespace I2cBus
