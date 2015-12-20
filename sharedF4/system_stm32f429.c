// system_stm32f429.c
#include "stm32f4xx.h"
//#define SDRAM_INIT
#ifdef SDRAM_INIT
  // PD00 = FMC_D2   | PE00 = FMC_NBL0  | PF00 = FMC_A0    | PG00 = FMC_A10   | PB5 = FMC_SDCKE1   |
  // PD01 = FMC_D3   | PE01 = FMC_NBL1  | PF01 = FMC_A1    | PG01 = FMC_A11   | PB6 = FMC_SDNE1    |
  // PD08 = FMC_D13  | PE07 = FMC_D4    | PF02 = FMC_A2    | PG04 = FMC_BA0   | PC0 = FMC_SDNWE    |
  // PD09 = FMC_D14  | PE08 = FMC_D5    | PF03 = FMC_A3    | PG05 = FMC_BA1   |--------------------+
  // PD10 = FMC_D15  | P0E9 = FMC_D6    | PF04 = FMC_A4    | PG08 = FMC_SDCLK |
  // PD14 = FMC_D0   | PE10 = FMC_D7    | PF05 = FMC_A5    | PG15 = FMC_NCAS  |
  // PD15 = FMC_D1   | PE11 = FMC_D8    | PF11 = FMC_NRAS  |------------------+
  //-----------------| PE12 = FMC_D9    | PF12 = FMC_A6    |
  //                 | PE13 = FMC_D10   | PF13 = FMC_A7    |
  //                 | PE14 = FMC_D11   | PF14 = FMC_A8    |
  //                 | PE15 = FMC_D12   | PF15 = FMC_A9    |
  // FMC_SDRAMInitStructure.FMC_Bank                       = FMC_Bank2_SDRAM;
  // FMC_SDRAMInitStructure.FMC_ColumnBitsNumber           = FMC_ColumnBits_Number_8b;
  // FMC_SDRAMInitStructure.FMC_RowBitsNumber              = FMC_RowBits_Number_12b;
  // FMC_SDRAMInitStructure.FMC_SDMemoryDataWidth          = FMC_SDMemory_Width_16b;
  // FMC_SDRAMInitStructure.FMC_InternalBankNumber         = FMC_InternalBank_Number_4;
  // FMC_SDRAMInitStructure.FMC_CASLatency                 = FMC_CAS_Latency_3;
  // FMC_SDRAMInitStructure.FMC_WriteProtection            = FMC_Write_Protection_Disable;
  // FMC_SDRAMInitStructure.FMC_SDClockPeriod              = FMC_SDClock_Period_2;
  // FMC_SDRAMInitStructure.FMC_ReadBurst                  = FMC_Read_Burst_Disable;
  // FMC_SDRAMInitStructure.FMC_ReadPipeDelay              = FMC_ReadPipe_Delay_1;
  // FMC_SDRAMTimingInitStructure.FMC_LoadToActiveDelay    = 2;
  // FMC_SDRAMTimingInitStructure.FMC_ExitSelfRefreshDelay = 7;
  // FMC_SDRAMTimingInitStructure.FMC_SelfRefreshTime      = 4;
  // FMC_SDRAMTimingInitStructure.FMC_RowCycleDelay        = 7;
  // FMC_SDRAMTimingInitStructure.FMC_WriteRecoveryTime    = 2;
  // FMC_SDRAMTimingInitStructure.FMC_RPDelay              = 2;
  // FMC_SDRAMTimingInitStructure.FMC_RCDDelay             = 2;
  // something in SystemInit_ExtMemCtl gets zapped
  #pragma GCC optimize ("O0")
#endif

#define VECT_TAB_OFFSET  0x00

// PLL_VCO = (HSE_VALUE / PLL_M) * PLL_N
// SYSCLK = PLL_VCO / PLL_P
// USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ
#define PLL_M  8
#define PLL_P  2

#define SYSCLK_192MHZ
#ifdef SYSCLK_192MHZ
  #define PLL_N  384 // 192Mhz
  #define PLL_Q  8   // 192Mhz/8 = 24mhz
  uint32_t SystemCoreClock = 192000000;
#else
  #define PLL_N  360 // 180mhz
  #define PLL_Q  7   // 180/7 = 24mhz
  uint32_t SystemCoreClock = 180000000;
#endif

uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
//{{{
void SystemCoreClockUpdate() {

  uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;

  // Get SYSCLK source
  tmp = RCC->CFGR & RCC_CFGR_SWS;

  switch (tmp) {
    case 0x00:  // HSI used as system clock source
      SystemCoreClock = HSI_VALUE;
      break;
    case 0x04:  // HSE used as system clock source
      SystemCoreClock = HSE_VALUE;
      break;
    case 0x08:  // PLL used as system clock source

      // PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
      // SYSCLK = PLL_VCO / PLL_P
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
      pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

      if (pllsource != 0)
        // HSE used as PLL clock source
        pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      else
        // HSI used as PLL clock source
        pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);

      pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
      SystemCoreClock = pllvco/pllp;
      break;

    default:
      SystemCoreClock = HSI_VALUE;
      break;
    }

  // Compute HCLK frequency, Get HCLK prescaler
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];

  // HCLK frequency
  SystemCoreClock >>= tmp;
}
//}}}
//{{{
void SystemInit() {

  // FPU settings
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  // set CP10 and CP11 Full Access
  #endif

  // Reset the RCC clock configuration to the default reset state
  // Set HSION bit
  RCC->CR |= (uint32_t)0x00000001;

  // Reset CFGR register
  RCC->CFGR = 0x00000000;

  // Reset HSEON, CSSON and PLLON bits
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  // Reset PLLCFGR register
  RCC->PLLCFGR = 0x24003010;

  // Reset HSEBYP bit
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  // Disable all interrupts
  RCC->CIR = 0x00000000;

  #ifdef SDRAM_INIT
  // Enable GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG clocks
  RCC->AHB1ENR |= 0x0000007E;

  // Connect PBx pins to FMC Alternate function
  GPIOB->AFR[0]  = 0x0CC00000;
  GPIOB->AFR[1]  = 0x00000000;
  // Configure PBx pins in Alternate function mode
  GPIOB->MODER   = 0x00002A80;
  // Configure PBx pins speed to 50 MHz
  GPIOB->OSPEEDR = 0x000028C0;
  // Configure PBx pins Output type to push-pull
  GPIOB->OTYPER  = 0x00000000;
  // No pull-up, pull-down for PBx pins
  GPIOB->PUPDR   = 0x00000100;

  // Connect PCx pins to FMC Alternate function
  GPIOC->AFR[0]  = 0x0000000C;
  GPIOC->AFR[1]  = 0x00000000;
  // Configure PCx pins in Alternate function mode
  GPIOC->MODER   = 0x00000002;
  // Configure PCx pins speed to 50 MHz
  GPIOC->OSPEEDR = 0x00000002;
  // Configure PCx pins Output type to push-pull
  GPIOC->OTYPER  = 0x00000000;
  // No pull-up, pull-down for PCx pins
  GPIOC->PUPDR   = 0x00000000;

  // Connect PDx pins to FMC Alternate function
  GPIOD->AFR[0]  = 0x000000CC;
  GPIOD->AFR[1]  = 0xCC000CCC;
  // Configure PDx pins in Alternate function mode
  GPIOD->MODER   = 0xA02A000A;
  // Configure PDx pins speed to 50 MHz
  GPIOD->OSPEEDR = 0xA02A000A;
  // Configure PDx pins Output type to push-pull
  GPIOD->OTYPER  = 0x00000000;
  // No pull-up, pull-down for PDx pins
  GPIOD->PUPDR   = 0x00000000;

  // Connect PEx pins to FMC Alternate function
  GPIOE->AFR[0]  = 0xC00000CC;
  GPIOE->AFR[1]  = 0xCCCCCCCC;
  // Configure PEx pins in Alternate function mode
  GPIOE->MODER   = 0xAAAA800A;
  // Configure PEx pins speed to 50 MHz
  GPIOE->OSPEEDR = 0xAAAA800A;
  // Configure PEx pins Output type to push-pull
  GPIOE->OTYPER  = 0x00000000;
  // No pull-up, pull-down for PEx pins
  GPIOE->PUPDR   = 0x00000000;

  // Connect PFx pins to FMC Alternate function
  GPIOF->AFR[0]  = 0x00CCCCCC;
  GPIOF->AFR[1]  = 0xCCCCC000;
  // Configure PFx pins in Alternate function mode
  GPIOF->MODER   = 0xAA800AAA;
  // Configure PFx pins speed to 50 MHz
  GPIOF->OSPEEDR = 0xAA800AAA;
  // Configure PFx pins Output type to push-pull
  GPIOF->OTYPER  = 0x00000000;
  // No pull-up, pull-down for PFx pins
  GPIOF->PUPDR   = 0x00000000;

  // Connect PGx pins to FMC Alternate function
  GPIOG->AFR[0]  = 0x00CC00CC;
  GPIOG->AFR[1]  = 0xC000000C;
  // Configure PGx pins in Alternate function mode
  GPIOG->MODER   = 0x80020A0A;
  // Configure PGx pins speed to 50 MHz
  GPIOG->OSPEEDR = 0x80020A0A;
  // Configure PGx pins Output type to push-pull
  GPIOG->OTYPER  = 0x00000000;
  // No pull-up, pull-down for PGx pins
  GPIOG->PUPDR   = 0x00000000;

  // Enable the FMC interface clock
  RCC->AHB3ENR |= 0x00000001;

  // Configure and enable SDRAM bank2
  FMC_Bank5_6->SDCR[0] = 0x00002800;
  FMC_Bank5_6->SDCR[1] = 0x000001D4;
  FMC_Bank5_6->SDTR[0] = 0x00106000;
  FMC_Bank5_6->SDTR[1] = 0x00010361;

  // Clock enable command - BANK2 0x08, BANK1 0x10
  FMC_Bank5_6->SDCMR = 0x08 | 0x01;

  register uint32_t timeout = 0xFFFF;
  while ((FMC_Bank5_6->SDSR & 0x00000020) && timeout--) {}

  // Delay
  for (timeout = 0; timeout < 1000; timeout++) {}

  // PALL command
  FMC_Bank5_6->SDCMR = 0x08 | 0x02;
  timeout = 0xFFFF;
  while ((FMC_Bank5_6->SDSR & 0x00000020) && timeout--) {}

  // Auto refresh command
  FMC_Bank5_6->SDCMR = 0x08 | 0x03 | 0x60;
  timeout = 0xFFFF;
  while ((FMC_Bank5_6->SDSR & 0x00000020) && timeout--) {}

  // MRD register program
  FMC_Bank5_6->SDCMR = 0x08 | 0x04 | 0x00046200;
  timeout = 0xFFFF;
  while ((FMC_Bank5_6->SDSR & 0x00000020) && timeout--) {}

  // Set refresh count, 64 ms / 8192 rows = 7.81us, (7.81us * 90 MHz) - 20 = 682.9
  FMC_Bank5_6->SDRTR |= 683 << 1;

  // Disable write protection
  FMC_Bank5_6->SDCR[1] &= 0xFFFFFDFF;
  #endif // SDRAM init

  // Config System clock source, PLL Multiplier, Divider factors, AHB/APBx prescalers Flash settings
  RCC->CR |= ((uint32_t)RCC_CR_HSEON); // Enable HSE

  // Wait till HSE is ready and if Time out is reached exit
  uint32_t StartUpCounter = 0;
  uint32_t HSEStatus = 0;
  do {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;
    } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != RESET)
    HSEStatus = (uint32_t)0x01;
  else
    HSEStatus = (uint32_t)0x00;

  if (HSEStatus == (uint32_t)0x01) {
    // Configure the main PLL
    RCC->PLLCFGR = PLL_M |
                  (PLL_N << 6) |
                (((PLL_P >> 1) -1) << 16) |
                  (RCC_PLLCFGR_PLLSRC_HSE) |
                  (PLL_Q << 24);

    // Select regulator voltage output Scale 1 mode, System frequency up to 180 MHz
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;

    // HCLK = SYSCLK / 1
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

    // PCLK2 = HCLK / 2
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

    // PCLK1 = HCLK / 4
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

    // Enable the main PLL
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    // Enable the Over-drive to extend the clock frequency to 180 Mhz
    PWR->CR |= PWR_CR_ODEN;
    while ((PWR->CSR & PWR_CSR_ODRDY) == 0) {}

    PWR->CR |= PWR_CR_ODSWEN;
    while ((PWR->CSR & PWR_CSR_ODSWRDY) == 0) {}

    // Configure Flash prefetch, Instruction cache, Data cache and wait state
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

    // Select the main PLL as system clock source
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL) {}
    }

  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
  }
//}}}
