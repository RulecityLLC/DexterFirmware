#ifndef AUTODETECT_DEPS_H
#define AUTODETECT_DEPS_H

#include "settings.h"	// LDPType

// returns non-zero if pin 11 is determined to be raised after performing some debouncing
uint8_t IsPin11Raised();

// detects which type mode to use when the user has selected 'other'
LDPType detect_other_mode();

#endif // AUTODETECT_DEPS_H
