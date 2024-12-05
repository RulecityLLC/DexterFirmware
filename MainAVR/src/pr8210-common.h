#include "common.h"

/* MACH 3 sends a bunch of low-high pulses instead of a steady low pulse so once we get the start of the first
pulse, we have to ignore everything else for a period of time so we don't get confused. */
#define MICROSECONDS_TIL_PULSE_END 520	/* pulse is to last 260 uS so pick something safe after that point */
// need some extra (/100's) in here to not overflow 32-bit limit of assembler
#define CYCLES_UNTIL_PULSE_END (((MY_F_CPU / 1000) * MICROSECONDS_TIL_PULSE_END) / (MILLION / 1000))

/* a 0 bit is indicated by 1.05ms being between the start of two adjacent pulses */
/* a 1 bit is indicated by 2.11ms being between the start of two adjacent pulses */

/* if second pulse comes after this amount of microseconds, we interpret it as a 1.  Otherwise we interpret it as a 0. */
#define MICROSECONDS_TIL_ITS_A_1 1580

#define CYCLES_TIL_ITS_A_1 (((MY_F_CPU / 1000) * MICROSECONDS_TIL_ITS_A_1) / (MILLION / 1000))

/* if we get a pulse and then don't get an adjacent pulse for a while, when do we give up and start over? */
#define MICROSECONDS_TIL_TIMEOUT 3000

#define CYCLES_TIL_TIMEOUT (((MY_F_CPU / 1000) * MICROSECONDS_TIL_TIMEOUT) / (MILLION / 1000))
