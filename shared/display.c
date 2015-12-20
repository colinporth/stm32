// display.c
#include "display.h"
#include "displayHw.h"

//{{{
void clearScreen (uint16_t colour) {

  drawRect (colour, 0, 0, getWidth(), getHeight());
  }
//}}}
//{{{
void tileImageScreen (image_t* image) {

  for (uint16_t y = 0; y < getHeight(); y += image->height)
    for (uint16_t x = 0; x < getWidth(); x += image->width)
      drawImage (image, x, y);
  }
//}}}
