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

   struct k_poll_event events[1] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                 K_POLL_MODE_NOTIFY_ONLY,
                                 &OsSiglnals[AudioInSamplesReady]),
   };


	while (1) {
      if (0 == k_poll(events, 1, K_MSEC(30)))
      {
         u32 NextAudioSampleIndex = (AudioSampleIndex + NUM_SAMPLES) & SAMPLE_INDEX_MASK;

         events[0].signal->signaled = 0;
         events[0].state = K_POLL_STATE_NOT_READY;

         StripOutput(Pixels, NUM_OF_PIXELS);
         Pixels = StripSwapBuffer(Pixels);

         AudioInRequestRead(&sample_buffer[NextAudioSampleIndex], NUM_SAMPLES * sizeof(u16), NUM_SAMPLES, AudioInSamplesReady);

         DspNormalizeSamples(&sample_buffer[AudioSampleIndex], NUM_SAMPLES, FftInput);
         DspCalculateSpectrum(FftInput, NUM_SAMPLES, FftComplex, FftOut);

         AudioSampleIndex = NextAudioSampleIndex;

         LightsUpdateAndRender(Pixels, NUM_OF_PIXELS, FftOut, NUM_SAMPLES);
      }
      k_sleep(DELAY_TIME);
	}
}
