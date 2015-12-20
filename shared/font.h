#pragma once

#include <stdint.h>

typedef struct font {
  uint8_t fixedWidth;
  uint8_t height;
  uint8_t spaceWidth;
  uint8_t lineSpacing;
  uint8_t firstChar;
  uint8_t lastChar;
  const uint8_t* glyphsBase;
  const uint16_t* glyphOffsets;
} font_t;
