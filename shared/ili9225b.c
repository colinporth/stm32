#include "display.h"
#include "displayHw.h"
#include <stdio.h>

#define TFTWIDTH  176
#define TFTHEIGHT 220

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
void vertScroll (uint16_t yorg) {
}
//}}}
//{{{
static bool writeWindow (int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  uint16_t xend = xorg + xlen - 1;
  uint16_t yend = yorg + ylen - 1;
  if ((xend < TFTWIDTH) && (yend < TFTHEIGHT)) {
    writeCommandData (0x36, xend);
    writeCommandData (0x37, xorg);

    writeCommandData (0x38, yend);
    writeCommandData (0x39, yorg);

    writeCommandData (0x20, xorg);
    writeCommandData (0x21, yorg);

    writeCommand (0x22);
    return true;
  }
  return false;
}
//}}}

//{{{
void drawRect (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  if (writeWindow (xorg, yorg, xlen, ylen))
    writeColour (colour, xlen * ylen);
}
//}}}
//{{{
void drawImage (image_t* image, int16_t xorg, int16_t yorg) {

  if (writeWindow (xorg, yorg, image->width, image->height))
    writePixels (image->pixels, image->width * image->height);
}
//}}}

//{{{
void displayInit() {

  printf ("ili9225b_init\n");
  displayHwInit (USE_DC_PIN | USE_16BIT_DATA | USE_MODE0);

  // start Initial Sequence
  writeCommandData (0x01,0x011C); // set SS and NL bit

  writeCommandData (0x02,0x0100); // set 1 line inversion
  writeCommandData (0x03,0x1030); // set GRAM write direction and BGR=1
  writeCommandData (0x08,0x0808); // set BP and FP
  writeCommandData (0x0C,0x0000); // RGB interface setting R0Ch=0x0110 for RGB 18Bit and R0Ch=0111for RGB16
  writeCommandData (0x0F,0x0b01); // Set frame rate//0b01

  writeCommandData (0x20,0x0000); // Set GRAM Address
  writeCommandData (0x21,0x0000); // Set GRAM Address
  delay_ms(50);

  // power On sequence
  writeCommandData (0x10,0x0a00); // Set SAP,DSTB,STB//0800
  writeCommandData (0x11,0x1038); // Set APON,PON,AON,VCI1EN,VC
  delay_ms(50);

  writeCommandData (0x12,0x1121); // Internal reference voltage= Vci;
  writeCommandData (0x13,0x0063); // Set GVDD
  writeCommandData (0x14,0x4b44); // Set VCOMH/VCOML voltage//3944

  // set GRAM area
  writeCommandData (0x30,0x0000);
  writeCommandData (0x31,0x00DB);
  writeCommandData (0x32,0x0000);
  writeCommandData (0x33,0x0000);
  writeCommandData (0x34,0x00DB);
  writeCommandData (0x35,0x0000);
  writeCommandData (0x36,0x00AF);
  writeCommandData (0x37,0x0000);
  writeCommandData (0x38,0x00DB);
  writeCommandData (0x39,0x0000);

  // set Gamma Curve
  writeCommandData (0x50,0x0003);
  writeCommandData (0x51,0x0900);
  writeCommandData (0x52,0x0d05);
  writeCommandData (0x53,0x0900);
  writeCommandData (0x54,0x0407);
  writeCommandData (0x55,0x0502);
  writeCommandData (0x56,0x0000);
  writeCommandData (0x57,0x0005);
  writeCommandData (0x58,0x1700);
  writeCommandData (0x59,0x001F);
  delay_ms (50);

  writeCommandData (0x07,0x1017);
}
//}}}
