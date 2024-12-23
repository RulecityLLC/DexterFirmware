#ifndef VSYNC_H
#define VSYNC_H

#include "rev.h"

#define ENABLE_VSYNC_INT() (EIMSK |= (1 << VSYNC_EXT_INT))
#define DISABLE_VSYNC_INT() (EIMSK &= ~(1 << VSYNC_EXT_INT))

typedef void(*vsync_callback)();

/*
Options for acting on vsync event:

- Creating your own assembly language callback (and using this set_vsync_isr_callback method).
	You'd want to do this if timing is critical and you need to start some other timer as soon as vsync ticks (such as the LD-V1000 or PR-8210 modes).
	If you go this route, you'll need to call ENABLE_VSYNC_INT() before the next vsync and your vsync callback will need to call DISABLE_VSYNC_INT_ASM so that hard-debugging is possible.
- Using the GOT_VSYNC() and CLEAR_VSYNC_FLAG() macros.  These poll vsync instead of using an interrupt and are much easier to implement if timing isn't that critical.
*/
void set_vsync_isr_callback(vsync_callback pCallback);

#endif // VSYNC_H
