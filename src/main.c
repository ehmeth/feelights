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

#define NUM_SAMPLES (128)
#define SAMPLE_INDEX_MASK  (0xFF)
#define DELAY_TIME K_MSEC(10)
#define NUM_OF_PIXELS (30)

internal int16_t sample_buffer[2*NUM_SAMPLES];

internal f32 FftInput[NUM_SAMPLES];
internal f32 FftComplex[2*NUM_SAMPLES];
internal f32 FftOut[NUM_SAMPLES];

void main(void)
{
   u32 AudioSampleIndex = 0;
   pixel *Pixels = StripGetBuffer();

   AudioInInit();
   EventsInit();
   StripInit();
   LightsInit();

   AudioInRequestRead(sample_buffer, NUM_SAMPLES * sizeof(u16), NUM_SAMPLES, AudioInSamplesReady);
   EventsStartPeriodicEvent(16);

	while (1) {
      fl_event Event = WaitForEvent(30);
      LOG_INF("fl_event: %d", Event);
      if (PeriodicEvent == Event)
      {
         u32 NextAudioSampleIndex = (AudioSampleIndex + NUM_SAMPLES) & SAMPLE_INDEX_MASK;

         AudioInRequestRead(&sample_buffer[NextAudioSampleIndex], NUM_SAMPLES * sizeof(u16), NUM_SAMPLES, AudioInSamplesReady);
         AudioSampleIndex = NextAudioSampleIndex;
      }
      if (AudioInSamplesReady == Event)
      {
         DspNormalizeSamples(&sample_buffer[AudioSampleIndex], NUM_SAMPLES, FftInput);
         DspCalculateSpectrum(FftInput, NUM_SAMPLES, FftComplex, FftOut);

         LightsUpdateAndRender(Pixels, NUM_OF_PIXELS, FftOut, NUM_SAMPLES);

         StripOutput(Pixels, NUM_OF_PIXELS);
         Pixels = StripSwapBuffer(Pixels);
      }
	}
}
