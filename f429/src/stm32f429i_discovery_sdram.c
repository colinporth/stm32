// sdram.c - info only
// +-------------------+--------------------+--------------------+--------------------+
// | PD0  <-> FMC_D2   | PE0  <-> FMC_NBL0  | PF0  <-> FMC_A0    | PG0  <-> FMC_A10   |
// | PD1  <-> FMC_D3   | PE1  <-> FMC_NBL1  | PF1  <-> FMC_A1    | PG1  <-> FMC_A11   |
// | PD8  <-> FMC_D13  | PE7  <-> FMC_D4    | PF2  <-> FMC_A2    | PG8  <-> FMC_SDCLK |
// | PD9  <-> FMC_D14  | PE8  <-> FMC_D5    | PF3  <-> FMC_A3    | PG15 <-> FMC_NCAS  |
// | PD10 <-> FMC_D15  | PE9  <-> FMC_D6    | PF4  <-> FMC_A4    |--------------------+
// | PD14 <-> FMC_D0   | PE10 <-> FMC_D7    | PF5  <-> FMC_A5    |
// | PD15 <-> FMC_D1   | PE11 <-> FMC_D8    | PF11 <-> FMC_NRAS  |
// +-------------------| PE12 <-> FMC_D9    | PF12 <-> FMC_A6    |
//                     | PE13 <-> FMC_D10   | PF13 <-> FMC_A7    |
//                     | PE14 <-> FMC_D11   | PF14 <-> FMC_A8    |
//                     | PE15 <-> FMC_D12   | PF15 <-> FMC_A9    |
// +-------------------+--------------------+--------------------+
// | PB5 <-> FMC_SDCKE1|
// | PB6 <-> FMC_SDNE1 |
// | PC0 <-> FMC_SDNWE |
// +-------------------+

#include "stm32f4xx.h"
#include "stm32f4xx_fmc.h"
void delayMs (__IO uint32_t ms);

#define SDRAM_BANK_ADDR  ((uint32_t)0xD0000000)

void SDRAM_Init() {

  // Enable GPIOs clock
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
                          RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);

  // Common GPIO configuration
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  //{{{  GPIOB configuration
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource5 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource6 , GPIO_AF_FMC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5  | GPIO_Pin_6;
  GPIO_Init (GPIOB, &GPIO_InitStructure);
  //}}}
  //{{{  GPIOC configuration
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource0 , GPIO_AF_FMC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init (GPIOC, &GPIO_InitStructure);
  //}}}
  //{{{  GPIOD configuration
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource0, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource1, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource8, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource9, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource10, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource14, GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource15, GPIO_AF_FMC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1  | GPIO_Pin_8 |
                                GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 |
                                GPIO_Pin_15;
  GPIO_Init (GPIOD, &GPIO_InitStructure);
  //}}}
  //{{{  GPIOE configuration
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource0 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource1 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource7 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource8 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource9 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource10 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource11 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource12 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource13 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource14 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource15 , GPIO_AF_FMC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_7 |
                                GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 |
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init (GPIOE, &GPIO_InitStructure);
  //}}}
  //{{{  GPIOF configuration
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource0 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource1 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource2 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource3 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource4 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource5 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource11 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource12 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource13 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource14 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOF, GPIO_PinSource15 , GPIO_AF_FMC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1 | GPIO_Pin_2 |
                                GPIO_Pin_3  | GPIO_Pin_4 | GPIO_Pin_5 |
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init (GPIOF, &GPIO_InitStructure);
  //}}}
  //{{{  GPIOG configuration
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource0 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource1 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource4 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource5 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource8 , GPIO_AF_FMC);
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource15 , GPIO_AF_FMC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 |
                                GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_15;
  GPIO_Init (GPIOG, &GPIO_InitStructure);
  //}}}

  // Enable FMC clock
  RCC_AHB3PeriphClockCmd (RCC_AHB3Periph_FMC, ENABLE);

  FMC_SDRAMInitTypeDef FMC_SDRAMInitStructure;
  FMC_SDRAMInitStructure.FMC_ColumnBitsNumber   = FMC_ColumnBits_Number_8b; // Row addressing: [7:0]
  FMC_SDRAMInitStructure.FMC_RowBitsNumber      = FMC_RowBits_Number_12b;   // Col addressing: [11:0]
  FMC_SDRAMInitStructure.FMC_SDMemoryDataWidth  = FMC_SDMemory_Width_16b;
  FMC_SDRAMInitStructure.FMC_InternalBankNumber = FMC_InternalBank_Number_4;
  FMC_SDRAMInitStructure.FMC_CASLatency         = FMC_CAS_Latency_3;
  FMC_SDRAMInitStructure.FMC_WriteProtection    = FMC_Write_Protection_Disable;
  FMC_SDRAMInitStructure.FMC_SDClockPeriod      = FMC_SDClock_Period_2;
  FMC_SDRAMInitStructure.FMC_ReadBurst          = FMC_Read_Burst_Disable;
  FMC_SDRAMInitStructure.FMC_ReadPipeDelay      = FMC_ReadPipe_Delay_1;
  FMC_SDRAMInitStructure.FMC_Bank               = FMC_Bank2_SDRAM;       // FMC SDRAM control configuration

  // FMC SDRAM Bank configuration - 180Mhz/2 = 90 Mhz SD clock
  FMC_SDRAMTimingInitTypeDef FMC_SDRAMTimingInitStructure;
  FMC_SDRAMTimingInitStructure.FMC_LoadToActiveDelay    = 2; // TMRD: 2 Clock cycles
  FMC_SDRAMTimingInitStructure.FMC_ExitSelfRefreshDelay = 7; // TXSR: min = 70ns (7 x 11.10ns)
  FMC_SDRAMTimingInitStructure.FMC_SelfRefreshTime      = 4; // TRAS: min = 42ns (4 x 11.10ns) max=120k (ns)
  FMC_SDRAMTimingInitStructure.FMC_RowCycleDelay        = 7; // TRC:  min = 63ns (7 x 11.10ns)
  FMC_SDRAMTimingInitStructure.FMC_WriteRecoveryTime    = 2; // TWR:  2 Clock cycles
  FMC_SDRAMTimingInitStructure.FMC_RPDelay              = 2; // TRP:  15ns => 2 x 11.10ns
  FMC_SDRAMTimingInitStructure.FMC_RCDDelay             = 2; // TRCD: 15ns => 2 x 11.10ns

  FMC_SDRAMInitStructure.FMC_SDRAMTimingStruct = &FMC_SDRAMTimingInitStructure;
  FMC_SDRAMInit (&FMC_SDRAMInitStructure);

  // Config clock configuration enable command
  FMC_SDRAMCommandTypeDef FMC_SDRAMCommandStructure;
  FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_CLK_Enabled;
  FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 1;
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

  // Wait until the SDRAM controller is ready
  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}

  // Send the command
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

  delayMs (100);

  // Configure a PALL(precharge all) command
  FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_PALL;
  FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 1;
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

  // Wait until the SDRAM controller is ready
  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}

  // Send the command
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

  // Configure a Auto-Refresh command
  FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_AutoRefresh;
  FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 4;
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition = 0;

  // Wait until the SDRAM controller is ready
  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}

  // Send the first command
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

  // Wait until the SDRAM controller is ready
  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}

  // Send the second command
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

  // Program the external memory mode register
  #define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
  #define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
  #define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
  #define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
  #define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
  #define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
  #define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
  #define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
  #define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
  #define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
  #define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

  // Configure a load Mode register command
  FMC_SDRAMCommandStructure.FMC_CommandMode = FMC_Command_Mode_LoadMode;
  FMC_SDRAMCommandStructure.FMC_CommandTarget = FMC_Command_Target_bank2;
  FMC_SDRAMCommandStructure.FMC_AutoRefreshNumber = 1;
  FMC_SDRAMCommandStructure.FMC_ModeRegisterDefinition =
    (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2          |
              SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
              SDRAM_MODEREG_CAS_LATENCY_3           |
              SDRAM_MODEREG_OPERATING_MODE_STANDARD |
              SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  // Wait until the SDRAM controller is ready
  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}

  // Send the command
  FMC_SDRAMCmdConfig (&FMC_SDRAMCommandStructure);

  // Set the refresh rate counter (7.81 us x Freq) - 20
  FMC_SetRefreshCount (683);

  // Wait until the SDRAM controller is ready
  while (FMC_GetFlagStatus (FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET) {}
  }
