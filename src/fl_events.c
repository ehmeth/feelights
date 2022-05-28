#include "fl_common.h"
#include "fl_events.h"
#include "zephyr.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(events);

struct k_poll_signal OsSiglnals[FlEventMax];

u32 EventsInit()
{
   for (fl_event Event = FlEventStart; Event < FlEventMax; Event++)
   {
      k_poll_signal_init(&OsSiglnals[Event]);
   }

   return 0;
}
