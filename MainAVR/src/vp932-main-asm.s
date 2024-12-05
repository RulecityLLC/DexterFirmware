#include <avr/io.h>
#include "common.h"
#include "common-asm.h"

; apparently this 'extern' statement is not needed but I'll include it anyway
.extern g_bVP932VsyncActive

.global vp932_vsync_callback
vp932_vsync_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber

	ldi r30, 1
	sts g_bVP932VsyncActive, r30

	; disable vsync interrupt
	; (this is particularly important when using a hardware debugger)
	DISABLE_VSYNC_INT_ASM

	ret
