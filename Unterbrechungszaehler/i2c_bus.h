#pragma once

#include <Arduino.h>

namespace I2cBus {

bool begin();
bool initialized();
bool probe(uint8_t address);
bool write(uint8_t address, const uint8_t *data, size_t length);
bool writeRegister(uint8_t address, uint8_t reg, const uint8_t *data, size_t length);
bool readRegisters(uint8_t address, uint8_t startReg, uint8_t *data, size_t length);

}  // namespace I2cBus
