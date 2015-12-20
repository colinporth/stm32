// mainf407.c
//{{{  includes
#include "stdio.h"

#include "display.h"
#include "delay.h"

#include "stm32f4xx.h"

#include "i2c.h"
//}}}
//{{{  externals
font_t font18;
font_t font36;
font_t font72;
font_t font120;
//}}}

int main() {
  delayInit();

  displayInit();
  clearScreen (Black);

  // lm75 i2c init
  i2cInit (0x90);
  bool tempFitted = i2cGetStatus();
  if (tempFitted)
    i2cWriteRegWord (0x01, 0x6000);

  char str[80];
  sprintf (str, "Ver " __TIME__" "__DATE__"");
  drawString (White, &font18, str, 0, 0, getWidth(), 24);

  sprintf (str, "f4 display %dx%d", getWidth(), getHeight());
  drawString (White, &font36, str, 0, getHeight()-42, getWidth(), 42);

  int i = 0;
  while (true) {
    int tenths = i % 10;

    if (tempFitted && (!tenths)) {
      // read lm75 temp
      uint16_t value =  i2cReadReg (0x00);
      sprintf (str, "%d.%d", value >> 8, ((value & 0xFF)*10) >> 8);
      if (!getMono())
        drawRect (Black, 0, 200, getWidth(), 80);
      drawString (White, &font18, str,  0, 200, getWidth(), 80);
      }

    drawRect (Black, 0, 26, getWidth(), 24);
    drawRect (White, tenths * (getWidth()/10), 26, (getWidth()/10)-1, (2*tenths)+4);

    sprintf (str, "%d:%02d:%1d", (i/600) % 60, (i/10) % 60, tenths);
    if (!getMono())
      drawRect (Black, 0, 40, getWidth(), getWidth()*5/16);
    drawString (White, &font120, str, 0, 40, getWidth(), getWidth()*5/16);

    delayMs (100);
    i++;
    }
  }
