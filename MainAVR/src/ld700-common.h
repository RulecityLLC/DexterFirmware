#include "common.h"

// need some extra (/100's) in here to not overflow 32-bit limit of assembler
// dividing by 8 because we are using a /8 prescaler

#define MICROSECONDS_8MS_MIN 7800
#define CYCLES_8MS_MIN ((((MY_F_CPU / 1000) * MICROSECONDS_8MS_MIN) / (MILLION / 1000)) / 8)

#define MICROSECONDS_TIL_8MS_TIMEOUT 8600
#define CYCLES_TIL_8MS_TIMEOUT ((((MY_F_CPU / 1000) * MICROSECONDS_TIL_8MS_TIMEOUT) / (MILLION / 1000)) / 8)

#define MICROSECONDS_4MS_MIN 3900
#define CYCLES_4MS_MIN ((((MY_F_CPU / 1000) * MICROSECONDS_4MS_MIN) / (MILLION / 1000)) / 8)

#define MICROSECONDS_TIL_4MS_TIMEOUT 4300
#define CYCLES_TIL_4MS_TIMEOUT ((((MY_F_CPU / 1000) * MICROSECONDS_TIL_4MS_TIMEOUT) / (MILLION / 1000)) / 8)

/* a 0 bit is indicated by 1ms being between the start of two adjacent pulses */
/* a 1 bit is indicated by 2ms being between the start of two adjacent pulses */

/* if second pulse comes after this amount of microseconds, we interpret it as a 1.  Otherwise we interpret it as a 0. */
#define MICROSECONDS_TIL_ITS_A_1 1500

#define CYCLES_TIL_ITS_A_1 ((((MY_F_CPU / 1000) * MICROSECONDS_TIL_ITS_A_1) / (MILLION / 1000)) / 8)

/* if we get a pulse and then don't get an adjacent pulse for a while, when do we give up and start over? */
#define MICROSECONDS_TIL_TIMEOUT 2150

#define CYCLES_TIL_TIMEOUT ((((MY_F_CPU / 1000) * MICROSECONDS_TIL_TIMEOUT) / (MILLION / 1000)) / 8)

//////////////////////

// used by ISR to remember which stage its at in processing the incoming control line
#define STAGE_WAITING_FOR_8MS 0
#define	STAGE_8MS_STARTED 1
#define	STAGE_4MS_STARTED 2
#define	STAGE_PULSES_STARTED 3
