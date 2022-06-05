#include "fl_common.h"
#include "fl_events.h"
#include "zephyr.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(events);

struct k_poll_signal OsSiglnals[EV_MAX_IDX];

struct k_poll_event OsEvents[EV_MAX_IDX];

internal struct k_timer PeriodicTimer;

internal void PeriodicTimerHandler(struct k_timer * Timer)
{
   k_poll_signal_raise(&OsSiglnals[EV_PERIODIC_FRAME], 0);
}


u32 EventsInit()
{
   for (fl_event Event = EV_START_IDX; Event < EV_MAX_IDX; Event++)
   {
      k_poll_event_init(&OsEvents[Event],
                        K_POLL_TYPE_SIGNAL,
                        K_POLL_MODE_NOTIFY_ONLY,
                        &OsSiglnals[Event]);

      /* TODO(kleindan) errors?! */
      k_poll_signal_init(&OsSiglnals[Event]);
   }

   /* TODO(kleindan) errors?! */
   k_timer_init(&PeriodicTimer, PeriodicTimerHandler, 0);

   return 0;
}

u32 EventsStartPeriodicEvent(u32 MsTimeout)
{
   k_timer_start(&PeriodicTimer, K_MSEC(MsTimeout), K_MSEC(MsTimeout));
   return 0;
}

u32 EventsStopPeriodicEvent()
{
   k_timer_stop(&PeriodicTimer);
   return 0;
}

fl_event WaitForEvent(u32 TimeoutMs)
{
   int WaitResult = 0;
   k_timeout_t Timeout = (TimeoutMs == 0) ? K_FOREVER : K_MSEC(TimeoutMs);
   fl_event TriggeredEvent = EV_MAX_IDX;

   WaitResult = k_poll(OsEvents, ArrayCount(OsEvents), Timeout);
   switch (WaitResult) 
   {
      case 0:
         for (fl_event Event = EV_START_IDX; Event < EV_MAX_IDX; Event++)
         {
            if (OsEvents[Event].state & K_POLL_STATE_SIGNALED)
            {
               OsEvents[Event].signal->signaled = 0;
               OsEvents[Event].state = K_POLL_STATE_NOT_READY;
               TriggeredEvent = Event;
               break;
            }
         }
         break;
      case -EAGAIN:
         LOG_ERR("Unexpected polling timeout");
         break;
      default:
         LOG_ERR("Unexpected event");
         break;
   }

   return TriggeredEvent;
}

u32 EventEmit(fl_event Event)
{
   k_poll_signal_raise(&OsSiglnals[Event], 0);

   return 0;
}

