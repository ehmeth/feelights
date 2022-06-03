#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr.h>
#include "fl_audioin.h"
#include "fl_events.h"
#include "fl_strip.h"
#include "fl_dsp.h"
#include "fl_lights.h"
#include "fl_button.h"

#ifdef CONFIG_TIMING_FUNCTIONS
#include <timing/timing.h>
/*
 * Samples: 21,651,281 ns <- This is obviously a problem...
 * Fft:      1,195,757 ns 
 * Update:     229,720 ns
 * Pixel:    2,471,695 ns
 * Total    25,548,456 ns
 *
 * 128 samples at 22us should amount to 2.8ms, not 21.6ms
 */

#endif

#define NUM_SAMPLES (128)
#define SAMPLE_INDEX_MASK  (0xFF)
#define DELAY_TIME K_MSEC(10)
#define NUM_OF_PIXELS (30)

internal int16_t SampleBuffer[2*NUM_SAMPLES];

internal f32 FftInput[NUM_SAMPLES];
internal f32 FftComplex[2*NUM_SAMPLES];
internal f32 FftOut[NUM_SAMPLES];

void main(void)
{
   u32 AudioSampleIndex = 0;
   pixel *Pixels = StripGetBuffer();
   u32 ret = 0;
#ifdef CONFIG_TIMING_FUNCTIONS
   uint64_t TotalCycles = 0, TotalNs = 0;
   uint64_t SamplesCycles = 0, SamplesNs = 0;
   uint64_t FftCycles = 0, FftNs = 0;
   uint64_t UpdateCycles = 0, UpdateNs = 0;
   uint64_t PixelPushCycles = 0, PixelPushNs = 0;
   timing_t TStart, TSamplesReady, TFftDone, TUpdateDone, TPixelPushed;
   u32 TimingCount = 0;
#endif

   AudioInInit();
   EventsInit();
   StripInit();
   LightsInit();
   ButtonInit();

#ifdef CONFIG_TIMING_FUNCTIONS
   timing_init();
   timing_start();
   TStart = timing_counter_get();
#endif

   StripOutput(Pixels, NUM_OF_PIXELS);
   EventsStartPeriodicEvent(16);

	while (1) {
      fl_event Event = WaitForEvent(30);
      if (EV_PERIODIC_FRAME == Event)
      {
#ifdef CONFIG_TIMING_FUNCTIONS
         SamplesCycles     = timing_cycles_get(&TStart, &TSamplesReady);
         FftCycles         = timing_cycles_get(&TSamplesReady, &TFftDone);
         UpdateCycles      = timing_cycles_get(&TFftDone, &TUpdateDone);
         PixelPushCycles   = timing_cycles_get(&TUpdateDone, &TPixelPushed);
         TotalCycles       = timing_cycles_get(&TStart, &TPixelPushed);

         SamplesNs        += timing_cycles_to_ns(SamplesCycles);
         FftNs            += timing_cycles_to_ns(FftCycles);
         UpdateNs         += timing_cycles_to_ns(UpdateCycles);
         PixelPushNs      += timing_cycles_to_ns(PixelPushCycles);
         TotalNs          += timing_cycles_to_ns(TotalCycles);

         TimingCount = (TimingCount + 1) & 0xFF;
         if (0 == TimingCount)
         {
            SamplesNs    >>= 8;
            FftNs        >>= 8;
            UpdateNs     >>= 8;
            PixelPushNs  >>= 8;
            TotalNs      >>= 8;

            LOG_INF("Samples: %lldns, Fft: %lldns, Update: %lldns, Pixel: %lldns, Total %lldns", SamplesNs, FftNs, UpdateNs, PixelPushNs, TotalNs);

            SamplesNs    = 0;
            FftNs        = 0;
            UpdateNs     = 0;
            PixelPushNs  = 0;
            TotalNs      = 8;
         }

         timing_start();
         TStart = timing_counter_get();
#endif

         u32 NextAudioSampleIndex = (AudioSampleIndex + NUM_SAMPLES) & SAMPLE_INDEX_MASK;

         ret = AudioInRequestRead(&SampleBuffer[NextAudioSampleIndex], NUM_SAMPLES * sizeof(u16), NUM_SAMPLES, EV_AUDIO_SAMPLES_AVAILABLE);
         if (ret)
         {
            LOG_ERR("audio req failed: %d", ret);
         }
         AudioSampleIndex = NextAudioSampleIndex;
      }
      if (EV_AUDIO_SAMPLES_AVAILABLE == Event)
      {
#ifdef CONFIG_TIMING_FUNCTIONS
         TSamplesReady = timing_counter_get();
#endif
         DspNormalizeSamples(&SampleBuffer[AudioSampleIndex], NUM_SAMPLES, FftInput);
         DspCalculateSpectrum(FftInput, NUM_SAMPLES, FftComplex, FftOut);
#ifdef CONFIG_TIMING_FUNCTIONS
         TFftDone = timing_counter_get();
#endif

         LightsUpdateAndRender(Pixels, NUM_OF_PIXELS, FftOut, NUM_SAMPLES);
#ifdef CONFIG_TIMING_FUNCTIONS
         TUpdateDone = timing_counter_get();
#endif

         StripOutput(Pixels, NUM_OF_PIXELS);
         Pixels = StripSwapBuffer(Pixels);
#ifdef CONFIG_TIMING_FUNCTIONS
         TPixelPushed = timing_counter_get();
         timing_stop();
#endif
      }
      if (EV_BUTTON_PRESSED == Event)
      {
         LOG_INF("Button pressed");
      }
      if (EV_BUTTON_RELEASED == Event)
      {
         LOG_INF("Button released");
      }
	}
}
