#include <avr/io.h>
#include "common.h"
#include "common-asm.h"
#include "ld700-common.h"

.global	ld700_on_ext_ctrl_changed
ld700_on_ext_ctrl_changed:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber

	push r1
	clr r1
	push r18
	push r19
	push r20
	push r21
	push r24
	push r25

	; const uint16_t u16Timer = TCNT1;
	lds r20,TCNT1L
	lds r21,TCNT1H
	; r24 = PA0
	in r24, _SFR_IO_ADDR(PINA)
	andi r24, 1
	; if (g_ld700_u8ReceivingStage >= STAGE_PULSES_STARTED) goto do_bit_pulses;
	lds r25,g_ld700_u8ReceivingStage
	cpi r25,STAGE_PULSES_STARTED
	brsh .do_bit_pulses
	
	; R18:R19 = g_ld700_u8ReceivingStage
	mov r18,r25
	ldi r19,0
	
	; R30:31 = g_ld700_u8ReceivingStage
	; prepare for 16-bit operations
	movw r30,r18
	
	; R30:R31 = g_ld700_u8ReceivingStage * 5
	; The g_ld700_cycles structure is 5 bytes so the index (g_ld700_u8ReceivingStage) needs to be multiplied by 5
	lsl r30
	rol r31
	lsl r30
	rol r31
	add r30,r18
	adc r31,r19
	
	; R30:R31 += g_ld700_cycles (no immediate add, so have to use subtraction)
	subi r30,lo8(-(g_ld700_cycles))
	sbci r31,hi8(-(g_ld700_cycles))

	; R18 = u8ExpectedSignalChange for the current stage (4 is the offset for this byte)
	ldd r18,Z+4
	
	; compare u8ExpectedSignalChange and PA0
	cp r18,r24
	
	; if we get what we expect, then proceed to handle the current stage
	breq .do_pre_bits_stage

	; if we come here, we aren't seeing what we expect, so we'll reset and try again
.reset:

	; g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;
	sts g_ld700_u8ReceivingStage,r1
	
	; g_ld700_u8ReceivedBitCount = 0;
	sts g_ld700_u8ReceivedBitCount,r1

.done:
	pop r25
	pop r24
	pop r21
	pop r20
	pop r19
	pop r18
	pop r1

	ret

.do_bit_pulses:

	; at this point, we only care about the pulse going low because we measure the duration between pulses going low to determine if it's a 0 or 1 a bit
	; if (PA0 != 0) goto done;
	cpse r24,r1
	rjmp .done
	
	; g_ld700_u8Message >> 1;
	lds r25,g_ld700_u8Message
	lsr r25

	; R20 = TCNT1L
	; R21 = TCNT1H
	; CYCLES_TIL_ITS_A_1 is 3456 (0xD80)
	; if (u16Timer >= CYCLES_TIL_ITS_A_1)
	cpi r20,lo8(CYCLES_TIL_ITS_A_1)
	sbci r21,hi8(CYCLES_TIL_ITS_A_1)	; R21 = R21 - hi8(CYCLES_TIL_ITS_A_1) - Carry
	
	; branch if carry is set (if u16Timer < CYCLES_TIL_ITS_A_1 which means we've received a 0 bit)
	brlo .do_bit_pulses_after_msg_shift

	; If we come here, we've got a 1 bit so we need to set the high bit
	; g_ld700_u8Message |= 0x80
	ori r25, -128
	
.do_bit_pulses_after_msg_shift:

	; store current message back to memory
	sts g_ld700_u8Message,r25

	; Clear timer so we can detect the next pulse duration
	; TCNT1 = 0
	sts TCNT1H,r1
	sts TCNT1L,r1

	; g_ld700_u8ReceivedBitCount++;
	lds r24,g_ld700_u8ReceivedBitCount
	subi r24,-1
	sts g_ld700_u8ReceivedBitCount,r24

	; if ((g_ld700_u8ReceivedBitCount & 7) != 0) goto done;
	mov r18,r24
	andi r18,7
	brne .done

	;g_ld700_u8FinishedByte = g_ld700_u8Message;
	sts g_ld700_u8FinishedByte,r25

	; g_ld700_u8FinishedByteReady = 1;	// tell non-ISR code that finished message is ready
	ldi r25,1
	sts g_ld700_u8FinishedByteReady,r25

	; if (g_ld700_u8ReceivedBitCount == 32) goto reset; else goto done;
	cpi r24,32
	brne .done
	rjmp .reset

.do_pre_bits_stage:

	; R18:R19 = u16CyclesMin
	ldd r18,Z+2
	ldd r19,Z+3
	
	; compare TCNT1 snapshot with minimum cycles needed for a pulse to be valid
	cp r20,r18
	cpc r21,r19
	
	; if TCNT1 snapshot is less than the minimum cycles needed, then reset because we're not seeing what we're expecting to see (not in sync)
	brlo .reset

	; g_ld700_u8ReceivingStage++;
	; At this point, r25 contains g_ld700_u8ReceivingStage	
	subi r25,-1
	sts g_ld700_u8ReceivingStage,r25
	
	; get into a predictable state
	DISABLE_CTC_INT_ASM r24

	; R30:R31 = R24:R25 = g_ld700_u8ReceivingStage
	lds r30,g_ld700_u8ReceivingStage
	mov r24,r30
	ldi r25,0
	movw r30,r24

	; R30:R31 = g_ld700_u8ReceivingStage * 5
	; The g_ld700_cycles structure is 5 bytes so the index (g_ld700_u8ReceivingStage) needs to be multiplied by 5		
	lsl r30
	rol r31
	lsl r30
	rol r31
	add r30,r24
	adc r31,r25
	
	; R30:R31 += g_ld700_cycles (no immediate add, so have to use subtraction)
	subi r30,lo8(-(g_ld700_cycles))
	sbci r31,hi8(-(g_ld700_cycles))
	
	; r24:r25 = u16CyclesTilTimeout
	ld r24,Z
	ldd r25,Z+1
	
	; OCR1A = u16CyclesTilTimeout
	sts OCR1AH,r25
	sts OCR1AL,r24
	
	; we are at beginning of new stage, so reset timer to look for the next pulse
	; TCNT1 = 0
	sts TCNT1H,r1
	sts TCNT1L,r1
	
	; clear the CTC flag so we don't immediately trigger ISR (writing a logic one to the set flag clears it)
	; TIFR1 |= (1 << OCF1A);
	sbi _SFR_IO_ADDR(TIFR1),OCF1A
	
	; start waiting for timeout
	ENABLE_CTC_INT_ASM r24
	
	rjmp .done

; /////////////////////////////////////////////////////////////////////////////////////////
.global ld700_on_ext_ctrl_timeout
ld700_on_ext_ctrl_timeout:
	; NOTE: do not need to preserve r30, r31, or SREGS because the ISR handler does that already for us
	; however, we must preserve all other regs that we clobber

	clr r30
	; g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;
	sts g_ld700_u8ReceivingStage,r30
	
	; g_ld700_u8ReceivedBitCount = 0;
	sts g_ld700_u8ReceivedBitCount,r30

	; now that we've reset, we need the EXT_CTRL' ISR to set us up again before we come back in here
	DISABLE_CTC_INT_ASM r30

	ret
