#ifndef AUTODETECT_H
#define AUTODETECT_H

#include "settings.h"	// LDPType

// detects whether the active player is LD-V1000 or PR-7820
LDPType detect_ldv1000_or_pr7820();

// detects which type mode to use when the user has selected 'other'
LDPType detect_other_mode();

#endif // AUTODETECT_H
