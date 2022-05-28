#ifndef FL_LIGHTS_H__
#define FL_LIGHTS_H__

#include "fl_common.h"
#include "fl_strip.h"

u32 LightsInit();

void LightsUpdateAndRender(pixel *Pixels, u32 NumPixels, f32 *Spectrum, u32 NumSamples);

#endif /* FL_LIGHTS_H__ */
