#include "fl_common.h"
#include "fl_strip.h"
#include "zephyr.h"
#include "device.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(strip);

/* LED STRIP */
#define STRIP_NODE		DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_STACKSIZE 1024
#define STRIP_PRIORITY 7
#define STRIP_START_DELAY_MS 5

internal const struct device *StripDevice = DEVICE_DT_GET(STRIP_NODE);

internal pixel PixelArena[STRIP_NUM_PIXELS * 2];

internal struct
{
   pixel *PixelsStart;
   u32 NumOfPixels;
   struct k_poll_signal PushSignal;
   struct k_poll_event PushEvent;
   
} PushJob;

internal void PushThread(void)
{
   int WaitResult;

   while (1)
   {
      WaitResult = k_poll(&PushJob.PushEvent, 1, K_FOREVER);
      switch (WaitResult) 
      {
         case 0:
            if (PushJob.PushEvent.state & K_POLL_STATE_SIGNALED)
            {
               PushJob.PushEvent.signal->signaled = 0;
               PushJob.PushEvent.state = K_POLL_STATE_NOT_READY;

               if (PushJob.NumOfPixels > STRIP_NUM_PIXELS)
               {
                  PushJob.NumOfPixels = STRIP_NUM_PIXELS;
               }
               int rc = led_strip_update_rgb(StripDevice, &PushJob.PixelsStart->Color, PushJob.NumOfPixels);

               if (rc) {
                  LOG_ERR("couldn't update strip: %d", rc);
               }
            }
            break;
         default:
            LOG_ERR("Unexpected wait value: %d", WaitResult);
            break;
      }
   }
}

K_THREAD_DEFINE(PushThreadId, STRIP_STACKSIZE, PushThread, NULL, NULL, NULL, STRIP_PRIORITY, 0, STRIP_START_DELAY_MS);

u32 StripInit()
{
   k_poll_event_init(&PushJob.PushEvent,
         K_POLL_TYPE_SIGNAL,
         K_POLL_MODE_NOTIFY_ONLY,
         &PushJob.PushSignal);

   /* TODO(kleindan) errors?! */
   k_poll_signal_init(&PushJob.PushSignal);

	if (device_is_ready(StripDevice)) {
		LOG_INF("Found LED strip device %s", StripDevice->name);
	} else {
		LOG_ERR("LED strip device %s is not ready", StripDevice->name);
      /* TODO(kleindan) define errors */
		return 234;
	}

   return 0;
}



u32 StripOutput(pixel *Pixels, u32 NumOfPixels)
{
   PushJob.PixelsStart = Pixels;
   PushJob.NumOfPixels = NumOfPixels;
   k_poll_signal_raise(&PushJob.PushSignal, 0);

   return 0;
}

pixel* StripGetBuffer()
{
   return PixelArena;
}

pixel* StripSwapBuffer(pixel *PixelBuffer)
{
   return (PixelBuffer == PixelArena) ? PixelArena + STRIP_NUM_PIXELS : PixelArena;
}

