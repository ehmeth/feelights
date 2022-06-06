#ifndef FL_EVENTS_H__
#define FL_EVENTS_H__

#include <zephyr.h>
#include <kernel.h>

typedef enum {
   EV_START_IDX = 0,
   EV_AUDIO_SAMPLES_AVAILABLE = EV_START_IDX,
   EV_BUTTON_PRESSED,
   EV_BUTTON_RELEASED,
   EV_PERIODIC_FRAME,
   EV_MAX_IDX,
} fl_event;

extern struct k_poll_signal OsSiglnals[EV_MAX_IDX];

u32 EventsInit();

fl_event WaitForEvent(u32 TimeoutMs);

u32 EventsStopPeriodicEvent();

u32 EventsStartPeriodicEvent(u32 MsTimeout);

u32 EventEmit(fl_event Event);

#endif
