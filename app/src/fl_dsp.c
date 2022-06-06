#include "fl_common.h"
#include "fl_dsp.h"
#include "arm_math.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(dsp);


u32 DspCalculateSpectrum(f32 *Input, u32 NumSamples, f32 *IntermediateBuffer, f32 *Output)
{
   const f32 FfftScalingFactor = 1.0f / 453.352f;
   arm_rfft_fast_instance_f32 fft;

   arm_rfft_fast_init_f32(&fft, NumSamples);
   arm_rfft_fast_f32(&fft, Input, IntermediateBuffer, 0);
   arm_cmplx_mag_f32(IntermediateBuffer, Output, NumSamples/2);

   static f32 MaxSpectrum = -32000.0f;
   static f32 MinSpectrum = 32000.0f;
   for (int i = 1; i < NumSamples/2; ++i)
   {
      Output[i] *= FfftScalingFactor;
      if (Output[i] > MaxSpectrum) MaxSpectrum = Output[i];
      if (Output[i] < MinSpectrum) MinSpectrum = Output[i];
   }
#if 0
   static u32 DebugCount = 0;
   if (DebugCount++ > 100)
   {
      LOG_INF("MinS: %d, MaxS: %d:", (i32)(MinSpectrum * 1000.0f), (i32)(MaxSpectrum * 1000.0f));
      DebugCount = 0;
   }
#endif
   

   /* TODO(kleindan) Check for error from CMSIS */
   return 0;
}

u32 DspNormalizeSamples(u16 *RawSamples, u32 NumSamples, f32 *Output)
{
   u16 MinSample = 0xffff;
   u16 MaxSample = 0x0000;
   u32 SamplesSum = 0;
   f32 MinSampleF = 0.0f;
   f32 MaxSampleF = 0.0f;
   f32 Mean = 0;

   for (int i = 0; i < NumSamples; ++i)
   {
      SamplesSum += RawSamples[i];
      if (RawSamples[i] < MinSample) MinSample = RawSamples[i];
      if (RawSamples[i] > MaxSample) MaxSample = RawSamples[i];
   }
   Mean = ((f32) SamplesSum) / ((f32) NumSamples);

#if 0
   static u32 DebugCount = 0;
   if (DebugCount++ > 100)
   {
      LOG_INF("Min: %d, Max: %d:, Mean %d", MinSample, MaxSample, (u32)(Mean * 1000.f));
      DebugCount = 0;
   }
#endif

   u16 RangeLo = (u16)Mean - MinSample;
   u16 RangeHi = MaxSample - (u16)Mean;
   u16 Range = Maximum(RangeLo, RangeHi);
   f32 NormalizationFactor = 1.0/Clamp(800.0f, (f32)Range, 2048);
   for (int i = 0; i < NumSamples; ++i)
   {
      Output[i] = NormalizationFactor * (((f32) RawSamples[i]) - Mean);
      if (Output[i] < MinSampleF) MinSampleF = Output[i];
      if (Output[i] > MaxSampleF) MaxSampleF = Output[i];
   }
#if 0
   static u32 DebugCount = 0;
   if (DebugCount++ > 100)
   {
      LOG_INF("Minf: %d, Maxf: %d:", (i32)(MinSampleF * 1000.0f), (i32)(MaxSampleF * 1000.0f));
      DebugCount = 0;
   }
#endif

   /* TODO(kleindan) Check for error from CMSIS */
   return 0;
}

