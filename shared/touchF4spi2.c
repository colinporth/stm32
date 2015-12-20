// touchF4spi2
//{{{  includes
#include "touch.h"

#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
//}}}

// GPIO PD12 = CS  normally high
// GPIO PA1  = PENIRQ input
// SPI2 PB13 = SCK
//      PB14 = MISO
//      PB15 = MOSI
#define GPIO_CS     GPIOD
#define CS_PIN      GPIO_Pin_12

#define GPIO_PENIRQ GPIOA
#define PENIRQ_PIN  GPIO_Pin_1

#define SPI             SPI2
#define GPIO_SPI        GPIOB
#define GPIO_AF_SPI     GPIO_AF_SPI2
#define SPI_SCK_PIN     GPIO_Pin_13
#define SPI_SCK_SOURCE  GPIO_PinSource13
#define SPI_MISO_PIN    GPIO_Pin_14
#define SPI_MISO_SOURCE GPIO_PinSource14
#define SPI_MOSI_PIN    GPIO_Pin_15
#define SPI_MOSI_SOURCE GPIO_PinSource15

//{{{  touch defines
#define ADS_CTRL_PD0             (1 << 0)        /* PD0 */
#define ADS_CTRL_PD1             (1 << 1)        /* PD1 */
#define ADS_CTRL_DFR             (1 << 2)        /* SER/DFR */
#define ADS_CTRL_EIGHT_BITS_MOD  (1 << 3)        /* Mode */
#define ADS_CTRL_START           (1 << 7)        /* Start Bit */
#define ADS_CTRL_SWITCH_SHIFT    4               /* Address setting */

#define CMD_X_POSITION ((1 << ADS_CTRL_SWITCH_SHIFT) | ADS_CTRL_START | ADS_CTRL_PD0 | ADS_CTRL_PD1)
#define CMD_Y_POSITION ((5 << ADS_CTRL_SWITCH_SHIFT) | ADS_CTRL_START | ADS_CTRL_PD0 | ADS_CTRL_PD1)
#define CMD_ENABLE_PENIRQ ((1 << ADS_CTRL_SWITCH_SHIFT) | ADS_CTRL_START)
//}}}

//{{{
static uint32_t touchCommand (uint8_t command) {

  uint8_t buf[3] = {command};

  // CS lo
  GPIO_CS->BSRRH = CS_PIN;

  for (int i = 0; i < 3; i++) {
    SPI->DR = buf[i];
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    while (!(SPI->SR & SPI_I2S_FLAG_RXNE));
    buf[i] = SPI->DR;
  }

  while ((SPI->SR & SPI_I2S_FLAG_BSY));

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  return ((buf[1] << 8) | buf[2]) >> 4 ;
}
//}}}

//{{{
static uint16_t torben (uint16_t m[], int n) {

  int i, less, greater, equal;
  uint16_t  min, max, guess, maxltguess, mingtguess;

  min = max = m[0] ;
  for (i = 1 ; i < n ; i++) {
    if (m[i] < min)
      min = m[i];
    if (m[i] > max)
      max = m[i];
  }

  while (1) {
    guess = (min+max)/2;

    less = 0;
    greater = 0;
    equal = 0;
    maxltguess = min ;
    mingtguess = max ;

    for (i = 0; i < n; i++) {
      if (m[i] < guess) {
        less++;
        if (m[i]>maxltguess)
          maxltguess = m[i] ;
      } else if (m[i]>guess) {
        greater++;
        if (m[i]<mingtguess)
          mingtguess = m[i] ;
      } else
        equal++;
    }

    if (less <= (n+1)/2 && greater <= (n+1)/2)
      break ;
    else if (less>greater)
      max = maxltguess ;
    else
      min = mingtguess;
  }

  if (less >= (n+1)/2)
    return maxltguess;
  else if (less+equal >= (n+1)/2)
    return guess;
  else
    return mingtguess;
}
//}}}

//{{{
bool getTouchDown() {

  return (GPIO_PENIRQ->IDR & PENIRQ_PIN) == 0;
}
//}}}
//{{{
void getTouchPos (int16_t* x, int16_t* y, int16_t* z, int16_t xlen, uint16_t ylen) {

  #define maxSamples 7
  uint16_t xsamples[maxSamples], ysamples[maxSamples];

  uint16_t samples = 0;
  while (samples < maxSamples) {
    ysamples[samples] = touchCommand (CMD_Y_POSITION);
    xsamples[samples] = touchCommand (CMD_X_POSITION);
    samples++;
  }

  int16_t xsample = torben (xsamples, samples);
  int16_t ysample = torben (ysamples, samples);

  *x = -10 +  (ysample * xlen) / 1900;
  *y = -10 + ((2048 - xsample) * ylen)/ 1900;

  touchCommand (CMD_ENABLE_PENIRQ);
}
//}}}

//{{{
bool touchInit() {

  // clocks
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD, ENABLE);
  RCC_APB1PeriphClockCmd (RCC_APB1Periph_SPI2, ENABLE);

  // enable PENIRQ GPIO and pins
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = PENIRQ_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIO_PENIRQ, &GPIO_InitStruct);

  // enable CS GPIO and pins
  GPIO_InitStruct.GPIO_Pin = CS_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init (GPIO_CS, &GPIO_InitStruct);

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  // enable SPI and pins
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
  GPIO_InitStruct.GPIO_Pin = SPI_SCK_PIN | SPI_MISO_PIN |SPI_MOSI_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIO_SPI, &GPIO_InitStruct);

  // set SPI pins alternate function
  GPIO_PinAFConfig (GPIO_SPI, SPI_SCK_SOURCE, GPIO_AF_SPI);
  GPIO_PinAFConfig (GPIO_SPI, SPI_MISO_SOURCE, GPIO_AF_SPI);
  GPIO_PinAFConfig (GPIO_SPI, SPI_MOSI_SOURCE, GPIO_AF_SPI);

  // master, mode3, 1 wire tx, 8bit, MSBfirst, NSS pin high
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
  SPI_Init (SPI, &SPI_InitStructure);

  SPI_Cmd (SPI, ENABLE);

  touchCommand (CMD_ENABLE_PENIRQ);

  return true;
  }
//}}}
