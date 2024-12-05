#include <avr/io.h>
#include "asm-macros.h"
#include "vbi_inject.h"

.global INT1_vect
INT1_vect:

	; START CYCLES : 0
	; 0 cyc

	push r0

	; +2 cyc

	in	r0, _SFR_IO_ADDR(SREG)

	; 3 cycA
	; +1 cyc

	push r0
	push r22
	push r23
	push r24
	push r25
	push r30
	push r31

	; +14 cyc

	; 17 cycA

	; prepare for single byte index
	clr r31

	; +1 cyc

	; 18 cycA

	; R30 = FIELD_PIN (will be 0 or 1)
	in r30, _SFR_IO_ADDR(PIND)

	; +1 cyc

	; 19 cycA


#if defined(V2) || defined(V3)
#if defined(V2)
	; move bit 4 (field pin) so it is bit 0 position
	swap r30

	; +1 cyc

	; 20 cycA

	; get in sync with V3's cycle counter
	nop
	; +1 cyc
	; 21 cycA
#endif
#if defined(V3)

	; move bit 2 down to bit 0
	lsr r30
	lsr r30

	; +2 cyc
	; 21 cycA

#endif

; else if we don't know what version we are, so force compile to fail
#else
	crash
#endif

	; XOR the result since the LM1881 uses 0 for even/bottom and 1 for top/odd
	; And we think of 0 as being top and 1 as being bottom
	com r30

	; +1 cyc

	; 22 cycA

	; isolate bit 0 so it can be used as an index
	andi r30, 1

	; +1 cyc

	; 23 cycA

	; R24 = g_u8Stage
	lds r22, g_u8Stage	; figure out which line we are waiting for

	; +2 cyc

	; 25 cycA

	cpi r22, STAGE_WAITING_FOR_LINE16	; line 16?

	; 26 cycA

	; +1 cyc

	breq line16

	; 27 cycA

	; +1 cyc

	cpi r22, STAGE_WAITING_FOR_LINE17	; line 17?

	; 28 cycA

	; +1 cyc

	breq line17

	; 29 cycA

	; +1 cyc

	cpi r22, STAGE_WAITING_FOR_LINE18	; line 18?

	; 30 cycA

	; +1 cyc

	breq line18

	; 31 cycA

	; +1 cyc

	; else it's line 11
line11:

	; START CYCLES 31

	lds r24, TCNT1L
	lds r25, TCNT1H
	; +4 cyc

	subi r24, lo8(CYCLES_TIL_LINE11)
	sbci r25, hi8(CYCLES_TIL_LINE11)
	; +2 cyc

	brcs line11_done	; branch if TCNT1-CYCLES_TIL_LINE11 is negative
	; +1 cyc if no branch taken

	ldi r24, STAGE_WAITING_FOR_LINE16
	; +1 cyc

	sts g_u8Stage, r24

	; +2 cyc

	; Z (r30,r31) = &g_u8WhiteFlags + r30
	; (need to use subtraction because AVR does not have an immediate add function)
	subi r30, lo8(-(g_u8WhiteFlags))
	sbci r31, hi8(-(g_u8WhiteFlags))

	; +2 cyc

	; put 8-bit value in R22
	ldd r22, Z+0

	; +2 cyc

	; if bit 0 is clear, skip instruction that does whiteflag
	sbrc r22, 0

	; +1 cyc if not skipped

	rjmp do_whiteflag_prep

	; will be 48 cyc after jump
	; +2 cyc after jump

line11_done:
	rjmp done

	; TOTAL CYCLES ADDED (since line11): 17

line16:

	; START CYCLES: 28

	; is it time to do line 16?
	lds r24, TCNT1L
	lds r25, TCNT1H
	; +4 cyc

	subi r24, lo8(CYCLES_TIL_LINE16)
	sbci r25, hi8(CYCLES_TIL_LINE16)
	; +2 cyc

	; nope, not time yet, this is the wrong CSYNC
	brcs done	; branch if TCNT1-CYCLES_TIL_LINE16 is negative
	; +1 cyc

	; ok it's time to do line 16
	ldi r24, STAGE_WAITING_FOR_LINE17
	; +1 cyc

	sts g_u8Stage, r24
	; +2 cyc

	NOPx6	; get in sync with where line18 branch will be
	; +6 cyc

	rjmp do_picnum_prep
	; +2 cyc

	; TOTAL CYCLES ADDED (since line16): 18

line17:

	; START CYCLES: 30

	lds r24, TCNT1L
	lds r25, TCNT1H
	; +4 cyc

	subi r24, lo8(CYCLES_TIL_LINE17)
	sbci r25, hi8(CYCLES_TIL_LINE17)
	; +2 cyc

	brcs done
	; +1 cyc

	ldi r24, STAGE_WAITING_FOR_LINE18
	; +1 cyc

	sts g_u8Stage, r24
	; +2 cyc

	nop	; get in sync with where line18 branch will be
	nop
	nop
	nop
	; +4 cyc

	rjmp do_picnum_prep
	; +2 cyc after jump

	; TOTAL CYCLES ADDED (since line 17): 16

line18:

	; START CYCLES: 32

	lds r24, TCNT1L
	lds r25, TCNT1H
	; +4 cyc

	subi r24, lo8(CYCLES_TIL_LINE18)
	sbci r25, hi8(CYCLES_TIL_LINE18)
	; +2 cyc

	brcs done
	; +1 cyc

	ldi r24, STAGE_WAITING_FOR_VSYNC
	; +1 cyc

	sts g_u8Stage, r24
	; +2 cyc

	; don't come into this ISR anymore until the next VSYNC
;      			DISABLE_CSYNC_INT();
	CBI       _SFR_IO_ADDR(EIMSK),1
	; +2 cyc

	; re-enable vsync interrupt
;      			ENABLE_VSYNC_INT();
	SBI       _SFR_IO_ADDR(EIMSK),0
	; +2 cyc

	; TOTAL CYCLES ADDED (since line 18): 14

do_picnum_prep:

	; START CYCLES: 46

	lsl r22	; r22 = g_u8Stage * 2, prepare to add field index
	; (cycle count resets here to make it easier to make changes)
	; +1 cyc

	add r30, r22	; r30 = field_idx + (g_u8Stage * 2)
	; +1 cyc

	; r30 *= 4 (since the array that has the data in it is a uint32_t array)
	lsl r30
	lsl r30
	; +2 cyc

	; Z (r30,r31) = &g_u32PicNums + r30
	; (need to use subtraction because AVR does not have an immediate add function)
	subi r30, lo8(-(g_u32PicNums))
	sbci r31, hi8(-(g_u32PicNums))
	; +2 cyc

	// put 24-bit value in R22-R24
	ldd r22, Z+0
	ldd r23, Z+1
	ldd r24, Z+2
	; +6 cyc

	; if high bit is clear, it means this is not a valid picture number, so don't display anything
	sbrc r24, 7
	; +1 cyc if not skipped

	rjmp do_picnum_prep2
	; +2 cyc

	; TOTAL CYCLES ADDED (since do_picnum_prep): 15

done:
	; if we are now waiting for vsync, it's okay to re-enable serial interrupt
	lds r24, g_u8Stage
	cpi r24, STAGE_WAITING_FOR_VSYNC
	brne done2

	; enable serial port RX interrupt now that we are done with the critical section
	; Z = UCSR0B
	ldi r30, 0xC1
	ldi r31, 0

	; set high bit (RXCIE0) of UCSR0B
	ld	r24, Z
	ori r24, 0x80
	st Z, r24

done2:
	pop r31
	pop r30
	pop r25
	pop r24
	pop r23
	pop r22
	pop r0
	out	_SFR_IO_ADDR(SREG), r0
	pop r0
	reti

do_picnum_prep2:

	; START CYCLES: 61

	; stall so picture number shows up in the right place
	NOPx8
	NOPx8
	NOPx8
	NOPx8

	; +32 cyc

	NOPx8
	NOPx8
	NOPx8
	NOPx8

	; +32 cyc

	NOPx8
	NOPx8
	NOPx8
	NOPx8

	; +32 cyc

	call do_picture_number

	; +4 cyc

	; TOTAL CYCLES ADDED (since do_picnum_prep2): 100

	; (we will be at 161 cycles once call completes)

	; we want to be at 161 cycles when we enter do_picture_number
	; Explanation:
	; When the CSYNC interrupt occurs, 8 cycles are immediately wasted just getting into the ISR
	; Inside do_picture_number, 6 cycles happen before the first bit is sent
	; Another 16 cycles elapse until the transition of the first bit occurs (each bit is 32 cycles)
	; According to laserdisc standard, the proper time when the middle of the first bit should be displayed is
	;  .188 H which I am pretty sure means .188 * 63.555 microseconds
	; .188 * 63.555 * 16 MHz = ~191 cycles.
	; 191 - (8+6+16) == 161
	; Thus, 161 is our target.

	rjmp done

do_whiteflag_prep:

	; START CYCLES: 48

	; White flag should start on cycle (.160 * 63.555 * 16), or 163
	; We've lost 8 cycles coming into the ISR
	; Doing a function call costs 4, and it takes 2 to change the line status,
	; So we want to make the function call at cycle count 163-(8+4+2) or 149

	NOPx8
	NOPx8
	NOPx8
	NOPx8

	; +32 cyc

	NOPx8
	NOPx8
	NOPx8
	NOPx8

	; +32 cyc

	NOPx8
	NOPx8
	NOPx8
	NOPx8

	; +32 cyc

	nop
	nop
	nop
	nop
	nop

	; +5 cyc

	; TOTAL CYCLES ADDED (since do_whiteflag_prep): 101

	call do_whiteflag

	; +4 cyc after call

	; TOTAL CYCLES ADDED (since do_whiteflag_prep): 105

	rjmp done
