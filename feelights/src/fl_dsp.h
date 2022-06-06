#ifndef FL_DSP_H__
#define FL_DSP_H__

#include "fl_common.h"

u32 DspCalculateSpectrum(f32 *Input, u32 NumSamples, f32 *IntermediateBuffer, f32 *Output);

u32 DspNormalizeSamples(u16 *RawSamples, u32 NumSamples, f32 *Output);

#endif /* FL_DSP_H__ */
