#ifdef V3
#include "v3.h"
#endif

#ifdef V2
#include "v2.h"
#endif

/////////////

#define VSYNC_EXT_INT INT0

#define ENABLE_VSYNC_INT() (EIMSK |= (1 << VSYNC_EXT_INT))
#define DISABLE_VSYNC_INT() (EIMSK &= ~(1 << VSYNC_EXT_INT))

/////////

#define CSYNC_EXT_INT INT1

#define ENABLE_CSYNC_INT() (EIMSK |= (1 << CSYNC_EXT_INT))
#define DISABLE_CSYNC_INT() (EIMSK &= ~(1 << CSYNC_EXT_INT))

// make interrupt occur on falling edge only
#define SETUP_CSYNC_INT() 	EICRA |= (1 << ISC11); EICRA &= ~(1 << ISC10)

////////////////////////////

// workaround AVR studio's stupidity
#if F_CPU == 16000000UL
#define MY_F_CPU 16000000
#endif

// to avoid typos
#define MILLION 1000000

/*
For top/odd field:
	LM1881 vsync occurs on line 4.5 which means it is 6.5 lines away from the beginning of line 11
		See Jan 24, 2013 blog post
	So to find line 11, a CSYNC must occur after 6 lines since VSYNC (6.25 may be okay)
		
For bottom/even field:
	LM1881 vsync occurs at the beginning of line 267 which means it is 7 lines away from the beginning of line 274
		See Jan 27, 2013 blog post
	So to find line 274, a CSYNC must occur after 6.5 lines since VSYNC (6.25 may be okay)
*/

// Each line is about 63.555 microseconds long.
// From the vsync pulse to line 11 (white flag) is about 6.5 horizontal lines.
// From the vsync pulse to line 274 (white flag) is about 7 horizontal lines.
// Therefore, if we get a CSYNC after 6.25 lines, we know it's line 11/274.
#define MICROSECONDS_TIL_LINE11 397	/* 63.555 * 6.25, rounded */

#define MICROSECONDS_TIL_LINE16 715 /* 63.555 * 11.25, rounded  */
#define MICROSECONDS_TIL_LINE17 779 /* 63.555 * 12.25, rounded  */
#define MICROSECONDS_TIL_LINE18 842 /* 63.555 * 13.25, rounded  */

// need some extra (/100's) in here to not overflow 32-bit limit of assembler
#define CYCLES_TIL_LINE11 (((MY_F_CPU / 100) * MICROSECONDS_TIL_LINE11) / (MILLION / 100))	/* 18D0 */
#define CYCLES_TIL_LINE16 (((MY_F_CPU / 100) * MICROSECONDS_TIL_LINE16) / (MILLION / 100))
#define CYCLES_TIL_LINE17 (((MY_F_CPU / 100) * MICROSECONDS_TIL_LINE17) / (MILLION / 100))
#define CYCLES_TIL_LINE18 (((MY_F_CPU / 100) * MICROSECONDS_TIL_LINE18) / (MILLION / 100))	/* 34A0 */

///////////////////////////////////////////////////////////

// 0,1,2 chosen here to make indexing easier
#define	STAGE_WAITING_FOR_LINE16 0
#define	STAGE_WAITING_FOR_LINE17 1
#define STAGE_WAITING_FOR_LINE18 2
#define STAGE_WAITING_FOR_VSYNC 3
#define	STAGE_WAITING_FOR_LINE11 4

//////////////////////////////////////////////////////////////
