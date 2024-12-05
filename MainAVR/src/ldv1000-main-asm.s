#include <avr/io.h>
#include "ldv1000-common.h"
#include "common-asm.h"

; apparently this 'extern' statement is not needed but I'll include it anyway
.extern g_u8CurField
.extern g_u8LDV1000Stage

.global ldv1000_vsync_callback
ldv1000_vsync_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber
	push r18

	; g_u8CurField ^= 1
	lds r30, g_u8CurField
	ldi r18, 1
	eor r30, r18
	sts g_u8CurField, r30

	; OCR1A = CYCLES_UNTIL_STATUS_STARTS
	ldi r31, hi8(CYCLES_UNTIL_STATUS_STARTS-2)
	ldi r30, lo8(CYCLES_UNTIL_STATUS_STARTS-2)
	sts OCR1AH, r31	; high byte must be written first since this is a 16-bit value (search datasheet for 'Accessing 16-bit Registers' section)
	sts OCR1AL, r30

	; this is currently redundant because r30 is already 0 but it is only 1 cycle and CYCLES_UNTIL_STATUS_STARTS may change in the future
	clr r30

	; TCNT1 = 0   my previous comment "this must be set after OCR1A is set because otherwise it is in an unknown state" I'm not sure I still agree but whatever
	sts TCNT1H, r30	; high byte must be written first since this is a 16-bit value (search datasheet for 'Accessing 16-bit Registers' section)
	sts TCNT1L, r30

	; TIFR1 = (1 << OCF1A)  make sure CTC interrupt does not fire as soon as this ISR returns (writing a 1 clears it)
	ldi r30, (1 << OCF1A)
	out _SFR_IO_ADDR(TIFR1), r30

	; g_u8LDV1000Stage = STAGE_VSYNC_STARTED
	;ldi r18, STAGE_VSYNC_STARTED	; this is redundant since r18 already contains correct value, but it is only 1 cycle and STAGE_VSYNC_STARTED's value could change in the future
	sts g_u8LDV1000Stage, r18

	; disable vsync interrupt
	; (this is particularly important when using a hardware debugger)
	DISABLE_VSYNC_INT_ASM

	; restore regs we clobbered
	pop r18

	ret

; /////////////////////////////////////////////////////////////////////////

; TIMER1_COMPA_vect
.global ldv1000_timer1_callback
ldv1000_timer1_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber
	
	; switch (g_u8LDV1000Stage)
	LDS       R30, g_u8LDV1000Stage

	; is it stage 2?
	CPI       R30,STAGE_STATUS_STARTED
	BRNE      more_checking
	RJMP      stage2

more_checking:
	; is it stage 3?
	CPI       R30,STAGE_STATUS_STOPPED
	BRCC      yet_more_checking

	; is it stage 1?
	CPI       R30,STAGE_VSYNC_STARTED
	BRNE      timer1_done

	; STAGE 1 STARTS HERE (STAGE_VSYNC_STARTED)

   	// Dragon's Lair reads the data as soon as status line is enabled, so we need to be in output mode
	//  with the data already present before we enabled status.
	// Warren did some tests with a real LD-V1000 and learned that one signal controls the output mode and the status strobe,
	//  so we should put them as close together as possible.

	;DATA_PORT_WRITE = g_u8LDV1000Status;	// put status on I/O pins
	LDS       R30,g_u8LDV1000Status
	OUT       _SFR_IO_ADDR(PORTA),R30

	// DATA_ENABLE_WRITE();	// go into output mode
	SER       R30
	OUT       _SFR_IO_ADDR(DDRA),R30

	// ENABLE_STATUS_STROBE();
	CBI       _SFR_IO_ADDR(PORTC),PC0

	// OCR1A = CYCLES_UNTIL_STATUS_ENDS;
	ldi r31, hi8(CYCLES_UNTIL_STATUS_ENDS-2)
	ldi r30, lo8(CYCLES_UNTIL_STATUS_ENDS-2)
	sts OCR1AH, r31	; high byte must be written first since this is a 16-bit value (search datasheet for 'Accessing 16-bit Registers' section)
	sts OCR1AL, r30

	// g_u8LDV1000Stage = STAGE_STATUS_STARTED;
	LDI       R30,STAGE_STATUS_STARTED
	STS       g_u8LDV1000Stage,R30

timer1_done:
	ret

yet_more_checking:
	CPI       R30,STAGE_STATUS_STOPPED
	BREQ      stage3
	CPI       R30,STAGE_CMD_STARTED
	BRNE      timer1_done

	; STAGE 4 starts here (STAGE_CMD_STARTED)

	; g_u8LDV1000Cmd = DATA_PORT_READ
	IN        R30,_SFR_IO_ADDR(PINA)
	STS       g_u8LDV1000Cmd,R30

	; TODO : read ENTER line

	; DISABLE_CMD_STROBE();
	SBI       _SFR_IO_ADDR(PORTC),PC1

	; DISABLE_CTC_INT();	// disable CTC interrupt because we don't have any more timed events coming up until the next vsync
	LDS       R30,0x006F
	ANDI      R30,0xFD
	STS       0x006F,R30

	; g_u8LDV1000Stage = STAGE_CMD_STOPPED;
	LDI       R30,STAGE_CMD_STOPPED
	STS       g_u8LDV1000Stage,R30

	ret

stage3:
// STAGE 3

	; ENABLE_CMD_STROBE();
	CBI       _SFR_IO_ADDR(PORTC),PC1

	// OCR1A = CYCLES_UNTIL_CMD_ENDS;
	ldi r31, hi8(CYCLES_UNTIL_CMD_ENDS-2)
	ldi r30, lo8(CYCLES_UNTIL_CMD_ENDS-2)
	sts OCR1AH, r31	; high byte must be written first since this is a 16-bit value (search datasheet for 'Accessing 16-bit Registers' section)
	sts OCR1AL, r30

	// g_u8LDV1000Stage = STAGE_CMD_STARTED;
	LDI       R30,STAGE_CMD_STARTED
	STS       g_u8LDV1000Stage,R30

	ret

	// Dragon's Lair goes into output mode the instant that it sees that we've disabled status.
	// So we should go into input mode right before we disable it.
	// UPDATE: Super don uses an LS374 positive-edge triggered flip flop, which means that it will
	//	store the status data as soon as status strobe goes high (de-activates).
	// That means we need to de-activate the status strobe _before_ we go into input mode.
	// NOTE: the real LD-V1000 seems to hold status for 5 uS after status strobe stops.  That complicates our design so we should avoid it if we can get away with it.

stage2:
	; DISABLE_STATUS_STROBE();
	SBI       _SFR_IO_ADDR(PORTC),PC0
	
	// OCR1A = CYCLES_UNTIL_CMD_STARTS;
	// this code comes before going into input mode to allow time for Super Don's LS374 to read the status (better than NOPs)

	ldi r31, hi8(CYCLES_UNTIL_CMD_STARTS-2)
	ldi r30, lo8(CYCLES_UNTIL_CMD_STARTS-2)
	sts OCR1AH, r31	; high byte must be written first since this is a 16-bit value (search datasheet for 'Accessing 16-bit Registers' section)
	sts OCR1AL, r30

	; g_u8LDV1000Stage = STAGE_STATUS_STOPPED;
	LDI       R30, STAGE_STATUS_STOPPED
	STS       g_u8LDV1000Stage,R30

		// enable pull-ups in read-mode (LD-V1000 does this)
		// Warren noticed that enabling the pull-ups seems to take a long time if we're already in read-mode,
		//  so I'm trying to set the bits high before going into read mode.

	; DATA_PORT_WRITE=0xFF;
	SER       R30
	OUT       _SFR_IO_ADDR(PORTA),R30

	; DATA_ENABLE_READ();
	CLR	R31	; NOTE: this is the only place where the cycle count is 1 off from the C code, but I am pretty sure it is okay/harmless
	OUT       _SFR_IO_ADDR(DDRA),R31

	ret
