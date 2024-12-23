#ifndef TIMER1_H
#define TIMER1_H

#include "rev.h"

typedef void(*timer1_callback)();
typedef void(*pcint0_callback)();

void set_timer1_isr_callback(timer1_callback pCallback);
void set_pcint0_isr_callback(pcint0_callback pCallback);

#endif // VSYNC_H
