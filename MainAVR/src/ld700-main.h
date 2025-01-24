#ifndef LD700_MAIN_H
#define LD700_MAIN_H

#include <stdint.h>

void ld700_main_loop();
void ld700_on_ext_ctrl_changed();	// assembly language
void ld700_on_ext_ctrl_timeout();	// assembly language

#endif // LD700_MAIN_H
