#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
  extern "C" {
#endif

// options
#define USE_RES_PIN     0x0001
#define USE_DC_PIN      0x0002
#define USE_16BIT_DATA  0x0004
#define USE_MODE0       0x0008
#define USE_DUPLEX      0x0010

uint16_t readData();
uint16_t readStatus();

void writeCommand (uint16_t command);
void writeData (uint16_t data);
void writeCommandData (uint16_t command, uint16_t data);

void writeColour (uint16_t colour, uint32_t length);
void writePixels (uint16_t* pixels, uint32_t length);

void writeBytes (uint8_t* bytes, uint32_t length);
void writeColourBytes (uint16_t colour, uint32_t length);
void writePixelBytes (uint16_t* pixels, uint32_t length);

bool writeWindow (int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen);

void displayHwInit (uint16_t useOptions);

#ifdef __cplusplus
  }
#endif
