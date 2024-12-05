#include "common.h"

// this file is used by both C and assembly language so it needs to have strict syntax (no floating point)

#define MICROSECONDS_TIL_STATUS 500	/* 500-650uS from the time vsync starts until the time status strobe starts */
// need some extra (/100's) in here to not overflow 32-bit limit of assembler
#define CYCLES_UNTIL_STATUS_STARTS ((MY_F_CPU * (MICROSECONDS_TIL_STATUS / 100)) / (MILLION / 100))

#define MICROSECONDS_OF_STATUS 26
#define CYCLES_UNTIL_STATUS_ENDS ((MY_F_CPU * MICROSECONDS_OF_STATUS) / MILLION)

// command strobe starts 54uS after status strobe starts
#define MICROSECONDS_BETWEEN_STROBES (54-26)
#define CYCLES_UNTIL_CMD_STARTS ((MY_F_CPU * MICROSECONDS_BETWEEN_STROBES) / MILLION)

#define MICROSECONDS_OF_CMD 25
#define CYCLES_UNTIL_CMD_ENDS ((MY_F_CPU * MICROSECONDS_OF_CMD) / MILLION)

// the different stages that our ld-v1000 emulation may be in, this is for our timer interrupt to know what to do next.
// This is here so the assembly file can also use it.
#define STAGE_WAITING_FOR_VSYNC 0
#define	STAGE_VSYNC_STARTED 1
// NOTE: no need to explicitly track when vsync stopped, we can say it ends when strobe starts and be fine
#define	STAGE_STATUS_STARTED 2
#define	STAGE_STATUS_STOPPED 3
#define STAGE_CMD_STARTED 4
#define STAGE_CMD_STOPPED 5
