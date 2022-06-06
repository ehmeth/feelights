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

internal const struct device *StripDevice = DEVICE_DT_GET(STRIP_NODE);

internal pixel PixelArena[STRIP_NUM_PIXELS * 2];

u32 StripInit()
{
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
   if (NumOfPixels > STRIP_NUM_PIXELS)
   {
      NumOfPixels = STRIP_NUM_PIXELS;
   }
   int rc = led_strip_update_rgb(StripDevice, &Pixels->Color, STRIP_NUM_PIXELS);

   if (rc) {
      LOG_ERR("couldn't update strip: %d", rc);
   }

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

