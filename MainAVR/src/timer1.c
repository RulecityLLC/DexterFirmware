#include <avr/interrupt.h>
#include "timer1.h"

timer1_callback g_pTimer1Callback = 0;

void set_timer1_isr_callback(timer1_callback pCallback)
{
	g_pTimer1Callback = pCallback;
}
