// displayTrueType.c
#include "display.h"
#include "displayHw.h"

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library library;
static FT_Face face;
static FT_GlyphSlot slot;

//{{{
void setTTFont (const uint8_t* ttFont, int length) {

  FT_Init_FreeType (&library);
  FT_New_Memory_Face (library, (FT_Byte*)ttFont, length, 0, &face);
  slot = face->glyph;

  //FT_Done_Face(face);
  //FT_Done_FreeType (library);
  }
//}}}
//{{{
void drawTTString (uint16_t colour, int height, const char* str,
                   int16_t xorg, int16_t yorg, uint16_t xlen, uint16_t ylen) {

  FT_Set_Pixel_Sizes (face, 0, height);

  setBitPixels (!colour, xorg, yorg, xlen, ylen);

  while (*str) {
    FT_Load_Char (face, *str++, getMono() ? (FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) : FT_LOAD_RENDER);
    if (slot->bitmap.buffer)
      setTTPixels (colour, slot->bitmap.buffer,
                   xorg + slot->bitmap_left, yorg + height - slot->bitmap_top,
                   slot->bitmap.pitch, slot->bitmap.rows);
    xorg += slot->advance.x/64;
    }

  drawLines (yorg, yorg+ylen-1);
  }
//}}}
