// sdram.c
// PG08 <-> FMC_SDCLK
// PC00 <-> FMC_SDNWE
// PF11 <-> FMC_NRAS
// PG15 <-> FMC_NCAS
// PE00 <-> FMC_NBL0
// PE01 <-> FMC_NBL1
// PC02 <-> FMC_SDNE0      BANK1 address 0xC0000000
// PC03 <-> FMC_SDCKE0
// PB06 <-> FMC_SDNE1      BANK2 address 0xD0000000
// PB05 <-> FMC_SDCKE1
//                       64mbit   8mbyte 0x00800000
//                      128mbit  16mbyte 0x01000000
//                      256mbit  32mbyte 0x02000000
//                      512mbit  64mbyte 0x04000000
//                     1024mbit 128mbyte 0x08000000
//                     2048mbit 256mbyte 0x10000000
// PF00:05 <-> FMC_A00:05   PD14:15 <-> FMC_D00:01
// PF12:15 <-> FMC_A06:09   PD00:01 <-> FMC_D02:03
// PG00:01 <-> FMC_A10:11   PE07:15 <-> FMC_D04:12
// PG04:05 <-> FMC_BA0:1    PD08:10 <-> FMC_D13:15

//{{{  includes
#include "sdram.h"

#include "stm32f4xx.h"
#include "stm32f4xx_fmc.h"

#include "delay.h"
//}}}
#define FMC_COMMAND_TARGET  FMC_Command_Target_bank1_2
#define AUTOREFRESH

void sdramInit() {

  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
                          RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);
  RCC_AHB3PeriphClockCmd (RCC_AHB3Periph_FMC, ENABLE);
  //{{{  gpio AF config
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource0 , GPIO_AF_FMC);

  GPIO_PinAFConfig (GPIOD, GPIO_PinSource0,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource1,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource8,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource9,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource10, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource14, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource15, GPIO_AF_FMC);

  GPIO_PinAFConfig (GPIOE, GPIO_PinSource0,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource1,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource7,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource8,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource9,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource10, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource11, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource12, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource13, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource14, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource15, GPIO_AF_FMC);

  GPIO_PinAFConfig (GPIOF, GPIO_PinSource0,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource1,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource2,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource3,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource4,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource5,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource11, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource12, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource13, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource14, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource15, GPIO_AF_FMC);

  GPIO_PinAFConfig (GPIOG, GPIO_PinSource0,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource1,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource4,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource5,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource8,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource15, GPIO_AF_FMC);

  // PC02,PC02 - CS,CKE bank1
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource2,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource3,  GPIO_AF_FMC);

  // PB05,PB06 - CS,CKE bank2
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource5,  GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource6,  GPIO_AF_FMC);
  //}}}
  //{{{  gpio pin config
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init (GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1  | GPIO_Pin_8 |
                                GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 |
                                GPIO_Pin_15;
  GPIO_Init (GPIOD, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_7 |
                                GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 |
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init (GPIOE, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1 | GPIO_Pin_2 |
                                GPIO_Pin_3  | GPIO_Pin_4 | GPIO_Pin_5 |
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init (GPIOF, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 |
                                GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_15;
  GPIO_Init (GPIOG, &GPIO_InitStructure);

  // PC02,PC02 - CS,CKE bank1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_Init (GPIOC, &GPIO_InitStructure);

  // PB05,PB06 - CS,CKE bank2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_Init (GPIOB, &GPIO_InitStructure);
  //}}}

  FMC_SDRAMInitTypeDef FMC_SDRAMInitStructure;
  FMC_SDRAMInitStructure.FMC_Bank               = FMC_Bank2_SDRAM;
  FMC_SDRAMInitStructure.FMC_ColumnBitsNumber   = FMC_ColumnBits_Number_9b; // Row add 7:0
  FMC_SDRAMInitStructure.FMC_RowBitsNumber      = FMC_RowBits_Number_12b;   // Col add 11:0
  FMC_SDRAMInitStructure.FMC_SDMemoryDataWidth  = FMC_SDMemory_Width_16b;
  FMC_SDRAMInitStructure.FMC_InternalBankNumber = FMC_InternalBank_Number_4;
  FMC_SDRAMInitStructure.FMC_WriteProtection    = FMC_Write_Protection_Disable;
  FMC_SDRAMInitStructure.FMC_SDClockPeriod      = FMC_SDClock_Period_2;
  FMC_SDRAMInitStructure.FMC_CASLatency         = FMC_CAS_Latency_2;        // bsp says 3
  FMC_SDRAMInitStructure.FMC_ReadPipeDelay      = FMC_ReadPipe_Delay_0;     // bsp says 1
  FMC_SDRAMInitStructure.FMC_ReadBurst          = FMC_Read_Burst_Enable;    // bsp say disable
  FMC_SDRAMTimingInitTypeDef FMC_SDRAMTimingInitStructure;   // 96 Mhz SD clock = 192Mhz/2 = 10.41ns
  FMC_SDRAMTimingInitStructure.FMC_LoadToActiveDelay    = 2; // 2 TMRD: 2 clocks
  FMC_SDRAMTimingInitStructure.FMC_ExitSelfRefreshDelay = 7; // 7 TXSR: min = 70ns  = 7 x 10.41ns
  FMC_SDRAMTimingInitStructure.FMC_SelfRefreshTime      = 4; // 4 TRAS: min = 42ns  = 4 x 10.41ns max=120k(ns)
  FMC_SDRAMTimingInitStructure.FMC_RowCycleDelay        = 6; // 6 TRC:  min = 63ns  = 6 x 10.41ns
  FMC_SDRAMTimingInitStructure.FMC_WriteRecoveryTime    = 2; // 2 TWR:  min = 1+7ns = 1 + 10.41ns
  FMC_SDRAMTimingInitStructure.FMC_RPDelay              = 2; // 2 TRP:         15ns = 2 x 10.41ns
  FMC_SDRAMTimingInitStructure.FMC_RCDDelay             = 2; // 2 TRCD:        15ns = 2 x 10.41ns
  FMC_SDRAMInitStructure.FMC_SDRAMTimingStruct = &FMC_SDRAMTimingInitStructure;
  FMC_SDRAMInit (&FMC_SDRAMInitStructure);

  FMC_SDRAMInitStructure.FMC_Bank               = FMC_Bank1_SDRAM;
  FMC_SDRAMInitStructure.FMC_ColumnBitsNumber   = FMC_ColumnBits_Number_8b; // Row add 8:0
  FMC_SDRAMInit (&FMC_SDRAMInitStructure);

  //{{{  FMC_Command Mode_CLK_Enabled
  FMC_SDRAMCommandTypeDef FMC_SDRAMCommandStructure;
  FMC_SDRAMCommandStructure.FMC_CommandMode            = FMC_Command_Mode_CLK_Enabled;
  FMC_SDRAMCommandStructure.FMC_CommandTarget          = FMC_COMMAND_TARGET;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber      = 1;
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
  while (FMC_GetFlagStatus (FMC_Bank1_SDRAM, FMC_FLAG_Busy) != RESET) {}
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);
  //}}}
  delayMs (1);
  //{{{  FMC_Command precharge all
  FMC_SDRAMCommandStructure.FMC_CommandMode            = FMC_Command_Mode_PALL;
  FMC_SDRAMCommandStructure.FMC_CommandTarget          = FMC_COMMAND_TARGET;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber      = 1;
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
  while (FMC_GetFlagStatus (FMC_Bank1_SDRAM, FMC_FLAG_Busy) != RESET) {}
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);
  //}}}
  #ifdef AUTOREFRESH
    //{{{  FMC_Command autoRefresh
    FMC_SDRAMCommandStructure.FMC_CommandMode            = FMC_Command_Mode_AutoRefresh;
    FMC_SDRAMCommandStructure.FMC_CommandTarget          = FMC_COMMAND_TARGET;
    FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber      = 4;
    FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

    // first autoRefresh command
    while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
    while (FMC_GetFlagStatus (FMC_Bank1_SDRAM, FMC_FLAG_Busy) != RESET) {}
    FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

    // second autoRefresh command
    while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
    while (FMC_GetFlagStatus (FMC_Bank1_SDRAM, FMC_FLAG_Busy) != RESET) {}
    FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);
    //}}}
  #endif
  //{{{  FMC_Command loadMode
  FMC_SDRAMCommandStructure.FMC_CommandMode            = FMC_Command_Mode_LoadMode;
  FMC_SDRAMCommandStructure.FMC_CommandTarget          = FMC_COMMAND_TARGET;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber      = 1;

  // SDRAM MODEREG defines
  #define SDRAM_MODEREG_BURST_LENGTH_1          ((uint16_t)0x0000)
  #define SDRAM_MODEREG_BURST_LENGTH_2          ((uint16_t)0x0001)
  #define SDRAM_MODEREG_BURST_LENGTH_4          ((uint16_t)0x0002)
  #define SDRAM_MODEREG_BURST_LENGTH_8          ((uint16_t)0x0004)
  #define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED  ((uint16_t)0x0008)
  #define SDRAM_MODEREG_CAS_LATENCY_2           ((uint16_t)0x0020)
  #define SDRAM_MODEREG_CAS_LATENCY_3           ((uint16_t)0x0030)
  #define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE  ((uint16_t)0x0200)

  // standard, sequential
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = SDRAM_MODEREG_BURST_LENGTH_4 |
                                                         SDRAM_MODEREG_CAS_LATENCY_2 |
                                                         SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
  while (FMC_GetFlagStatus (FMC_Bank1_SDRAM, FMC_FLAG_Busy) != RESET) {}
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);
  //}}}

  #ifdef AUTOREFRESH
    // Set refreshCount = 64ms/4096 = 15.62us x 90mhz - 20
    FMC_SetRefreshCount (1386);
    while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
    while (FMC_GetFlagStatus (FMC_Bank1_SDRAM, FMC_FLAG_Busy) != RESET) {}
  #endif
  }
