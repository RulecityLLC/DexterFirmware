#include <avr/io.h>
#include "common.h"
#include "common-asm.h"

; apparently this 'extern' statement is not needed but I'll include it anyway
.extern g_u8LDP1000CurField
.extern g_u8LDP1000VsyncActive

.global ldp1000_vsync_callback
ldp1000_vsync_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber
	push r18

	; g_u8CurField2 ^= 1
	lds r30, g_u8LDP1000CurField
	ldi r18, 1
	eor r30, r18
	sts g_u8LDP1000CurField, r30

	sts g_u8LDP1000VsyncActive, r18

	; disable vsync interrupt
	; (this is particularly important when using a hardware debugger)
	DISABLE_VSYNC_INT_ASM

	; restore regs we clobbered
	pop r18

	ret
