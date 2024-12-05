#include <avr/interrupt.h>
#include "vsync.h"

vsync_callback g_pVsyncCallback = 0;

void set_vsync_isr_callback(vsync_callback pCallback)
{
	g_pVsyncCallback = pCallback;
}
