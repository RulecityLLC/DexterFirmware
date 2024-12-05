#include <avr/io.h>
#include "asm-macros.h"

.extern g_u32Line

.global do_picture_number
do_picture_number:

	; Input pic num will be in R22-R24
	; don't need to save flags, ISR does that

	CLR       R25

do_picnum_loop:

	; 0 pre cyc

	SBRS R24, 7	; skip if we need to render a 1 bit
	RJMP NotSetPath
	NOP	; so we are in sync with the other path

	; 3 pre cyc

	SBI _SFR_IO_ADDR(PORTB), 4	; disable white

	; 5 pre cyc

	; (resetting cycle counting notes under the assumption that the white/black state has changed,
	;	this will be point from which we reckon cycle time to make things easy)
	; 0 cyc

	NOPx8

	; 8 cyc

	NOPx6

	; 14 cyc

	CBI _SFR_IO_ADDR(PORTB), 4	; enable white
	
	; 16 cyc (1 uS)

	RJMP shifter

NotSetPath:

	; 3 pre cyc

	CBI _SFR_IO_ADDR(PORTB), 4 ; enable white

	; (resetting cycle counter)
	; 0 cyc

	NOPx8

	; 8 cyc

	NOPx6

	; 14 cyc

	SBI _SFR_IO_ADDR(PORTB), 4 ; disable white

	; 16 cyc (1 uS)

	; to be in cycle sync with the other path
	NOP
	NOP

shifter:

	; 18 cyc

	LSL R22
	ROL R23
	ROL R24

	; 21 cyc

	SUBI R25, 0xFF	; add 1 to counter

	; 22 cyc

	CPI R25, 24	; have we done 24 bits?

	; 23 cyc

	BREQ	done

	; 24 cyc

	NOP

	; 25 cyc

	RJMP do_picnum_loop

	; 27 cyc (taking into account the 5 'pre' cycles at the beginning of the loop)

done:

	; 25 cyc

	NOP
	NOP
	NOP
	NOP
	NOP

	; 30 cyc

	; make sure white is disabled
	SBI _SFR_IO_ADDR(PORTB), 4

	; 32 cyc (2 uS)

	RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.global do_whiteflag
do_whiteflag:
	
	CBI _SFR_IO_ADDR(PORTB), 4 ; enable white

	; white flag needs to be displayed for (.790 * 63.555 * 16) cycle, 803

	NOPx32
	NOPx32
	NOPx32
	NOPx32
	; 128cyc

	NOPx32
	NOPx32
	NOPx32
	NOPx32
	; 256 cyc

	NOPx32
	NOPx32
	NOPx32
	NOPx32
	; 384 cyc

	NOPx32
	NOPx32
	NOPx32
	NOPx32
	; 512 cyc

	NOPx32
	NOPx32
	NOPx32
	NOPx32
	; 640 cyc

	NOPx32
	NOPx32
	NOPx32
	NOPx32
	; 768 cyc

	NOPx32
	; 800 cyc

	nop
	nop
	nop

	; 803 cyc

	SBI _SFR_IO_ADDR(PORTB), 4	; disable white

	ret
