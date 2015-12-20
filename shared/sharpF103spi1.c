// sharpF103spi1.c
//{{{  includes
#include "display.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
//}}}

#define DISP_PIN     GPIO_Pin_3  // GPIO PA3 = DISP normally hi
#define CS_PIN       GPIO_Pin_4  // GPIO PA4 = CS   normally lo, could be AF SPI NSS
#define VCOM_PIN     GPIO_Pin_6  // GPIO PA6 = VCOM normally flipping
#define SPI_SCK_PIN  GPIO_Pin_5  // SPI1 PA5 = SCK  AF
#define SPI_MOSI_PIN GPIO_Pin_7  // SPI1 PA7 = MOSI AF

#define paddingByte  0x00
#define commandByte  0x80
#define vcomByte     0x40
#define clearByte    0x20

#define TFTWIDTH     400 // 96
#define TFTHEIGHT    240 // 96
#define TFTPITCH     ((TFTWIDTH/8)+2)  // line has lineNum, byte packed bit data, padding

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

bool getMono() { return true; }
uint16_t getWidth() { return TFTWIDTH; }
uint16_t getHeight() { return TFTHEIGHT; }

//{{{
void toggleVcom() {

  if (vcom) {
    GPIOA->BRR = VCOM_PIN;
    vcom = false;
    }
  else {
    GPIOA->BSRR = VCOM_PIN;
    vcom = true;
    }
  }
//}}}
//{{{
void clearScreenOff() {
  // CS hi, clearScreen, CS lo
  GPIOA->BSRR = CS_PIN;

  writeByte (clearByte);
  writeByte (paddingByte);

  while (SPI1->SR & SPI_I2S_FLAG_BSY);
  GPIOA->BRR = CS_PIN;
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
    GPIOA->BSRR = CS_PIN;

    writeByte (commandByte);

    uint8_t* frameBufPtr = frameBuf + (yorg * TFTPITCH);
    for (int i = 0; i < (yend-yorg+1) * TFTPITCH; i++)
      writeByte (*frameBufPtr++);

    writeByte (paddingByte);
    while (SPI1->SR & SPI_I2S_FLAG_BSY);

    GPIOA->BRR = CS_PIN;
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
void drawImage (image_t* image, int16_t xorg, int16_t yorg) {}

//{{{
void displayInit() {

  // enable clocks
  RCC->APB2ENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1;

  // enable CS, VCOM, DISP pins
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = CS_PIN | VCOM_PIN | DISP_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init (GPIOA, &GPIO_InitStruct);

  // CS, DISP lo
  GPIOA->BRR = CS_PIN | DISP_PIN | VCOM_PIN;

  // enable SPI pins
  GPIO_InitStruct.GPIO_Pin = SPI_SCK_PIN | SPI_MOSI_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init (GPIOA, &GPIO_InitStruct);

  // set SPI master, mode0, 8bit, MSBfirst
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;   // mode 0
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; // mode 0
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;    // 72Mhz / 8 = 9MHz
  SPI_Init (SPI1, &SPI_InitStructure);

  // SPI1 enable
  SPI1->CR1 |= SPI_CR1_SPE;

  clearScreenOff();

  // DISP hi
  GPIOA->BSRR = DISP_PIN;

  // frameBuf stuffed with lineAddress and line end paddingByte
  for (uint16_t y = 0; y < TFTHEIGHT; y++) {
    frameBuf [y*TFTPITCH] = reverseByte (y+1);
    frameBuf [(y*TFTPITCH) + 1 + (TFTWIDTH/8)] = paddingByte;
    }
  }
//}}}
