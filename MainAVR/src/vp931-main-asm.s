#include <avr/io.h>
#include "common.h"
#include "common-asm.h"
#include "vp931-common.h"

; apparently this 'extern' statement is not needed but I'll include it anyway
.extern g_bVP931VsyncActive
.extern g_arrVP931StatusBytes
.extern g_u8VP931StatusBytesSent
.extern g_arrVP931CmdBytes
.extern g_u8VP931CmdBytesReceived
.extern g_bVP931CmdOverflow

.global vp931_vsync_callback
vp931_vsync_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber

	; g_bVP931VsyncActive = 1
	ldi r30, 1
	sts g_bVP931VsyncActive, r30

	; disable vsync interrupt
	; (this is particularly important when using a hardware debugger)
	DISABLE_VSYNC_INT_ASM

	ret

//////////////////////////////////////////////////////////////////////////////////////////////

// WREN' interrupt, this is the only place that can use it because performance is critical
.global INT1_vect
INT1_vect:

	; don't need to preserve flags here because in, sts, and cbi do not change them and it is critical that we read the data lines as fast as possible.

	push r29	; does not mess with flags (2 clocks)

	; read data lines as fast as possible (needed for FFR, which has a very short pulse width)
	in	r29, _SFR_IO_ADDR(PINA)	; in does not mess with flags	 (1 clock)

	; lower DAK super fast (second most important thing behind reading the data lines) to meet demands of firefox
	cbi _SFR_IO_ADDR(PORTD), 5

	; preserve registers and flags
	push r2
	in	r2, _SFR_IO_ADDR(SREG)
	push r28
	push r30
	push r31

	; Loop in an attempt to read the data bus at the last possible moment before WREN' goes high.
	; This gives the highest chance of getting a successful read.
while_wren_is_low:
	mov r28, r29	; r28 will always contain a known good read
	in r29, _SFR_IO_ADDR(PINA)	; read the latest data on the bus (it will be valid as long as WREN' is still low)
	in r30, _SFR_IO_ADDR(PIND)	; check to see if WREN' is still low
	sbrs r30, 3	// D3 is WREN', skip next instruction if it is set
	rjmp while_wren_is_low

	; at this point, r28 has our good data

	lds r30, g_u8VP931CmdBytesReceived

	; have we received our max # of cmd bytes?
	cpi r30, VP931_CMD_BYTE_ARRAY_SIZE

	; if so (r30-VP931_CMD_BYTE_ARRAY_SIZE has no carry) then we have overflowed and won't store the command
	; Increasing the buffer size will fix this
	brcc wren_overflow

	; prepare for addition (subtraction)
	clr r31

	; Z (r30,r31) = &g_arrVP931CmdBytes + r30
	; (need to use subtraction because AVR does not have an immediate add function)
	subi r30, lo8(-(g_arrVP931CmdBytes))
	sbci r31, hi8(-(g_arrVP931CmdBytes))

	; store the good received byte into our cmd byte array
	st Z, r28

	; increment received count
	lds r30, g_u8VP931CmdBytesReceived
	inc r30
	sts g_u8VP931CmdBytesReceived, r30

	; skip section that sets overflow
	rjmp wren_change_dak_high

wren_overflow:
	; notify C code that we overflowed so it can log it
	ldi r30, 1
	sts g_bVP931CmdOverflow, r30

wren_change_dak_high:
	; now change DAK to high and lower DAV'
	sbi _SFR_IO_ADDR(PORTD), 5

	; NOTE : we do not want to change PORTA from being 0 at this point to avoid bus contention.
	; Firefox has resistors to (slowly?) pull all the lines high when they are not being driven.

	; clear our INT0 flag in case we got some spurious RDEN's before we lower DAV'
	sbi _SFR_IO_ADDR(EIFR), 0

	; lower DAV', raise DAV to mimic VP931 behavior (FFR seems to depend on this, FFX doesn't care)
	cbi _SFR_IO_ADDR(PORTC), 1
	sbi _SFR_IO_ADDR(PORTC), 0

wren_done:

	; NOTE : we have to leave this interrupt enabled to catch the next one

	pop r31
	pop r30
	pop r28
	out	_SFR_IO_ADDR(SREG), r2
	pop r2
	pop r29

	reti

////////////////////////////////////////////////////////////////

// RDEN' interrupt, performance critical for freedom fighter which only keeps it low for a very brief time
.global INT0_vect
INT0_vect:

	; data is already on the bus (via the timer1 interrupt) or else it doesn't need to be on the bus (cmd phase..)

	; raise DAV' super fast to meet demands of firefox (does not affect flags)
	sbi _SFR_IO_ADDR(PORTC), 1

	; lower DAV super fast to meet demands of freedom fighter (does not affect flags)
	cbi _SFR_IO_ADDR(PORTC), 0

	; preserve registers and flags
	push r2
	in	r2, _SFR_IO_ADDR(SREG)
	push r29
	push r30
	push r31

	; loop waiting for RDEN' to go high (it's the best for performance, sadly)
while_rden_is_low:
	in r30, _SFR_IO_ADDR(PIND)
	sbrs r30, 2	// D2 is RDEN', skip next instruction if it is set
	rjmp while_rden_is_low

	; RDEN' has gone up.

	; increment status byte sent count
	lds r30, g_u8VP931StatusBytesSent
	inc r30
	sts g_u8VP931StatusBytesSent, r30

	; have we sent VP931_STATUS_BYTE_ARRAY_SIZE (or more) bytes?
	cpi r30, VP931_STATUS_BYTE_ARRAY_SIZE

	; if so (r30-VP931_STATUS_BYTE_ARRAY_SIZE has no carry) then we are in the cmd stage
	brcc rden_in_cmd_stage

	; else we have another status byte to send

	; prepare to add (using subtraction) to array's base address
	; r30 contains the counter already
	clr r31

	; Z (r30,r31) = &g_arrVP931StatusBytes + r30
	; (need to use subtraction because AVR does not have an immediate add function)
	subi r30, lo8(-(g_arrVP931StatusBytes))
	sbci r31, hi8(-(g_arrVP931StatusBytes))

	; r29 = next status byte to send
	ld r29, Z

	; update byte that we are outputting
	; (we should already be in output mode at this point and if we are not, it's a bug)
	out _SFR_IO_ADDR(PORTA), r29

	; lower DAV', raise DAV (this intentionally occurs right after we change the output byte)
	cbi _SFR_IO_ADDR(PORTC), 1
	sbi _SFR_IO_ADDR(PORTC), 0

	rjmp rden_done

rden_in_cmd_stage:

	; if our status byte count did not exactly equal VP931_STATUS_BYTE_ARRAY_SIZE, we can skip the transition to cmd stage
	; (and this may help to skip, I'm not sure)
	brne rden_done

	clr r30

	; ensure that we are in read mode on the data lines
	out _SFR_IO_ADDR(DDRA), r30

	; ensure all pull-ups are disabled (and that we don't have a strong transition if we are leaving output mode)
	out _SFR_IO_ADDR(PORTA), r30

	; clear our INT1 flag in case we got some spurious WREN's during the status phase
	; (I've observed this on real firefox hardware)
	sbi _SFR_IO_ADDR(EIFR), 1

	; start accepting commands
	ENABLE_INT1

rden_done:

	; NOTE : we have to leave this interrupt enabled to catch the next one

	pop r31
	pop r30
	pop r29
	out	_SFR_IO_ADDR(SREG), r2
	pop r2

	reti
