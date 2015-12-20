// ltdc.c
//  PG07 = CK   PF10 = DE
//  PC10 = R2   PA06 = G2   PD06 = B2
//  PB00 = R3   PG10 = G3   PG11 = B3
//  PA11 = R4   PB10 = G4   PG12 = B4
//  PA12 = R5   PB11 = G5   PA03 = B5
//  PB01 = R6   PC07 = G6   PB08 = B6
//  PG06 = R7   PD03 = G7   PB09 = B7
//{{{  includes
#include "delay.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_ltdc.h"
#include "stm32f4xx_dma2d.h"

#include "display.h"
//}}}

#define TFTWIDTH   1024    // min 39Mhz typ 45Mhz max 51.42Mhz
#define TFTHEIGHT  600
#define HORIZ_SYNC 176     // min 136 typ 176 max 216
#define VERT_SYNC  25      // min 12  typ 25  max 38

#define FRAME_BUFFER      0xD0000000
#define FRAME_BUFFER_SIZE   0x800000
//                          0x12c000
#define BUFFER_OFFSET     TFTWIDTH*TFTHEIGHT*2

static uint32_t frameBuffer = FRAME_BUFFER;

bool getMono() { return false; }
uint16_t getWidth() { return TFTWIDTH; }
uint16_t getHeight() { return TFTHEIGHT; }

void setBitPixels (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {}
void drawLines (uint16_t yorg, uint16_t yend) {}

//{{{
void setPixel (uint16_t colour, int16_t x, int16_t y) {

  *((uint16_t*)(frameBuffer) + (y*TFTWIDTH) + x) = colour;

  }
//}}}
//{{{
void setTTPixels (uint16_t colour, uint8_t* bytes, int16_t x, int16_t y, uint16_t pitch, uint16_t rows) {

  // Config dma2d - memory to memory with blending
  DMA2D->CR = (DMA2D->CR & 0xFFFCE0FC) | DMA2D_M2M_BLEND;
  DMA2D->OPFCCR = DMA2D_RGB565;
  DMA2D->OCOLR = 0;
  DMA2D->OMAR = frameBuffer + ((y*TFTWIDTH) + x)*2;
  DMA2D->OOR = TFTWIDTH - pitch;
  DMA2D->NLR = (pitch << 16) | rows;

  DMA2D->FGMAR = (uint32_t)bytes;
  DMA2D->FGOR = 0;
  DMA2D->FGPFCCR = CM_A8;
  DMA2D->FGCOLR = (colour & 0xF800)<<8 | (colour & 0x07E0)<<5 | (colour & 0x001f)<<3;
  DMA2D->FGCMAR = 0;

  DMA2D->BGMAR = frameBuffer + ((y * TFTWIDTH) + x)*2;
  DMA2D->BGOR = TFTWIDTH - pitch;
  DMA2D->BGPFCCR = CM_RGB565;
  DMA2D->BGCOLR = 0;
  DMA2D->BGCMAR = 0;

  // start dma2d
  DMA2D->CR |= DMA2D_CR_START;

  while (!(DMA2D->ISR & DMA2D_FLAG_TC)) {}
  DMA2D->IFCR |= DMA2D_IFSR_CTEIF | DMA2D_IFSR_CTCIF | DMA2D_IFSR_CTWIF|
                 DMA2D_IFSR_CCAEIF | DMA2D_IFSR_CCTCIF | DMA2D_IFSR_CCEIF;
  }
//}}}
//{{{
void drawRect (uint16_t colour, int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  // config dma2d - register to memory
  DMA2D->CR = (DMA2D->CR & 0xFFFCE0FC) | DMA2D_R2M;          // reset flags, set mode
  DMA2D->OPFCCR = DMA2D_RGB565;                              // colour format
  DMA2D->OCOLR = colour;                                     // fixed colour
  DMA2D->OMAR = frameBuffer + ((yorg*TFTWIDTH) + xorg)*2; // output memory address
  DMA2D->OOR = TFTWIDTH - xlen;                              // line offset
  DMA2D->NLR = (xlen << 16) | ylen;                          // xlen:ylen

  // start dma2d
  DMA2D->CR |= DMA2D_CR_START;

  while (!(DMA2D->ISR & DMA2D_FLAG_TC)) {}
  DMA2D->IFCR |= DMA2D_IFSR_CTEIF | DMA2D_IFSR_CTCIF | DMA2D_IFSR_CTWIF|
                 DMA2D_IFSR_CCAEIF | DMA2D_IFSR_CCTCIF | DMA2D_IFSR_CCEIF;
  }
//}}}
//{{{
void drawImage (image_t* image, int16_t xorg, int16_t yorg) {

  uint16_t* fromPixels = image->pixels;
  uint16_t* toPixels = (uint16_t*)(frameBuffer) + (yorg*TFTWIDTH) + xorg;

  for (uint16_t y = 0; y < image->height; y++) {
    for (uint16_t x = 0; x < image->width; x++)
      *toPixels++ = *fromPixels++;
    toPixels += TFTWIDTH - image->width;
    }

  }
//}}}

//{{{
void displayInit() {

  RCC->AHB1ENR |= RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC |
                  RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_DMA2D;
  RCC->APB2ENR |= RCC_APB2Periph_LTDC;

  RCC_PLLSAIConfig (160, 7, 2);                // 160/2 = 80mhz
  RCC_LTDCCLKDivConfig (RCC_PLLSAIDivR_Div2);  // 80/2  = 40mhz
  RCC_PLLSAICmd (ENABLE);
  while (RCC_GetFlagStatus (RCC_FLAG_PLLSAIRDY) == RESET) {}

  GPIO_PinAFConfig (GPIOA, GPIO_PinSource3,  GPIO_AF_LTDC);  // PA03  b5
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource6,  GPIO_AF_LTDC);  // PA06  g2
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource11, GPIO_AF_LTDC);  // PA11  r4
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource12, GPIO_AF_LTDC);  // PA12  r5
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource0,  0x09);          // PB00  r3
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource1,  0x09);          // PB01  r6
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource8,  GPIO_AF_LTDC);  // PB08  b6
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource9,  GPIO_AF_LTDC);  // PB09  b7
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource10, GPIO_AF_LTDC);  // PB10  g4
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource11, GPIO_AF_LTDC);  // PB11  g5
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource7,  GPIO_AF_LTDC);  // PC07  g6
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource10, GPIO_AF_LTDC);  // PC10  r2
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource3,  GPIO_AF_LTDC);  // PD03  g7
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource6,  GPIO_AF_LTDC);  // PD06  b2
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource10, GPIO_AF_LTDC);  // PF10  de
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource6,  GPIO_AF_LTDC);  // PG06  r7
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource7,  GPIO_AF_LTDC);  // PG07  ck
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource10, 0x09);          // PG10  g3
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource11, GPIO_AF_LTDC);  // PG11  b3
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource12, 0x09);          // PG12  b4

  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_6 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_Init (GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_Init (GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_10;
  GPIO_Init (GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_6;
  GPIO_Init (GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
  GPIO_Init (GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_Init (GPIOG, &GPIO_InitStruct);

  //{{{  config LTDC
  LTDC_InitTypeDef LTDC_InitStruct;
  LTDC_InitStruct.LTDC_HSPolarity = LTDC_HSPolarity_AL;
  LTDC_InitStruct.LTDC_VSPolarity = LTDC_VSPolarity_AL;
  LTDC_InitStruct.LTDC_DEPolarity = LTDC_DEPolarity_AL;
  LTDC_InitStruct.LTDC_PCPolarity = LTDC_PCPolarity_IIPC;
  LTDC_InitStruct.LTDC_BackgroundRedValue = 0;
  LTDC_InitStruct.LTDC_BackgroundGreenValue = 0;
  LTDC_InitStruct.LTDC_BackgroundBlueValue = 0;
  LTDC_InitStruct.LTDC_HorizontalSync = HORIZ_SYNC-1;
  LTDC_InitStruct.LTDC_AccumulatedHBP = HORIZ_SYNC-1;
  LTDC_InitStruct.LTDC_AccumulatedActiveW = HORIZ_SYNC+TFTWIDTH-1;
  LTDC_InitStruct.LTDC_TotalWidth = HORIZ_SYNC+TFTWIDTH-1;
  LTDC_InitStruct.LTDC_VerticalSync = VERT_SYNC-1;
  LTDC_InitStruct.LTDC_AccumulatedVBP = VERT_SYNC-1;
  LTDC_InitStruct.LTDC_AccumulatedActiveH = VERT_SYNC+TFTHEIGHT-1;
  LTDC_InitStruct.LTDC_TotalHeigh = VERT_SYNC+TFTHEIGHT-1;
  LTDC_Init (&LTDC_InitStruct);

  LTDC_Layer_InitTypeDef LTDC_Layer_InitStruct;
  LTDC_Layer_InitStruct.LTDC_PixelFormat = LTDC_Pixelformat_RGB565;
  LTDC_Layer_InitStruct.LTDC_ConstantAlpha = 255;
  LTDC_Layer_InitStruct.LTDC_HorizontalStart = HORIZ_SYNC;
  LTDC_Layer_InitStruct.LTDC_HorizontalStop = HORIZ_SYNC+TFTWIDTH-1;
  LTDC_Layer_InitStruct.LTDC_VerticalStart = VERT_SYNC;
  LTDC_Layer_InitStruct.LTDC_VerticalStop = VERT_SYNC+TFTHEIGHT-1;
  LTDC_Layer_InitStruct.LTDC_CFBLineLength = (TFTWIDTH*2)+3;
  LTDC_Layer_InitStruct.LTDC_CFBPitch = TFTWIDTH*2;
  LTDC_Layer_InitStruct.LTDC_CFBLineNumber = TFTHEIGHT;
  LTDC_Layer_InitStruct.LTDC_DefaultColorBlue = 0;
  LTDC_Layer_InitStruct.LTDC_DefaultColorGreen = 0;
  LTDC_Layer_InitStruct.LTDC_DefaultColorRed = 0;
  LTDC_Layer_InitStruct.LTDC_DefaultColorAlpha = 0;
  LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_CA;
  LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_CA;
  LTDC_Layer_InitStruct.LTDC_CFBStartAdress = FRAME_BUFFER;
  LTDC_LayerInit (LTDC_Layer1, &LTDC_Layer_InitStruct);

  // LTDC configuration reload
  LTDC_ReloadConfig (LTDC_IMReload);
  LTDC_LayerCmd (LTDC_Layer1, ENABLE);
  LTDC_ReloadConfig (LTDC_IMReload);
  LTDC_DitherCmd (ENABLE);
  LTDC_Cmd (ENABLE);
  //}}}
  }
//}}}
