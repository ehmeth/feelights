#ifndef FL_EVENTS_H__
#define FL_EVENTS_H__

#include <zephyr.h>
#include <kernel.h>

typedef enum {
   FlEventStart = 0,
   AudioInSamplesReady = FlEventStart,
   FlEventMax,
} fl_event;

extern struct k_poll_signal OsSiglnals[FlEventMax];

u32 EventsInit();

#endif
