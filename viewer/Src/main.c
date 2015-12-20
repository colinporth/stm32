//{{{  includes
#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"

#include "../shared/display.h"
#include "../shared/touch.h"

#include "../files/ff.h"
#include "../files/tjpgd.h"
#include "../files/sd.h"

#include <ft2build.h>
#include FT_FREETYPE_H
//}}}
//{{{  externs
int freeSansBold_len;
const uint8_t freeSansBold[64228];
SD_CardInfo SDCardInfo;
//}}}

static __IO uint32_t DelayCount;

static FATFS fatfs;
static DWORD serialNo;
static char path[128] = "/";
static char label[128];
static char fileStr[256];
static char longFileName[_MAX_LFN + 1];
static int dirs = 0;
static int file = 0;
static int files = 0;

//{{{
void delayMs (__IO uint32_t ms) {

  DelayCount = ms;
  while (DelayCount != 0) {}
  }
//}}}
//{{{
void SysTick_Handler(void) {

  if (DelayCount != 0x00)
    DelayCount--;
  }
//}}}

//{{{
static UINT inFunc (JDEC* jdec, BYTE* buf, UINT bytes) {

  //printf ("input_func %d\n", bytes);
  if (buf)
    f_read ((FIL*)jdec->device, buf, bytes, &bytes);
  else
    f_lseek ((FIL*)jdec->device, f_tell ((FIL*)jdec->device) + bytes);

  return bytes;
}
//}}}
//{{{
static UINT outFunc (JDEC* jdec, void* buf, JRECT* rect) {

  //printf ("output_func %d %d %d %d\n", rect->left, rect->top, rect->width, rect->height);
  image_t image;
  image.width = rect->width;
  image.height = rect->height;
  image.pixels = buf;

  drawImage (&image, rect->left, 16+rect->top);
  return JDR_OK;
  }
//}}}
//{{{
static int decodeJpgFile (const char* fileName) {

  JDEC jdec;
  BYTE jdecWork[4096];

  FIL file;
  f_open (&file, fileName, FA_OPEN_EXISTING | FA_READ);

  jd_prepare (&jdec, inFunc, jdecWork, 4096, &file);

  int scale = 0;
  int scaleRatio = 1;
  while (scale < 3 && ((jdec.width/scaleRatio) > getWidth() || (jdec.height/scaleRatio) > getHeight())) {
     scale++;
     scaleRatio *= 2;
     }

  jd_decomp (&jdec, outFunc, scale);

  char str[50];
  sprintf (str, "%dx%d scaled 1/%d\0", jdec.width, jdec.height, scaleRatio);
  //drawRect (Black, 0, getHeight()-32, getWidth(), 16);
  drawTTString (LightGrey, 12, str, 0, getHeight()-32, getWidth(), 16);

  return 1;
  }
//}}}
//{{{
static int decodeFiles (char* path) {

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
        ok = decodeFiles (path);
        if (!ok)
          break;
        path[i] = 0;
        }
      else {
        // file
        file++;
        int len = strlen (fileName);
        bool jpg = ((len > 4) &&
                    (fileName[len-4] == '.') &&
                    ((fileName[len-3] == 'j') || (fileName[len-3] == 'J')) &&
                    ((fileName[len-2] == 'p') || (fileName[len-2] == 'P')) &&
                    ((fileName[len-1] == 'g') || (fileName[len-1] == 'G')));
        if (jpg) {
          sprintf (fileStr, "%s/%s", path, fileName);
          decodeJpgFile (fileStr);
          }

        sprintf (fileStr, "%d:%d %s", file, files, fileName);
        drawRect (jpg ? Black : Yellow, 0, getHeight()-18, getWidth(), 18);
        drawTTString (White, 12, fileStr, 0, getHeight()-18, getWidth(), 17);
        }
      }
    }

  return ok;
  }
//}}}
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

int main (void) {
  // setup sysTick
  SysTick->VAL = 0;
  SysTick->LOAD = SystemCoreClock / 1000; // sysTick countdown value
  SysTick->CTRL = 0x7; // 0x4 = sysTick HCLK, 0x2 = int enable, 0x1 = enable

  displayInit();
  clearScreen (Black);
  setTTFont (freeSansBold, freeSansBold_len);

  char str[40];
  sprintf (str, "Ver " __TIME__" "__DATE__"");
  drawRect (Black, 0, 0, getWidth(), 16);
  drawTTString (White, 12, str, 0, 0, getWidth(), 16);

  if (SD_init (true) == SD_OK) {
    f_mount (0, &fatfs);
    f_getlabel (path, label, &serialNo);
    countFiles (path);
    decodeFiles (path);
    }

  touchInit();
  int16_t lastx = 0, lasty = 0;
  while (true) {
    while (getTouchDown()) {
      int16_t x,y,z;
      getTouchPos (&x, &y, &z, getWidth(), getHeight());

      if ((lastx != x) || (lasty != y)) {
        drawRect (Yellow, x-1, y-1, 3,3);

        lastx = x;
        lasty = y;

        sprintf (str, "touch %d %d", (int)x, (int)y);
        drawRect (Black, 20, getHeight()-30, 200, 30);
        drawTTString (White, 24, str, 20, getHeight()-30, getWidth(), 30);
        }
      }
    }

  return 0;
  }
