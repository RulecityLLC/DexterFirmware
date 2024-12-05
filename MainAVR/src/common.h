#ifndef COMMON_H
#define COMMON_H

#include <avr/io.h> 
#include <avr/interrupt.h>

#include "rev.h"

// F_CPU is the frequency (in cycles/second) of the AVR's clock
// F_CPU is defined by using -D

// in case avr studio undefines this behind our backs
#ifndef F_CPU
fix your project
#endif

// workaround AVR studio's stupidity
#if F_CPU == 18432000UL
#define MY_F_CPU 18432000
#endif

// to avoid typos
#define MILLION 1000000

// enable/disable CTC interrupt
#define ENABLE_CTC_INT() (TIMSK1 |= (1 << OCIE1A))
#define DISABLE_CTC_INT() (TIMSK1 &= ~(1 << OCIE1A))
#define IS_CTC_INT_ENABLED() (TIMSK1 & (1 << OCIE1A))

#endif // COMMON_H
