#include "common.h"

// (23-4) * 63.555uS	(-4 because vsync occurs on line 4)
#define MICROSECONDS_TIL_LINE23 1208
// should be about 22266, doesn't need to be exact
#define CYCLES_TIL_LINE23 ((MY_F_CPU * (MICROSECONDS_TIL_LINE23 / 100)) / (MILLION / 100))

// how many cycles/1024 occupy 13 milliseconds
#define TICKS_FOR_13MS (((MY_F_CPU / 1000) * 13) / 1024)

// how many status bytes we will send (should be 6, but can be reduced for testing)
#define VP931_STATUS_BYTE_ARRAY_SIZE	6

// how many command bytes we will receive per field
#define VP931_CMD_BYTE_ARRAY_SIZE	6
