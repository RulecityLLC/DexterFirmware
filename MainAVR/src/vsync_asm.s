#include <avr/io.h>

.extern g_pVsyncCallback

#if defined(REV2) || defined(REV3)
.global INT2_vect
INT2_vect:
#endif
#if defined(REV1)
.global INT0_vect
INT0_vect:
#endif
	; TODO : see if we can eliminate push r2 and pop r2 by reserving that register
	push r2
	in	r2, _SFR_IO_ADDR(SREG)
	push r30
	push r31
	lds	r30, g_pVsyncCallback
	lds r31, (g_pVsyncCallback)+1
	icall	; indirect call to (Z)
	pop r31
	pop r30
	out	_SFR_IO_ADDR(SREG), r2
	pop r2
	reti
