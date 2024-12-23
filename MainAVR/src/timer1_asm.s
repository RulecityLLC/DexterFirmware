#include <avr/io.h>

.extern g_pTimer1Callback
.extern g_pPCINT0Callback

; TIMER1_COMPA_vect

.global TIMER1_COMPA_vect
TIMER1_COMPA_vect:
	; TODO : see if we can eliminate push r2 and pop r2 by reserving that register
	push r2
	in	r2, _SFR_IO_ADDR(SREG)
	push r30
	push r31
	lds	r30, g_pTimer1Callback
	lds r31, (g_pTimer1Callback)+1
	icall	; indirect call to (Z)
	pop r31
	pop r30
	out	_SFR_IO_ADDR(SREG), r2
	pop r2
	reti

.global PCINT0_vect
PCINT0_vect:
	push r2
	in	r2, _SFR_IO_ADDR(SREG)
	push r30
	push r31
	lds	r30, g_pPCINT0Callback
	lds r31, (g_pPCINT0Callback)+1
	icall	; indirect call to (Z)
	pop r31
	pop r30
	out	_SFR_IO_ADDR(SREG), r2
	pop r2
	reti
