#ifndef TIMER_GLOBAL_H
#define TIMER_GLOBAL_H

#include <avr/io.h>

// according to my calculations, the slow timer will run at 70.3 ticks/second (18432000/1024/256)

void timer_global_init();

void timer_global_think();

uint16_t timer_global_get_slow_val();

// NOTE : TCNT0 is 8-bits, and /1024 of clock so it's 18 kHz.  ( 18.432MHz / 1024 )
#define GET_GLOBAL_TIMER_VALUE() ((uint8_t) TCNT0)

#define GLOBAL_TIMER_DIFF_SINCE_START(startval) ((uint8_t) (TCNT0 - startval))

// An even SLOWER timer, based on somewhat inaccurate overflow counting (useful for long delays where the exact duration isn't that important)
#define GET_GLOBAL_SLOW_TIMER_VALUE() (timer_global_get_slow_val())

// Recommended way to tell when the slow timer has reached a certain point.
// (add 1 because one cannot predict when the slow timer's first transition will be, so add a 1 to guarantee some safe padding)
#define GLOBAL_SLOW_TIMER_DIFF_SINCE_START(startval) ((timer_global_get_slow_val() + 1) - startval)

#endif // TIMER_GLOBAL_H
