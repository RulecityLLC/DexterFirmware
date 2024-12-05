#ifndef LDP1000_MAIN
#define LDP1000_MAIN

#include "settings.h"

void ldp1000_main_loop(LDPType type);	// ldptype passed in to avoid knowledge of whether we auto-detected the player or not
void ldp1000_vsync_callback();
void alg_multirom_check();

#endif // LDP1000_MAIN
