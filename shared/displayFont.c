// displayFont.c
#include "display.h"
#include "displayHw.h"

#include "font.h"

//{{{
void drawString (uint16_t colour, font_t* font, const char* str,
                 int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  setBitPixels (!colour, xorg, yorg, xlen, ylen);

  int16_t x = xorg;
  int16_t y = yorg;

  do {
    if (*str == ' ')
      x += font->spaceWidth;

    else if ((*str >= font->firstChar) && (*str <= font->lastChar)) {
      uint8_t* glyphData = (uint8_t*)(font->glyphsBase + font->glyphOffsets[*str - font->firstChar]);
      uint8_t width = (uint8_t)*glyphData++;
      uint8_t height = (uint8_t)*glyphData++;
      int8_t left = (int8_t)*glyphData++;
      uint8_t top = (uint8_t)*glyphData++;
      uint8_t advance = (uint8_t)*glyphData++;

      for (int16_t j = y+font->height-top; j < y+font->height-top+height; j++) {
        uint8_t glyphByte;
        for (int16_t i = 0; i < width; i++) {
          if (i % 8 == 0)
            glyphByte = *glyphData++;
          if (glyphByte & 0x80)
            setPixel (colour, x+left+i, j);
          glyphByte <<= 1;
          }
        }
      x += advance;
      }
    } while (*(++str));

  drawLines (yorg, yorg+ylen-1);
  }
//}}}
