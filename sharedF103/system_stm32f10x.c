#include "stm32f10x.h"

uint32_t SystemCoreClock = 72000000; // 72Mhz

__I uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
//{{{
void SystemCoreClockUpdate (void) {

  uint32_t tmp = 0;
  uint32_t pllmull = 0;
  uint32_t pllsource = 0;

  // Get SYSCLK source
  tmp = RCC->CFGR & RCC_CFGR_SWS;
  switch (tmp) {
    case 0x00:  // HSI
      SystemCoreClock = HSI_VALUE;
      break;

    case 0x04:  // HSE
      SystemCoreClock = HSE_VALUE;
      break;

    case 0x08:  // PLL
      // Get PLL clock source and multiplication factor
      pllmull = RCC->CFGR & RCC_CFGR_PLLMULL;
      pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;
      pllmull = ( pllmull >> 18) + 2;

      if (pllsource == 0x00)
        // HSI oscillator clock divided by 2 selected as PLL clock entry
        SystemCoreClock = (HSI_VALUE >> 1) * pllmull;
      else {
         // HSE selected as PLL clock entry
        if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) != (uint32_t)RESET)
          // HSE oscillator clock divided by 2
          SystemCoreClock = (HSE_VALUE >> 1) * pllmull;
        else
          SystemCoreClock = HSE_VALUE * pllmull;
        }
      break;

    default:
      SystemCoreClock = HSI_VALUE;
      break;
    }

  // Compute HCLK clock frequency, Get HCLK prescaler
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];

  // HCLK clock frequency
  SystemCoreClock >>= tmp;
  }
//}}}

// #define VECT_TAB_SRAM
#define VECT_TAB_OFFSET  0x0 /*!< Vector Table base offset field, must be a multiple of 0x200. */

// Setup microcontroller system initEmbedded Flash Interface, PLL, update SystemCoreClock
void SystemInit (void) {

  // config System clock freq, HCLK, PCLK2, PCLK1 prescalers, Flash Latency enable prefetch buffer
  __IO uint32_t StartUpCounter = 0;
  __IO uint32_t HSEStatus = 0;

  // Reset the RCC clock configuration to the default reset state(for debug purpose), Set HSION bit
  RCC->CR |= (uint32_t)0x00000001;

  // Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits
  RCC->CFGR &= (uint32_t)0xF8FF0000;

  // Reset HSEON, CSSON and PLLON bits
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  // Reset HSEBYP bit
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  // Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits
  RCC->CFGR &= (uint32_t)0xFF80FFFF;

  // Disable all interrupts and clear pending bits
  RCC->CIR = 0x009F0000;

  // SYSCLK, HCLK, PCLK2 and PCLK1 configuration, Enable HSE
  RCC->CR |= (uint32_t)RCC_CR_HSEON;

  // Wait till HSE is ready and if Time out is reached exit
  do {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;
    } while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != RESET) {
    // Enable Flash Prefetch Buffer, 2 wait state
    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;

    // HCLK = SYSCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

    // PCLK2 = HCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;

    // PCLK1 = HCLK
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

    // PLLCLK = HSE * 6 = 72 MHz
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL6);

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;

    // Wait till PLL is ready
    while((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    // Select PLL as system clock source
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

    // Wait till PLL is used as system clock source
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) {}
    }

// We disable the vector table mapping if we actively debug because we are doing this in our
// debugger script. By doing this in the debugger script, we can switch between flash or ram execution.
// For the release version, please enable the code below by undefining __DONT_INIT_VTABLE
// Configure the Vector Table location add offset address ------------------*/
// If you work ith the STlink for debug, please don't use this.             */
#ifndef __DONT_INIT_VTABLE
  #ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
  #else
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
  #endif
#else
#warning For release version do not use __DONT_INIT_VTABLE (see above).
#endif
  }
