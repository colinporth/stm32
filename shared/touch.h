#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

bool getTouchDown();
void getTouchPos (int16_t* x, int16_t* y, int16_t* z, int16_t xlen, uint16_t ylen);

bool touchInit();

#ifdef __cplusplus
  }
#endif
