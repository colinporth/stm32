// main.c
//{{{  includes
#include <stdio.h>
#include <string.h>

#include "../files/ff.h"
#include "../files/sd.h"

#include "delay.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

#include "display.h"
#include "camera.h"
//}}}
//{{{  externals
int freeSansBold_len;
const uint8_t freeSansBold[64228];
//}}}

static FATFS fatfs;
static DWORD serialNo;
static char path[128] = "/";
static char label[128];
static char fileStr[256];
static char longFileName[_MAX_LFN + 1];
static int dirs = 0;
static int file = 0;
static int files = 0;

#define FRAME_BUFFER  0xD0000000

//{{{
static int countFiles (char* path) {

  FILINFO filinfo;
  filinfo.lfname = longFileName;
  filinfo.lfsize = sizeof longFileName;

  DIR dir;
  int ok = f_opendir (&dir, path) == FR_OK;
  if (ok) {
    int i = strlen (path);
    while (true) {
      ok = (f_readdir (&dir, &filinfo) == FR_OK);
      if (!ok || filinfo.fname[0] == 0) // Break on error or end of dir
        break;
      if (filinfo.fname[0] == '.') // Ignore dot entry
        continue;

      char* fileName = *filinfo.lfname ? filinfo.lfname : filinfo.fname;
      if (filinfo.fattrib & AM_DIR) {
        // directory
        sprintf (&path[i], "/%s", fileName);
        ok = countFiles (path);
        if (!ok)
          break;
        path[i] = 0;
        dirs++;
        }
      else
        files++;
      }
    }

  return ok;
  }
//}}}

int main() {
  delayInit();

  displayInit();
  clearScreen (Blue);
  setTTFont (freeSansBold, freeSansBold_len);

  char str[80];
  sprintf (str, "Ver " __TIME__" "__DATE__"");
  drawTTString (Yellow, 18, str, 0, 0, getWidth(), 24);

  if (SD_init (true) == SD_OK) {
    f_mount (0, &fatfs);
    countFiles (path);
    }

  if (initCamera (true)) {
    startCamera();
    while (true) {
      waitCamera();
      showCamera();

      //sprintf (str, "frame %d files:%d", cameraFrame(), files);
      //drawTTString (White, 24, str, 0, getHeight()-32, getWidth(), 32);

      //saveToFile (cameraFrame());
      }
    }
  else {
    sprintf (str, "camera id %x", cameraId());
    drawTTString (White, 24, str, 0, getHeight()-32, getWidth(), 32);
    }
  }
