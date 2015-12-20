#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct image {
  uint8_t width;
  uint8_t height;
  uint16_t* pixels;
} image_t;

#ifdef __cplusplus
  extern "C" {
#endif

image_t* getImage (uint16_t image);

#ifdef __cplusplus
  }
#endif
