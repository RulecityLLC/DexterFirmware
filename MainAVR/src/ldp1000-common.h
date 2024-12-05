#include "common.h"

/*
	LDP1000_LATENCY_CLEAR = 0,
	LDP1000_LATENCY_NUMBER,
	LDP1000_LATENCY_ENTER,
	LDP1000_LATENCY_PLAY,
	LDP1000_LATENCY_STILL,
	LDP1000_LATENCY_INQUIRY,
	LDP1000_LATENCY_GENERIC
*/

// need some extra (/100's) in these defines to not overflow 32-bit limit of assembler

// NOTE : a value of 3500 microseconds just barely fits in a 16-bit unsigned space, don't go any higher :)

#define MICROSECONDS_FOR_CLEAR 1500
#define CYCLES_FOR_CLEAR ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))

#define MICROSECONDS_FOR_NUMBER 1500
#define CYCLES_FOR_NUMBER ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))

#define MICROSECONDS_FOR_ENTER 1500
#define CYCLES_FOR_ENTER ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))

#define MICROSECONDS_FOR_PLAY 1500
#define CYCLES_FOR_PLAY ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))

#define MICROSECONDS_FOR_STILL 1500
#define CYCLES_FOR_STILL ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))

#define MICROSECONDS_FOR_INQUIRY 0
#define CYCLES_FOR_INQUIRY ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))

#define MICROSECONDS_FOR_GENERIC 1500
#define CYCLES_FOR_GENERIC ((MY_F_CPU * (MICROSECONDS_FOR_CLEAR / 100)) / (MILLION / 100))
