#ifndef PR8210_MAIN_H
#define PR8210_MAIN_H

#include "settings.h"

void pr8210_main_loop(LDPType type);	// ldptype passed in to avoid knowledge of whether we auto-detected the player or not
void pr8210_think();
void pr8210_vsync_callback();
void pr8210_timer1_callback();

#endif // PR8210_MAIN_H
