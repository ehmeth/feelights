#ifndef FL_STRIP_H__
#define FL_STRIP_H__

#include "fl_common.h"
#include <drivers/led_strip.h>

typedef union {
   u32 Dword;
   struct led_rgb Color;
} pixel;

u32 StripInit();

u32 StripOutput(pixel *Pixels, u32 NumOfPixels);

pixel* StripGetBuffer();

pixel* StripSwapBuffer(pixel *PixelBuffer);

#endif /* FL_STRIP_H__ */
