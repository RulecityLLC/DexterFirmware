#include <avr/io.h>
#include "common.h"
#include "common-asm.h"

; apparently this 'extern' statement is not needed but I'll include it anyway
.extern g_u8PR8210VsyncActive
.extern g_pr8210_u8ReceivingMessage
.extern g_u8PR8210AGotJumpTriggerThisField

.global pr8210_vsync_callback
pr8210_vsync_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber

	; clear TCNT2 as soon as possible so we get max accuracy
	; TCNT2 = 0
	clr r30
	sts TCNT2, r30

	; reset flag that indicates whether we got a jump trigger or not
	sts g_u8PR8210AGotJumpTriggerThisField, r30

	; g_u8PR8210VsyncActive = 1
	ldi r30, 1
	sts g_u8PR8210VsyncActive, r30

	; disable vsync interrupt
	; (this is particularly important when using a hardware debugger)
	DISABLE_VSYNC_INT_ASM

	ret

; TIMER1_COMPA_vect
.global pr8210_timer1_callback
pr8210_timer1_callback:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber

	; g_pr8210_u8ReceivingMessage = 0	
	clr r30
	sts g_pr8210_u8ReceivingMessage, r30

	; don't come back in here until we get a new pulse
	DISABLE_CTC_INT_ASM r30

	ret
