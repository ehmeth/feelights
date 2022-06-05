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
internal pixel *Pixels;

#ifdef CONFIG_TIMING_FUNCTIONS
internal uint64_t TotalCycles = 0, TotalNs = 0;
internal uint64_t SamplesCycles = 0, SamplesNs = 0;
internal uint64_t FftCycles = 0, FftNs = 0;
internal uint64_t UpdateCycles = 0, UpdateNs = 0;
internal uint64_t PixelPushCycles = 0, PixelPushNs = 0;
internal timing_t TStart, TSamplesReady, TFftDone, TUpdateDone, TPixelPushed;
internal u32 TimingCount = 0;
#endif

typedef enum {
   MODE_NORMAL,
   MODE_INSPECTION,
   MODE_BROWNOUT,
   MODE_MAX
} fl_system_mode;


internal void           ModeNormalOnEnter();
internal fl_system_mode ModeNormalOnEvent(fl_event Event);
internal void           ModeNormalOnLeave();

internal void           ModeInspectionOnEnter();
internal fl_system_mode ModeInspectionOnEvent(fl_event Event);
internal void           ModeInspectionOnLeave();

internal void           ModeBrownOutOnEnter();
internal fl_system_mode ModeBrownOutOnEvent(fl_event Event);
internal void           ModeBrownOutOnLeave();

typedef fl_system_mode (*mode_event_handler_t)(fl_event);
typedef void (*mode_enter_handler_t)();
typedef void (*mode_leave_handler_t)();

typedef struct {
   mode_enter_handler_t OnEnter;
   mode_event_handler_t OnEvent;
   mode_leave_handler_t OnLeave;
} mode_functions;

internal u32 ModeTimeouts[MODE_MAX] = {
   60,
   0,
   2000
};

internal mode_functions ModeHandlers[MODE_MAX] = {
   {
      .OnEnter = ModeNormalOnEnter,
      .OnEvent = ModeNormalOnEvent,
      .OnLeave = ModeNormalOnLeave,
   },
   {
      .OnEnter = ModeInspectionOnEnter,
      .OnEvent = ModeInspectionOnEvent,
      .OnLeave = ModeInspectionOnLeave,
   },
   {
      .OnEnter = ModeBrownOutOnEnter,
      .OnEvent = ModeBrownOutOnEvent,
      .OnLeave = ModeBrownOutOnLeave,
   },
};

internal void ModeInspectionOnEnter()
{
   static const pixel RED = {
      .Color = {
         .r = 0xff,
      },
   };

   for (int I = 0; I < NUM_OF_PIXELS; ++I)
   {
      Pixels[I].Dword = RED.Dword;
   }

   StripOutput(Pixels, NUM_OF_PIXELS);
   Pixels = StripSwapBuffer(Pixels);
}

internal fl_system_mode ModeInspectionOnEvent(fl_event Event)
{
   fl_system_mode NextMode = MODE_INSPECTION;

   static const pixel GREEN_BLUE[2] = {
      {
         .Color = {
            .g = 0xff,
         },
      },
      {
         .Color = {
            .b = 0xff,
         },
      },
   };
   static u32 CurrentColor = 0;

   switch (Event)
   {
      case EV_PERIODIC_FRAME:
         break;
      case EV_AUDIO_SAMPLES_AVAILABLE:
         break;
      case EV_BUTTON_PRESSED:
         break;
      case EV_BUTTON_RELEASED:
         if (CurrentColor >= 2)
         {
            NextMode = MODE_BROWNOUT;
            CurrentColor = 0;
         }
         else
         {
            for (int I = 0; I < NUM_OF_PIXELS; ++I)
            {
               Pixels[I].Dword = GREEN_BLUE[CurrentColor].Dword;
            }
            CurrentColor++;

            StripOutput(Pixels, NUM_OF_PIXELS);
            Pixels = StripSwapBuffer(Pixels);
         }
         break;
      default:
         break;
   }

   return NextMode;
}

internal void ModeInspectionOnLeave()
{
   /* Nothing to do really? */
}

internal void ModeBrownOutOnEnter()
{
   static const pixel ALL = {
      .Color = {
         .r = 0xff,
         .g = 0xff,
         .b = 0xff,
      },
   };

   for (int I = 0; I < NUM_OF_PIXELS; ++I)
   {
      Pixels[I].Dword = ALL.Dword;
   }
   Pixels = StripSwapBuffer(Pixels);
   for (int I = 0; I < NUM_OF_PIXELS; ++I)
   {
      if ((I % 2) == 0)
      {
         Pixels[I].Dword = ALL.Dword;
      }
      else
      {
         Pixels[I].Dword = 0;
      }
   }

   Pixels = StripSwapBuffer(Pixels);
   StripOutput(Pixels, NUM_OF_PIXELS);
   Pixels = StripSwapBuffer(Pixels);

   EventsStartPeriodicEvent(1000);
}

internal fl_system_mode ModeBrownOutOnEvent(fl_event Event)
{
   fl_system_mode NextMode = MODE_BROWNOUT;

   switch (Event)
   {
      case EV_PERIODIC_FRAME:
         StripOutput(Pixels, NUM_OF_PIXELS);
         Pixels = StripSwapBuffer(Pixels);
         break;
      case EV_AUDIO_SAMPLES_AVAILABLE:
         break;
      case EV_BUTTON_PRESSED:
         break;
      case EV_BUTTON_RELEASED:
         NextMode = MODE_NORMAL;
         break;
      default:
         break;
   }

   return NextMode;
}

internal void ModeBrownOutOnLeave()
{
   EventsStopPeriodicEvent();
}

internal void ModeNormalOnEnter()
{
#ifdef CONFIG_TIMING_FUNCTIONS
   timing_init();
   timing_start();
   TStart = timing_counter_get();
#endif
   for (int I = 0; I < NUM_OF_PIXELS; ++I)
   {
      Pixels[I].Dword = 0;
   }
   StripOutput(Pixels, NUM_OF_PIXELS);
   Pixels = StripSwapBuffer(Pixels);
   AudioInStart();

}
internal void ModeNormalOnLeave()
{
   AudioInStop();
#ifdef CONFIG_TIMING_FUNCTIONS
   timing_stop();
#endif
}

internal inline fl_system_mode ModeNormalOnEvent(fl_event Event)
{
   static u32 AudioSampleIndex = 0;
   fl_system_mode NextMode = MODE_NORMAL;

   switch (Event)
   {
      case EV_PERIODIC_FRAME:
         break;
      case EV_AUDIO_SAMPLES_AVAILABLE:
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
         break;
      case EV_BUTTON_PRESSED:
         break;
      case EV_BUTTON_RELEASED:
         NextMode = MODE_INSPECTION;
         break;
      default:
         break;
   }

   return NextMode;
}

void main(void)
{
   Pixels = StripGetBuffer();

   EventsInit();
   StripInit();
   LightsInit();
   ButtonInit();
   AudioInInit(SampleBuffer, sizeof(SampleBuffer));

   StripOutput(Pixels, NUM_OF_PIXELS);
   Pixels = StripSwapBuffer(Pixels);

   fl_system_mode CurrentMode = MODE_NORMAL;
   ModeHandlers[CurrentMode].OnEnter();

	while (1) {
      fl_event Event = WaitForEvent(ModeTimeouts[CurrentMode]);

      fl_system_mode NextMode = ModeHandlers[CurrentMode].OnEvent(Event);

      if (NextMode != CurrentMode)
      {
         ModeHandlers[CurrentMode].OnLeave();
         CurrentMode = NextMode;
         ModeHandlers[CurrentMode].OnEnter();
      }
	}
}
