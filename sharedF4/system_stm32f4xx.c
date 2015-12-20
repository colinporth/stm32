//{{{  clocks
// -----------------------------------------------------------------------------
//         System Clock source                    | PLL (HSE)
// -----------------------------------------------------------------------------
//         SYSCLK(Hz)                             | 168000000
// -----------------------------------------------------------------------------
//         HCLK(Hz)                               | 168000000
// -----------------------------------------------------------------------------
//         AHB Prescaler                          | 1
// -----------------------------------------------------------------------------
//         APB1 Prescaler                         | 4
// -----------------------------------------------------------------------------
//         APB2 Prescaler                         | 2
// -----------------------------------------------------------------------------
//         HSE Frequency(Hz)                      | 8000000
// -----------------------------------------------------------------------------
//         PLL_M                                  | 8
// -----------------------------------------------------------------------------
//         PLL_N                                  | 336
// -----------------------------------------------------------------------------
//         PLL_P                                  | 2
// -----------------------------------------------------------------------------
//         PLL_Q                                  | 7
// -----------------------------------------------------------------------------
//         PLLI2S_N                               | 192
// -----------------------------------------------------------------------------
//         PLLI2S_R                               | 5
// -----------------------------------------------------------------------------
//         I2S input clock(Hz)                    | 38400000
// -----------------------------------------------------------------------------
//         VDD(V)                                 | 3.3
// -----------------------------------------------------------------------------
//         High Performance mode                  | Enabled
// -----------------------------------------------------------------------------
//         Flash Latency(WS)                      | 5
// -----------------------------------------------------------------------------
//         Prefetch Buffer                        | OFF
// -----------------------------------------------------------------------------
//         Instruction cache                      | ON
// -----------------------------------------------------------------------------
//         Data cache                             | ON
// -----------------------------------------------------------------------------
//         Require 48MHz for USB OTG FS,          | Enabled
//         SDIO and RNG clock                     |
// -----------------------------------------------------------------------------
//}}}
#include "stm32f4xx.h"

#define PLL_M 8   //                                                 HSE =   8Mhz
#define PLL_N 336 // PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N  = 336Mhz
#define PLL_P 2   // SYSCLK  = PLL_VCO / PLL_P                           = 168Mhz
#define PLL_Q 7   // USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ    =  48Mhz

uint32_t SystemCoreClock = 168000000;

__I uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
//{{{
void SystemCoreClockUpdate(void)
{
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
        /* HSE used as PLL clock source */
        pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      else
        /* HSI used as PLL clock source */
        pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);

      pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
      SystemCoreClock = pllvco/pllp;
      break;

    default:
      SystemCoreClock = HSI_VALUE;
      break;
    }

  /* Compute HCLK frequency, Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];

  /* HCLK frequency */
  SystemCoreClock >>= tmp;
  }
//}}}

void SystemInit(void) {
  // Config System clock source, PLL Multiplier Divider, AHB/APBx prescalers and Flash settings
  __IO uint32_t StartUpCounter = 0;
  __IO uint32_t HSEStatus = 0;

  // FPU settings
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

  // Reset the RCC clock configuration to the default reset state, Set HSION bit */
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

  // Enable HSE, Wait till HSE ready, exit if Timeout
  RCC->CR |= ((uint32_t)RCC_CR_HSEON);
  do {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;
    } while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != RESET) {
    // Enable high performance mode, System frequency up to 168 MHz
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;

    // HCLK = SYSCLK/1, PCLK2 = HCLK/2, PCLK1 = HCLK/4
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4;

    // Configure the main PLL
    RCC->PLLCFGR = PLL_M |
                  (PLL_N << 6) |
                (((PLL_P >> 1) -1) << 16) |
                  (RCC_PLLCFGR_PLLSRC_HSE) |
                  (PLL_Q << 24);

    // Enable main PLL, wait till ready
    RCC->CR |= RCC_CR_PLLON;
    while((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    // Flash prefetch, Instruction cache, Data cache, 5 wait state
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

    // Select main PLL as system clock source, wait till ready
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL) {}
    }

// Configure the Vector Table location add offset address
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE;
#else
  SCB->VTOR = FLASH_BASE;
#endif
  }
