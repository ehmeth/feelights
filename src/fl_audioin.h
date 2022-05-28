#ifndef FL_AUDIOIN_H__
#define FL_AUDIOIN_H__

#include "fl_common.h"
#include "fl_events.h"

u32 AudioInInit();

u32 AudioInRequestRead(u16* Buffer, u32 BufferSize, u32 NumSamples, fl_event EvSamplesAvailable);

#endif // FL_AUDIOIN_H__
