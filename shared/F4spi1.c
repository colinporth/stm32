//{{{  includes
#include "displayHw.h"

#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
//}}}

// PINS  PD15 = CS   normally high
//       PD10 = RES  normally high, optional
//       PD11 = DC   normally lo, optional
#define GPIO_PINS   GPIOD
#define CS_PIN      GPIO_Pin_15
#define RES_PIN     GPIO_Pin_10
#define DC_PIN      GPIO_Pin_11

// SPI1  PA5 = SCK
//       PA6 = MISO, optional
//       PA7 = MOSI
#define SPI             SPI1
#define GPIO_SPI        GPIOA
#define GPIO_AF_SPI     GPIO_AF_SPI1
#define SPI_SCK_PIN     GPIO_Pin_5
#define SPI_SCK_SOURCE  GPIO_PinSource5
#define SPI_MISO_PIN    GPIO_Pin_6
#define SPI_MISO_SOURCE GPIO_PinSource6
#define SPI_MOSI_PIN    GPIO_Pin_7
#define SPI_MOSI_SOURCE GPIO_PinSource7

static uint16_t options;

//{{{
uint16_t readData(void) {

  if (options & USE_DC_PIN)
    return 0xFAFF;
  else {
    uint8_t buf[3] = {0x73};

    // CS lo
    GPIO_PINS->BSRRH = CS_PIN;

    for (uint16_t i = 0; i < 3; i++) {
      SPI->DR = buf[i];
      while (!(SPI->SR & SPI_I2S_FLAG_TXE));
      while (!(SPI->SR & SPI_I2S_FLAG_RXNE));
      buf[i] = SPI->DR;
    }

    // CS hi
    while ((SPI->SR & SPI_I2S_FLAG_BSY));
    GPIO_PINS->BSRRL = CS_PIN;

    printf ("readData %x %x %x\n", buf[0], buf[1], buf[2]);
    return (buf[1] << 8) | buf[2];
  }

}
//}}}
//{{{
uint16_t readStatus(void) {

  if (options & USE_DC_PIN)
    return 0xFAFF;
  else {
    uint8_t buf[3] = {0x72};

    // CS lo
    GPIO_PINS->BSRRH = CS_PIN;

    for (uint16_t i = 0; i < 3; i++) {
      SPI->DR = buf[i];
      while (!(SPI->SR & SPI_I2S_FLAG_TXE));
      while (!(SPI->SR & SPI_I2S_FLAG_RXNE));
      buf[i] = SPI->DR;
    }

    // CS hi
    while ((SPI->SR & SPI_I2S_FLAG_BSY));
    GPIO_PINS->BSRRL = CS_PIN;

    printf ("readStatus %x %x %x\n", buf[0], buf[1], buf[2]);
    return (buf[1] << 8) | buf[2];
  }

}
//}}}

//{{{
void writeCommand (uint16_t command) {

  if (options & USE_DC_PIN) {
    // CS, DC lo
    GPIO_PINS->BSRRH = CS_PIN | DC_PIN;

    SPI->DR = command;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    while ((SPI->SR & SPI_I2S_FLAG_BSY));

    // CS, DC hi
    GPIO_PINS->BSRRL = CS_PIN | DC_PIN;
  }
  else {
    uint8_t buf[3] = {0x70, 0, command};

    // CS lo
    GPIO_PINS->BSRRH = CS_PIN;

    for (int i = 0; i < 3; i++) {
      SPI->DR = buf[i];
      while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    }

    // CS hi
    while ((SPI->SR & SPI_I2S_FLAG_BSY));
    GPIO_PINS->BSRRL = CS_PIN;
  }
}
//}}}
//{{{
void writeData (uint16_t data) {

  // CS lo, assumes DC hi
  GPIO_PINS->BSRRH = CS_PIN;

  if (options & USE_DC_PIN) {
    if (options & USE_16BIT_DATA) {
      SPI->DR = data >> 8;
      while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    }
    SPI->DR = data;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
  }
  else {
    // CS lo
    uint8_t buf[3] = {0x72, data >> 8, data & 0xff};

    for (int i = 0; i < 3; i++) {
      SPI->DR = buf[i];
      while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    }
  }

  // CS hi
  while ((SPI->SR & SPI_I2S_FLAG_BSY));
  GPIO_PINS->BSRRL = CS_PIN;
}
//}}}
//{{{
void writeCommandData (uint16_t command, uint16_t data) {

  writeCommand (command);
  writeData (data);
}
//}}}

//{{{
void writeColour (uint16_t colour, uint32_t length) {

  // CS lo, assumes DC hi
  GPIO_PINS->BSRRH = CS_PIN;

  if (!(options & USE_DC_PIN)) {
    SPI->DR = 0x72;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
  }

  for (int i = 0; i < length; i++) {
    SPI->DR = colour >> 8;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    SPI->DR = colour;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
  }

  // CS hi
  while ((SPI->SR & SPI_I2S_FLAG_BSY));
  GPIO_PINS->BSRRL = CS_PIN;
}
//}}}
//{{{
void writePixels (uint16_t* pixels, uint32_t length) {

  // CS lo, assumes DC hi
  GPIO_PINS->BSRRH = CS_PIN;

  if (!(options & USE_DC_PIN)) {
    SPI->DR = 0x72;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
  }

  for (int i = 0; i < length; i++) {
    SPI->DR = *pixels >> 8;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
    SPI->DR =  *pixels++;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
  }

  // CS hi
  while ((SPI->SR & SPI_I2S_FLAG_BSY));
  GPIO_PINS->BSRRL = CS_PIN;
}
//}}}
//{{{
void writeBytes (uint8_t* bytes, uint32_t length) {

  // CS lo, assumes DC hi
  GPIO_PINS->BSRRH = CS_PIN;

  for (int i = 0; i < length; i++) {
    SPI->DR =  *bytes++;
    while (!(SPI->SR & SPI_I2S_FLAG_TXE));
  }

  // CS hi
  while ((SPI->SR & SPI_I2S_FLAG_BSY));
  GPIO_PINS->BSRRL = CS_PIN;
}
//}}}

//{{{
void displayHwInit (uint16_t useOptions) {

  options = useOptions;

  // enable clocks
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOD, ENABLE);
  RCC_APB2PeriphClockCmd (RCC_APB2Periph_SPI1, ENABLE);

  uint16_t gpioPins = CS_PIN;
  if (options & USE_RES_PIN)
    gpioPins |= RES_PIN;
  if (options & USE_DC_PIN)
    gpioPins |= DC_PIN;

  // enable CS, RES, DC pins
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = gpioPins;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init (GPIO_PINS, &GPIO_InitStruct);

  // set gpioPins hi
  GPIO_PINS->BSRRL = gpioPins;

  // enable SPI pins
  uint16_t spiPins = SPI_SCK_PIN | SPI_MOSI_PIN;
  if (options & USE_DUPLEX)
    spiPins |= SPI_MISO_PIN;
  GPIO_InitStruct.GPIO_Pin = spiPins;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIO_SPI, &GPIO_InitStruct);

  // set GPIO pins SPI alternate function
  GPIO_PinAFConfig (GPIO_SPI, SPI_SCK_SOURCE, GPIO_AF_SPI);
  GPIO_PinAFConfig (GPIO_SPI, SPI_MOSI_SOURCE, GPIO_AF_SPI);
  if (options & USE_DUPLEX)
    GPIO_PinAFConfig (GPIO_SPI, SPI_MISO_SOURCE, GPIO_AF_SPI);

  // set SPI master, 8bit, MSBfirst, NSS pin high
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_CPOL = (options & USE_MODE0) ? SPI_CPOL_Low : SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = (options & USE_MODE0) ? SPI_CPHA_1Edge : SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_Direction = (options & USE_DUPLEX) ? SPI_Direction_2Lines_FullDuplex : SPI_Direction_1Line_Tx;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // clock apb/2
  SPI_Init (SPI, &SPI_InitStructure);

  SPI_Cmd (SPI, ENABLE);

  if (options & USE_RES_PIN) {
    // strobe reset lo
    GPIO_PINS->BSRRH = RES_PIN; // RES lo
    delay_ms (10);
    GPIO_PINS->BSRRL = RES_PIN; // RES hi
    delay_ms (120);
  }
}
//}}}
