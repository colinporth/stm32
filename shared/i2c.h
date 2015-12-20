#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
  extern "C" {
#endif

void i2cInit (uint8_t slaveAddr);

bool i2cGetStatus();

uint8_t i2cReadReg (uint8_t regName);
uint16_t i2cReadWord (uint8_t regName);
void i2cWriteReg (uint8_t regName, uint8_t value);
void i2cWriteRegWord (uint8_t regName, uint16_t value);

#ifdef __cplusplus
  }
#endif
