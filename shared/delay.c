// delay.c
#include "delay.h"

#ifdef STM32F10X_HD
#include "stm32f10x.h"
#else
#include "stm32f4xx.h"
#endif

static __IO uint32_t DelayCount;

void delayMs (uint32_t ms) {

  DelayCount = ms;
  while (DelayCount != 0) {}
  }

void SysTick_Handler() {

  if (DelayCount != 0x00)
    DelayCount--;
  }

void delayInit() {
  SysTick->VAL = 0;                       // config sysTick
  SysTick->LOAD = SystemCoreClock / 1000; // - countdown value
  SysTick->CTRL = 0x7;                    // - 0x4 = sysTick HCLK, 0x2 = intEnable, 0x1 = enable
  }
