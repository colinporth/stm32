// sharpf407.c
//{{{  includes
#include "display.h"
#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
//}}}

#define VCOM_PIN    GPIO_Pin_2 // GPIO PA2 = VCOM normally flipping
#define DISP_PIN    GPIO_Pin_3 // GPIO PA3 = DISP normally hi
#define CS_PIN      GPIO_Pin_4 // GPIO PA4 = CS   normally lo, could be AF SPI NSS
//                                SPI1 PA5 = SCK  AF
//                                SPI1 PA7 = MOSI AF
#define paddingByte 0x00
#define commandByte 0x80
#define vcomByte    0x40
#define clearByte   0x20

#define TFTWIDTH    400 // 96
#define TFTHEIGHT   240 // 96
#define TFTPITCH    ((TFTWIDTH/8)+2)  // line has lineNum, byte packed bit data, padding

static bool vcom;
static uint8_t frameBuf [TFTPITCH*TFTHEIGHT];

//{{{
static uint8_t reverseByte (uint8_t b) {

  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
  }
//}}}
//{{{
static inline void writeByte (uint8_t byte) {

  SPI1->DR = byte;
  while (!(SPI1->SR & SPI_I2S_FLAG_TXE));
  }
//}}}

//{{{
bool getMono() {

  return true;
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
void toggleVcom() {

  if (vcom) {
    GPIOA->BSRRH = VCOM_PIN;
    vcom = false;
    }
  else {
    GPIOA->BSRRL = VCOM_PIN;
    vcom = true;
    }
  }
//}}}
//{{{
void clearScreenOff() {
  // CS hi, clearScreen, CS lo
  GPIOA->BSRRL = CS_PIN;

  writeByte (clearByte);
  writeByte (paddingByte);

  while (SPI1->SR & SPI_I2S_FLAG_BSY);
  GPIOA->BSRRH = CS_PIN;
  }
//}}}

//{{{
void setPixel (uint16_t colour, int16_t x, int16_t y) {

  if ((x < TFTWIDTH) && (y < TFTHEIGHT)) {
    uint8_t* framePtr = frameBuf + (y * TFTPITCH) + 1 + (x/8);
    uint8_t xMask = 0x80 >> (x & 7);
    if (colour)
      *framePtr |= xMask;
    else
      *framePtr &= ~xMask;
    }
  }
//}}}
//{{{
void setBitPixels (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  uint16_t xend = xorg + xlen - 1;
  if (xend >= TFTWIDTH) xend = TFTWIDTH - 1;
  uint16_t yend = yorg + ylen - 1;
  if (yend >= TFTHEIGHT) yend = TFTHEIGHT - 1;

  for (uint16_t y = yorg; y <= yend; y++) {
    uint8_t* linePtr = frameBuf + (y * TFTPITCH) + 1;
    for (uint16_t x = xorg; x <= xend; x++) {
      uint8_t* framePtr = linePtr + (x/8);
      uint8_t xMask = 0x80 >> (x & 7);
      if (colour)
        *framePtr |= xMask;
      else
        *framePtr &= ~xMask;
      }
    }
  }
//}}}
//{{{
void setTTPixels (uint16_t colour, uint8_t* bytes, int16_t x, int16_t y, uint16_t pitch, uint16_t rows) {
// set pixels for a truetype bitmap fontChar

  uint8_t xstart  = x / 8;
  uint8_t xshift1 = x & 7;
  uint8_t xshift2 = 8 - (x & 7);

  if ((y+rows) > TFTHEIGHT)
    rows = TFTHEIGHT - y;

  for (int16_t j = y; j < y+rows; j++) {
    uint8_t* framePtr = frameBuf + (j * TFTPITCH) + 1 + xstart;

    for (int16_t i = 0; i < pitch; i++) {
      if ((x + (i*8)) >= TFTWIDTH)
         bytes++;
      else if (colour) {
        *framePtr++ |= (*bytes) >> xshift1;
        *framePtr |= *bytes++ << xshift2; // might spill into padding byte
        }
      else {
        *framePtr++ &= ~(*bytes >> xshift1);
        *framePtr &= ~(*bytes++ << xshift2); // might spill into padding byte
        }
      }
    }
  }
//}}}

//{{{
void drawLines (uint16_t yorg, uint16_t yend) {

  if (yorg < TFTHEIGHT) {
    GPIOA->BSRRL = CS_PIN;

    writeByte (commandByte);

    uint8_t* frameBufPtr = frameBuf + (yorg * TFTPITCH);
    for (int i = 0; i < (yend-yorg+1) * TFTPITCH; i++)
      writeByte (*frameBufPtr++);

    writeByte (paddingByte);
    while (SPI1->SR & SPI_I2S_FLAG_BSY);

    GPIOA->BSRRH = CS_PIN;
    }

  toggleVcom();
  }
//}}}
//{{{
void drawRect (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  uint16_t xend = xorg + xlen - 1;
  if (xend >= TFTWIDTH) xend = TFTWIDTH - 1;

  uint16_t yend = yorg + ylen - 1;
  if (yend >= TFTHEIGHT) yend = TFTHEIGHT - 1;

  uint8_t xFirstByte = xorg/8;
  uint8_t xFirstMask = 0x80 >> (xorg & 7);

  for (uint16_t y = yorg; y <= yend; y++) {
    uint8_t* framePtr = frameBuf + (y * TFTPITCH) + 1 + xFirstByte;
    uint8_t xmask = xFirstMask;
    for (uint16_t x = xorg; x <= xend; x++) {
      if (colour)
        *framePtr |= xmask;
      else
        *framePtr &= ~xmask;
      xmask >>= 1;
      if (xmask == 0) {
        xmask = 0x80;
        framePtr++;
        };
      }
    }

  drawLines (yorg, yend);
  }
//}}}

//{{{
void displayInit() {
  // enable clocks
  RCC->AHB1ENR |= RCC_AHB1Periph_GPIOA;
  RCC->APB2ENR |= RCC_APB2Periph_SPI1;

  // enable CS, VCOM, DISP pins
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = VCOM_PIN | DISP_PIN | CS_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init (GPIOA, &GPIO_InitStruct);

  // VCOM, CS, DISP lo
  GPIOA->BSRRH = VCOM_PIN | DISP_PIN | CS_PIN;

  // enable GPIO AF SPI pins
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOA, &GPIO_InitStruct);

  GPIO_PinAFConfig (GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

  // set SPI1 master, mode0, 8bit, LSBfirst, NSS pin high, baud rate
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;   // SPI mode0
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; // SPI mode0
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 168mHz/2 / 8 = 10.5mHz
  SPI_Init (SPI1, &SPI_InitStructure);

  // SPI1 enable
  SPI1->CR1 |= SPI_CR1_SPE;

  clearScreenOff();

  // DISP hi
  GPIOA->BSRRL = DISP_PIN;

  // frameBuf stuffed with lineAddress and line end paddingByte
  for (uint16_t y = 0; y < TFTHEIGHT; y++) {
    frameBuf [y*TFTPITCH] = reverseByte (y+1);
    frameBuf [(y*TFTPITCH) + 1 + (TFTWIDTH/8)] = paddingByte;
    }
  }
//}}}
