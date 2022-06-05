#ifndef FL_AUDIOIN_H__
#define FL_AUDIOIN_H__

#include "fl_common.h"
#include "fl_events.h"

u32 AudioInInit(u16* Buffer, u32 BufferSize);
u32 AudioInStart();
u32 AudioInStop();

#endif // FL_AUDIOIN_H__
