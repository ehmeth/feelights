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
/* TODO(kleindan)
 * Also we need a west.yml file for easier building in the future: https://github.com/kpochwala/kubot-software/blob/main/west.yml
 *
 * Samples:    11648299ns
 * Fft:        11079338ns
 * Update:       410983ns
 * Pixel:       2454577ns
 * Total       25593201ns
 */

#endif

#define NUM_SAMPLES (1024)
#define SAMPLE_INDEX_MASK  ((NUM_SAMPLES * 2) - 1)
#define NUM_OF_PIXELS (30)

internal int16_t SampleBuffer[2*NUM_SAMPLES];

internal f32 FftInput[NUM_SAMPLES];
internal f32 FftComplex[NUM_SAMPLES];
internal f32 FftOut[NUM_SAMPLES/2];

void main(void)
{
   u32 AudioSampleIndex = 0;
   pixel *Pixels = StripGetBuffer();
#ifdef CONFIG_TIMING_FUNCTIONS
   uint64_t TotalCycles = 0, TotalNs = 0;
   uint64_t SamplesCycles = 0, SamplesNs = 0;
   uint64_t FftCycles = 0, FftNs = 0;
   uint64_t UpdateCycles = 0, UpdateNs = 0;
   uint64_t PixelPushCycles = 0, PixelPushNs = 0;
   timing_t TStart, TSamplesReady, TFftDone, TUpdateDone, TPixelPushed;
   u32 TimingCount = 0;
#endif

   EventsInit();
   StripInit();
   LightsInit();
   ButtonInit();
   AudioInInit(SampleBuffer, sizeof(SampleBuffer));

#ifdef CONFIG_TIMING_FUNCTIONS
   timing_init();
   timing_start();
   TStart = timing_counter_get();
#endif

   StripOutput(Pixels, NUM_OF_PIXELS);
   // EventsStartPeriodicEvent(16);

	while (1) {
      fl_event Event = WaitForEvent(30);
      if (EV_PERIODIC_FRAME == Event)
      {


      }
      if (EV_AUDIO_SAMPLES_AVAILABLE == Event)
      {
#ifdef CONFIG_TIMING_FUNCTIONS
         TSamplesReady = timing_counter_get();
#endif
         DspNormalizeSamples(&SampleBuffer[AudioSampleIndex], NUM_SAMPLES, FftInput);
         DspCalculateSpectrum(FftInput, NUM_SAMPLES, FftComplex, FftOut);
         AudioSampleIndex = (AudioSampleIndex + NUM_SAMPLES) & SAMPLE_INDEX_MASK;
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
