#ifndef TIMER_GLOBAL_H
#define TIMER_GLOBAL_H

#include <avr/io.h>

// according to my calculations, the slow timer will run at 70/second (18432000/1024/256)

extern uint16_t g_u16TimerGlobalOverflowCount;

void timer_global_init();

void timer_global_think();

// NOTE : TCNT0 is 8-bits
#define GET_GLOBAL_TIMER_VALUE() ((uint8_t) TCNT0)

#define GLOBAL_TIMER_DIFF_SINCE_START(startval) ((uint8_t) (TCNT0 - startval))

// An even SLOWER timer, based on somewhat inaccurate overflow counting (useful for long delays where the exact duration isn't that important)
#define GET_GLOBAL_SLOW_TIMER_VALUE() (g_u16TimerGlobalOverflowCount)

// Recommended way to tell when the slow timer has reached a certain point.
// (add 1 because one cannot predict when the slow timer's first transition will be, so add a 1 to guarantee some safe padding)
#define GLOBAL_SLOW_TIMER_DIFF_SINCE_START(startval) ((g_u16TimerGlobalOverflowCount + 1) - startval)

#endif // TIMER_GLOBAL_H
