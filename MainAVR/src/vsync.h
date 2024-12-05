#ifndef VSYNC_H
#define VSYNC_H

#include "rev.h"

#define ENABLE_VSYNC_INT() (EIMSK |= (1 << VSYNC_EXT_INT))
#define DISABLE_VSYNC_INT() (EIMSK &= ~(1 << VSYNC_EXT_INT))

typedef void(*vsync_callback)();

void set_vsync_isr_callback(vsync_callback pCallback);

#endif // VSYNC_H
