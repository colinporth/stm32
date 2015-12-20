// saveToBmp.c
#include <stdint.h>
#include <string.h>

#include "ff.h"

#define CAMERA_FRAME_BUFFER 0xD0000000
#define CAMERA_CVRT_BUFFER  0xD0025800

#define RGB_HEADER_SIZE      54
#define IMAGE_BUFFER_SIZE   (320*240*2)  // Size of RGB16 image
#define MAX_IMAGE_SIZE      (320*240*3)  // Size of RGB24 image
#define IMAGE_COLUMN_SIZE   240
#define IMAGE_LINE_SIZE     320

const uint8_t BMPHeader_QQVGA24Bit[] = {
  0x42, 0x4D,              // Offet0   : BMP Magic Number
  0x36, 0x84, 0x03, 0x00,  // Offset2  : filesz : Size of the BMP file 240*320*3 + 54
  0x00, 0x00, 0x00, 0x00,  // Offset6, Offset8 : Reserved0, Reserved1 =0
  0x36, 0x00, 0x00, 0x00,  // Offset10 : bmp_offset: Offset of bitmap data (pixels)  = 54 = 0x36
  0x28, 0x00, 0x00, 0x00,  // Offset14 : header_sz : The number of bytes in the header (from this point)
  0x40, 0x01, 0x00, 0x00,  // Offset18 : width 320
  0xF0, 0x00, 0x00, 0x00,  // Offset2  : height 240
  0x01, 0x00,              // Offset26 : nplanes
  0x18, 0x00,              // Offset24 : Bits per Pixels
  0x00, 0x00, 0x00, 0x00,  // Offset30 : compress_type = 0
  0x00, 0x58, 0x02, 0x00,  // Offset34 : bmp bytes size
  0x00, 0x00, 0x00, 0x00,  // Offset38 : X Resolution : Pixel per meters = 0
  0x00, 0x00, 0x00, 0x00,  // Offset42 : Y Resolution : Pixel per meters
  0x00, 0x00, 0x00, 0x00,  // Offset46 : Number of Colours = 0
  0x00, 0x00, 0x00, 0x00,  // Offset50 : Important Colours = 0
  };

//{{{
static void RGB16toRGB24 (uint8_t *pDestBuffer, uint8_t *pSrcBuffer) {

  #define BMP_PIXEL16_TO_R(pixel)  ((pixel & 0x1F) << 3)
  #define BMP_PIXEL16_TO_G(pixel)  (((pixel >> 5) & 0x3F) << 2)
  #define BMP_PIXEL16_TO_B(pixel)  (((pixel >> 11) & 0x1F) << 3)

  uint16_t *pSrc;
  uint8_t *pDest;
  uint32_t i = 0, j = 0;
  uint16_t value;

  pSrc = (uint16_t*) &pSrcBuffer[IMAGE_BUFFER_SIZE] - IMAGE_LINE_SIZE;
  pDest = (uint8_t*) &pDestBuffer[0];

  for (i = IMAGE_COLUMN_SIZE; i > 0; i-- ) {
    for (j = 0; j < 2 * IMAGE_LINE_SIZE;  j += 2 ) {
      value    = (uint16_t)*pSrc;

      *pDest++   = BMP_PIXEL16_TO_R(value);
      *pDest++   = BMP_PIXEL16_TO_G(value);
      *pDest++   = BMP_PIXEL16_TO_B(value);
      pSrc++;
      }

    pSrc -= (IMAGE_LINE_SIZE * 2);
    }

  }
//}}}

uint8_t saveToFile (int frame) {

  char filename [80];
  sprintf (filename, "hello%d.bmp", frame);

  FIL file;

  uint32_t  NumWrittenData;
  uint8_t ret = 1;

  if (f_open (&file, filename, FA_CREATE_NEW | FA_WRITE) == FR_OK) {
    if (f_write (&file, (char*)BMPHeader_QQVGA24Bit, RGB_HEADER_SIZE, (UINT*)&NumWrittenData) == FR_OK) {
      f_sync (&file);

      RGB16toRGB24 ((uint8_t*)CAMERA_CVRT_BUFFER, (uint8_t*)CAMERA_FRAME_BUFFER);

      if (f_write (&file, (char*)CAMERA_CVRT_BUFFER, MAX_IMAGE_SIZE, (UINT*)&NumWrittenData) == FR_OK)
        ret = 0;
      }

    f_close(&file);
    }

  return ret;
  }
