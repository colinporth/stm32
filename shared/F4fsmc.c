// F4fsmc.c
//{{{  includes
#include "displayHw.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_fsmc.h"

void delayMs (__IO uint32_t ms);
//}}}
//  PD04 = NOE = RD hi - not used
//  PD05 = NWE = WR hi
//  PD07 = NE1 = CS hi
//  PD11 = A16 = DC lo
//  PE01 = RES hi
//  PD14:15 - D0:1
//  PD00:01 - D2:3
//  PE07:15 - D4:12
//  PD08:19 - D13:15
#define RES_PIN  GPIO_Pin_1  // PE1 = RES hi

static volatile uint16_t* fsmcCmd  = (volatile uint16_t*)(0x60000000);
static volatile uint16_t* fsmcData = (volatile uint16_t*)(0x60020000);

//{{{
//}}}
void writeCommand (uint16_t reg) {
  *fsmcCmd = reg;
  }

void writeData (uint16_t data) {
  *fsmcData = data;
  }

void writeCommandData (uint16_t reg, uint16_t data) {
  *fsmcCmd = reg;
  *fsmcData = data;
  }

void writeColour (uint16_t colour, uint32_t length) {
  for (int i = 0; i < length; i++)
    *fsmcData = colour;
  }

void writePixels (uint16_t* pixels, uint32_t length) {
  for (int i = 0; i < length; i++)
    *fsmcData = *pixels++;
  }

//{{{
void displayHwInit (uint16_t useOptions) {
// could use USE_16BIT_DATA to limit GPIO usage

  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);

  // setup RES_PIN
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = RES_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init (GPIOE, &GPIO_InitStruct);
  GPIOE->BSRRH = RES_PIN; // PE01 - RES lo

  // GPIOD PD0:1 4:5 7:11 14:15
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 |
                             GPIO_Pin_4 | GPIO_Pin_5 |
                             GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 |
                             GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOD, &GPIO_InitStruct);

  // GPIOE PE7:15
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 |
                             GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 |
                             GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init (GPIOE, &GPIO_InitStruct);

  GPIO_PinAFConfig (GPIOD, GPIO_PinSource0,  GPIO_AF_FSMC); // PD00 - D2
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource1,  GPIO_AF_FSMC); // PD01 - D3
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource4,  GPIO_AF_FSMC); // PD04 - NOE -> RD
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource5,  GPIO_AF_FSMC); // PD05 - NWE -> WR
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource7,  GPIO_AF_FSMC); // PD07 - NE1 -> CS
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource8,  GPIO_AF_FSMC); // PD08 - D13
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource9,  GPIO_AF_FSMC); // PD09 - D14
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource10, GPIO_AF_FSMC); // PD10 - D15
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource11, GPIO_AF_FSMC); // PD11 - A16 -> RS
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource14, GPIO_AF_FSMC); // PD14 - D0
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource15, GPIO_AF_FSMC); // PD15 - D1

  GPIO_PinAFConfig (GPIOE, GPIO_PinSource7,  GPIO_AF_FSMC); // PE07 - D4
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource8,  GPIO_AF_FSMC); // PE08 - D5
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource9,  GPIO_AF_FSMC); // PE09 - D6
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource10, GPIO_AF_FSMC); // PE10 - D7
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource11, GPIO_AF_FSMC); // PE11 - D8
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource12, GPIO_AF_FSMC); // PE12 - D9
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource13, GPIO_AF_FSMC); // PE13 - D10
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource14, GPIO_AF_FSMC); // PE14 - D11
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource15, GPIO_AF_FSMC); // PE15 - D12

  // turn FSMC clocks
  RCC_AHB3PeriphClockCmd (RCC_AHB3Periph_FSMC, ENABLE);

  // init NOR/SRAM bank 1
  FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMInitStructure.FSMC_Bank =               FSMC_Bank1_NORSRAM1;           // Bank1
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux =     FSMC_DataAddressMux_Disable;   // No mux
  FSMC_NORSRAMInitStructure.FSMC_MemoryType =         FSMC_MemoryType_SRAM;          // SRAM type
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth =    FSMC_MemoryDataWidth_16b;      // 16 bits wide
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =    FSMC_BurstAccessMode_Disable;  // No Burst
  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait =   FSMC_AsynchronousWait_Disable; // No wait
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;   // Don'tcare
  FSMC_NORSRAMInitStructure.FSMC_WrapMode =           FSMC_WrapMode_Disable;         // No wrap mode
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive =   FSMC_WaitSignalActive_BeforeWaitState; //Don't care
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation =     FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal =         FSMC_WaitSignal_Disable;        // Don't care
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode =       FSMC_ExtendedMode_Enable;       // Allow distinct Read/Write parameters
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst =         FSMC_WriteBurst_Disable;        // Don't care

  // Define Read timing parameters
  FSMC_NORSRAMTimingInitTypeDef FSMC_NORSRAMTimingInitStructureRead;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_AddressSetupTime       = 1;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_AddressHoldTime        = 0;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_DataSetupTime          = 15;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_BusTurnAroundDuration  = 0;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_CLKDivision            = 1;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_DataLatency            = 0;
  FSMC_NORSRAMTimingInitStructureRead.FSMC_AccessMode             = FSMC_AccessMode_A;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &FSMC_NORSRAMTimingInitStructureRead;

  // Define Write Timing parameters
  FSMC_NORSRAMTimingInitTypeDef FSMC_NORSRAMTimingInitStructureWrite;
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_AddressSetupTime      = 2; // 2 // 1
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_AddressHoldTime       = 0;
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_DataSetupTime         = 5; // 5 // 3
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_BusTurnAroundDuration = 0;
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_CLKDivision           = 1;
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_DataLatency           = 0;
  FSMC_NORSRAMTimingInitStructureWrite.FSMC_AccessMode            = FSMC_AccessMode_A;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &FSMC_NORSRAMTimingInitStructureWrite;

  FSMC_NORSRAMInit (&FSMC_NORSRAMInitStructure);

  // Enable FSMC
  FSMC_NORSRAMCmd (FSMC_Bank1_NORSRAM1, ENABLE);

  delayMs (1);
  GPIOE->BSRRL = RES_PIN; // PE1 - RESET hi
  delayMs (1);
}
//}}}
