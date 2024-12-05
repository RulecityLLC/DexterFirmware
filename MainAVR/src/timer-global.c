#include "timer-global.h"

// this is a 16-bit number because the slow timer runs at about 70 ticks/second (18432000/1024/256)
uint16_t g_u16TimerGlobalOverflowCount = 0;

void timer_global_init()
{
	// sensible defaults
	TCCR0A = 0;

	// start 8-bit timer to do software debounce
	// divide by 1024 of actual clock since this is just an 8-bit counter
	TCCR0B = (1 << CS02) | (1 << CS00);
}

void timer_global_think()
{
	// if the global timer has overflowed (this is intentionally not handled by an interrupt so that we aren't interfering with higher priority interrupts like serial I/O or vsync)
	if (TIFR0 & (1 << TOV0))
	{
		// increment overflow count
		g_u16TimerGlobalOverflowCount++;

		// flag is cleared by writing a 1
		TIFR0 |= (1 << TOV0);
	}
}
