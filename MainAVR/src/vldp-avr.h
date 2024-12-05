#ifndef VLDP_AVR_H
#define VLDP_AVR_H

extern volatile unsigned char g_bRestartPlayer;
extern volatile unsigned char g_bReprogramForced;

// Global AVR pinouts:

#include "rev.h"

// Since some LDP's may behave differently, we do not set pull-ups here; ldp-specific code should set pull-ups where appropriate
#define DATA_ENABLE_READ() DATA_CONTROL=0

// NOTE: since the AVR has internal pull-ups, the value on the data bus will not change when switching to output mode.
// So you don't need to worry about setting it before changing to output mode
#define DATA_ENABLE_WRITE() DATA_CONTROL=0xFF

// for now we will initialize in read mode
#define DATA_INIT() DATA_ENABLE_READ()

// NOTE : here's a URL with some handy info about NTSC http://inst.eecs.berkeley.edu/~cs150/sp99/sp99/project/compvideo.htm

#endif // VLDP_AVR_H
