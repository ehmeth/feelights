#include "fl_common.h"
#include "fl_dsp.h"
#include "arm_math.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(dsp);

u32 DspCalculateSpectrum(f32 *Input, u32 NumSamples, f32 *IntermediateBuffer, f32 *Output)
{
   arm_rfft_fast_instance_f32 fft;

   arm_rfft_fast_init_f32(&fft, NumSamples);
   arm_rfft_fast_f32(&fft, Input, IntermediateBuffer, 0);
   arm_cmplx_mag_f32(IntermediateBuffer, Output, NumSamples/2);

   /* TODO(kleindan) Check for error from CMSIS */
   return 0;
}

u32 DspNormalizeSamples(u16 *RawSamples, u32 NumSamples, f32 *Output)
{
   u32 SamplesSum = 0;
   f32 Mean = 0;
   const f32 NormalizationFactor = 1.0/2000.0;

   for (int i = 0; i < NumSamples; ++i)
   {
      SamplesSum += RawSamples[i];
   }
   Mean = ((f32) SamplesSum) / ((f32) NumSamples);

   for (int i = 0; i < NumSamples; ++i)
   {
      Output[i] = NormalizationFactor * (((f32) RawSamples[i]) - Mean);
   }

   /* TODO(kleindan) Check for error from CMSIS */
   return 0;
}

