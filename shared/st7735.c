#include "display.h"
#include "displayHw.h"
#include <stdio.h>

#define TFTWIDTH  128
#define TFTHEIGHT 160
//{{{  commands
#define ST7735_NOP 0x0
#define ST7735_SWRESET 0x01
//#define ST7735_RDDID 0x04
//#define ST7735_RDDST 0x09
//#define ST7735_RDDPM 0x0A
//#define ST7735_RDDMADCTL 0x0B
//#define ST7735_RDDCOLMOD 0x0C
//#define ST7735_RDDIM 0x0D
//#define ST7735_RDDSM 0x0E

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26

#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RGBSET  0x2D
//#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR   0x30
#define ST7735_TEOFF   0x34
#define ST7735_TEON    0x35
#define ST7735_MADCTL  0x36
#define ST7735_IDMOFF  0x38
#define ST7735_IDMON   0x39
#define ST7735_COLMOD  0x3A

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_VMOFCTR 0xC7

#define ST7735_WRID2   0xD1
#define ST7735_WRID3   0xD2
#define ST7735_NVCTR1  0xD9
#define ST7735_NVCTR2  0xDE
#define ST7735_NVCTR3  0xDF

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1
//}}}

//{{{
bool getMono() {

  return false;
}
//}}}
//{{{
uint16_t getWidth() {

  return TFTWIDTH;
}
//}}}
//{{{
uint16_t getHeight() {

  return TFTHEIGHT;
}
//}}}

//{{{
static bool writeWindow (int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  uint16_t xend = xorg + xlen - 1;
  uint16_t yend = yorg + ylen - 1;
  if ((xend < TFTWIDTH) && (yend < TFTHEIGHT)) {
    writeCommand (ST7735_CASET);  // column addr set
    writeData (0);
    writeData (xorg);   // XSTART
    writeData (0);
    writeData (xend);   // XEND

    writeCommand (ST7735_RASET);  // row addr set
    writeData (0);
    writeData (yorg);    // YSTART
    writeData (0);
    writeData (yend);    // YEND

    writeCommand (ST7735_RAMWR);

    return true;
  }
  return false;
}
//}}}

//{{{
void drawRect (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  if (writeWindow (xorg, yorg, xlen, ylen))
    writeColour (colour, xlen*ylen);
}
//}}}
//{{{
void drawImage (image_t* image, int16_t xorg, int16_t yorg) {

  if (writeWindow (xorg, yorg, image->width, image->height))
    writePixels (image->pixels, image->width*image->height);
}
//}}}

//{{{
void displayInit() {

  displayHwInit (USE_DC_PIN);

  writeCommand (ST7735_SLPOUT);
  delay_ms (120);

  writeCommand (ST7735_FRMCTR1); // Frame rate in normal mode
  writeData (0x01);
  writeData (0x2C);
  writeData (0x2D);

  writeCommand (ST7735_FRMCTR2); // Frame rate in idle mode
  writeData (0x01);
  writeData (0x2C);
  writeData (0x2D);

  writeCommand (ST7735_FRMCTR3); // Frame rate in partial mode
  writeData (0x01);
  writeData (0x2C);
  writeData (0x2D);
  writeData (0x01);   // inversion mode settings
  writeData (0x2C);
  writeData (0x2D);

  writeCommandData (ST7735_INVCTR, 0x07); // Inverted mode off

  writeCommand (ST7735_PWCTR1); // POWER CONTROL 1
  writeData (0xA2);
  writeData (0x02);             // -4.6V
  writeData (0x84);             // AUTO mode

  writeCommandData (ST7735_PWCTR2, 0xC5); // POWER CONTROL 2 - VGH25 = 2.4C VGSEL =-10 VGH = 3*AVDD

  writeCommand (ST7735_PWCTR3); // POWER CONTROL 3
  writeData (0x0A);             // Opamp current small
  writeData (0x00);             // Boost freq

  writeCommand (ST7735_PWCTR4); // POWER CONTROL 4
  writeData (0x8A);             // BCLK/2, Opamp current small / medium low
  writeData (0x2A);

  writeCommand (ST7735_PWCTR5); // POWER CONTROL 5
  writeData (0x8A);             // BCLK/2, Opamp current small / medium low
  writeData (0xEE);

  writeCommandData (ST7735_VMCTR1, 0x0E); // POWER CONTROL 6
  writeCommandData (ST7735_MADCTL, 0xC0); // ORIENTATION
  writeCommandData (ST7735_COLMOD, 0x05); // COLOR MODE - 16bit per pixel

  //{{{  gamma GMCTRP1
  writeCommand (ST7735_GMCTRP1);
  writeData (0x02);
  writeData (0x1c);
  writeData (0x07);
  writeData (0x12);
  writeData (0x37);
  writeData (0x32);
  writeData (0x29);
  writeData (0x2d);
  writeData (0x29);
  writeData (0x25);
  writeData (0x2B);
  writeData (0x39);
  writeData (0x00);
  writeData (0x01);
  writeData (0x03);
  writeData (0x10);
  //}}}
  //{{{  Gamma GMCTRN1
  writeCommand (ST7735_GMCTRN1);
  writeData (0x03);
  writeData (0x1d);
  writeData (0x07);
  writeData (0x06);
  writeData (0x2E);
  writeData (0x2C);
  writeData (0x29);
  writeData (0x2D);
  writeData (0x2E);
  writeData (0x2E);
  writeData (0x37);
  writeData (0x3F);
  writeData (0x00);
  writeData (0x00);
  writeData (0x02);
  writeData (0x10);
  //}}}

  writeCommand (ST7735_DISPON); // display ON
}
//}}}
