#include "display.h"
#include "displayHw.h"
#include <stdio.h>

#define TFTWIDTH  240
#define TFTHEIGHT 320

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
    writeCommandData (0x50, xorg); // Horizontal Start Position
    writeCommandData (0x51, xend); // Horizontal End Position

    writeCommandData (0x52, yorg); // Vertical Start Position
    writeCommandData (0x53, yend); // Vertical End Position

    writeCommandData (0x20, xorg); // Horizontal GRAM Address Set
    writeCommandData (0x21, yorg); // Vertical GRAM Address Set

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

  displayHwInit (USE_RES_PIN | USE_DUPLEX);

  printf ("ili9320 init data:%x status:%x\n", readData(), readStatus());

  writeCommandData (0x00,0x0000);  // start oscillation - stopped?
  writeCommandData (0x01,0x0100);  // Driver Output Control 1 - SS=1 and SM=0
  writeCommandData (0x02,0x0700);  // LCD Driving Control - set line inversion
  writeCommandData (0x03,0x1030);  // Entry Mode - BGR, HV inc, vert write,
  writeCommandData (0x04,0x0000);  // Resize Control
  writeCommandData (0x08,0x0202);  // Display Control 2
  writeCommandData (0x09,0x0000);  // Display Control 3
  writeCommandData (0x0a,0x0000);  // Display Control 4 - frame marker
  writeCommandData (0x0c,0x0001);  // RGB Display Interface Control 1
  writeCommandData (0x0d,0x0000);  // Frame Marker Position
  writeCommandData (0x0f,0x0000);  // RGB Display Interface Control 2
  delay_ms (40);

  writeCommandData (0x07,0x0101);  // Display Control 1
  delay_ms (40);

  writeCommandData (0x10,0x10C0);  // Power Control 1
  writeCommandData (0x11,0x0007);  // Power Control 2
  writeCommandData (0x12,0x0110);  // Power Control 3
  writeCommandData (0x13,0x0b00);  // Power Control 4
  writeCommandData (0x29,0x0000);  // Power Control 7
  writeCommandData (0x2b,0x4010);  // Frame Rate and Color Control

  // 0x30 - 0x3d gamma
  writeCommandData (0x60,0x2700);  // Driver Output Control 2
  writeCommandData (0x61,0x0001);  // Base Image Display Control
  writeCommandData (0x6a,0x0000);  // Vertical Scroll Control

  writeCommandData (0x80,0x0000);  // Partial Image 1 Display Position
  writeCommandData (0x81,0x0000);  // Partial Image 1 Area Start Line
  writeCommandData (0x82,0x0000);  // Partial Image 1 Area End Line
  writeCommandData (0x83,0x0000);  // Partial Image 2 Display Position
  writeCommandData (0x84,0x0000);  // Partial Image 2 Area Start Line
  writeCommandData (0x85,0x0000);  // Partial Image 2 Area End Line

  writeCommandData (0x90,0x0010);  // Panel Interface Control 1
  writeCommandData (0x92,0x0000);  // Panel Interface Control 2
  writeCommandData (0x93,0x0001);  // Panel Interface Control 3
  writeCommandData (0x95,0x0110);  // Panel Interface Control 4
  writeCommandData (0x97,0x0000);  // Panel Interface Control 5
  writeCommandData (0x98,0x0000);  // Panel Interface Control 6

  writeCommandData (0x07,0x0133);  // Display Control 1
  delay_ms (40);

  //backlightOn (true);
}
//}}}
