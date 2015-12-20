#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "font.h"
#include "image.h"

//{{{  colour defines
#define Black        0x0000  /*   0,   0,   0 */
#define Blue         0x001F  /*   0,   0, 255 */
#define Green        0x07E0  /*   0, 255,   0 */
#define Cyan         0x07FF  /*   0, 255, 255 */
#define Red          0xF800  /* 255,   0,   0 */
#define Magenta      0xF81F  /* 255,   0, 255 */
#define Yellow       0xFFE0  /* 255, 255,   0 */
#define White        0xFFFF  /* 255, 255, 255 */

#define Navy         0x000F  /*   0,   0, 128 */
#define DarkGreen    0x03E0  /*   0, 128,   0 */
#define DarkCyan     0x03EF  /*   0, 128, 128 */
#define Maroon       0x7800  /* 128,   0,   0 */
#define Purple       0x780F  /* 128,   0, 128 */
#define Olive        0x7BE0  /* 128, 128,   0 */
#define LightGrey    0xC618  /* 192, 192, 192 */
#define DarkGrey     0x7BEF  /* 128, 128, 128 */
#define Orange       0xFD20  /* 255, 165,   0 */
#define GreenYellow  0xAFE5  /* 173, 255,  47 */
//}}}

#ifdef __cplusplus
  extern "C" {
#endif

void lcdSpiDisplay1 (uint32_t frameBuffer, int16_t xorg, int16_t yorg, uint16_t xpitch);
void lcdSpiDisplay2 (uint32_t frameBuffer, int16_t xorg, int16_t yorg, uint16_t xpitch);
void lcdSpiDisplayMono (uint32_t frameBuffer, int16_t xorg, int16_t yorg, uint16_t xpitch);

bool getMono();
uint16_t getWidth();
uint16_t getHeight();

void setPixel (uint16_t colour, int16_t x, int16_t y);
void setBitPixels (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen);
void setTTPixels (uint16_t colour, uint8_t* bytes, int16_t x, int16_t y, uint16_t pitch, uint16_t rows);
void drawLines (uint16_t yorg, uint16_t yend);

void setTTFont (const uint8_t* ttFont, int length);
void drawTTString (uint16_t colour, int height, const char* str,
                   int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen);

void drawString (uint16_t colour, font_t* font, const char* str,
                 int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen);
void drawRect (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen);
void drawImage (image_t* image, int16_t x, int16_t y);

void clearScreen (uint16_t colour);
void tileImageScreen (image_t* image);

void displayInit();

#ifdef __cplusplus
  }
#endif
