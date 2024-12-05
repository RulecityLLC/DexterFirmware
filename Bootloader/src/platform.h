#ifndef PLATFORM_H
#define PLATFORM_H

// change this to 328p, 644p, or whatever AVR you're using

#if defined(PLATFORM_VBI_INJECT)
#include "platform_aux.h"
#else
#include "platform_main.h"
#endif

#endif // PLATFORM_H
