#include <avr/io.h>
#include "ld700_main_isr.h"

uint8_t LD700_EXT_CTRL = 0;
uint8_t g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;
uint8_t g_ld700_u8Message = 0;
uint8_t g_ld700_u8ReceivedBitCount = 0;
volatile uint8_t g_ld700_u8FinishedByte = 0;
volatile uint8_t g_ld700_u8FinishedByteReady = 0;
uint16_t OCR1A = 0;

typedef struct
{
	uint16_t u16CyclesTilTimeout;
	uint16_t u16CyclesMin;
	uint8_t u8ExpectedSignalChange;
} ld700_cycles;

ld700_cycles g_ld700_cycles[4] =
{
	{0, 0, 0},
	{CYCLES_TIL_8MS_TIMEOUT, CYCLES_8MS_MIN, 1},
	{CYCLES_TIL_4MS_TIMEOUT, CYCLES_4MS_MIN, 0},
	{CYCLES_TIL_TIMEOUT, 0, 0}	// second and third value are ignored
};

void PCINT0_vect()
{
	// store hardware values as close to when interrupt started to maximize accuracy
	const uint16_t u16Timer = TCNT1;
    const uint8_t u8LD700ExtCtrl = LD700_EXT_CTRL;

	// before the pulses, the logic is similar
	if (g_ld700_u8ReceivingStage < STAGE_PULSES_STARTED)
	{
		if	(
					// if we got an unexpected pulse change
					(u8LD700ExtCtrl != g_ld700_cycles[g_ld700_u8ReceivingStage].u8ExpectedSignalChange) ||
					// or if the length of the previous pulse is too short
					(u16Timer < g_ld700_cycles[g_ld700_u8ReceivingStage].u16CyclesMin)
					// then reset and start over because we're not seeing what we're expecting to see (not in sync)
			)
		{
			goto reset;
		}

		g_ld700_u8ReceivingStage++;

		// get into a predictable state
		DISABLE_CTC_INT();

		OCR1A = g_ld700_cycles[g_ld700_u8ReceivingStage].u16CyclesTilTimeout;	// make interrupt occur when it's been too long since we've got another pulse

		// we are at beginning of new stage, so reset timer to look for the next pulse
		TCNT1 = 0;	// make sure timer is cleared after OCR1A set but before we clear CTC flag, to ensure we don't have any false positives

		TIFR1 |= (1 << OCF1A); // clear the CTC flag so we don't immediately trigger ISR (writing a logic one to the set flag clears it)
		ENABLE_CTC_INT();	// start waiting for timeout

		goto done;
	}

	// If we get this far, we're doing bit pulses

    // at this point, we only care about the pulse going low because we measure the duration between pulses going low to determine if it's a 0 or 1 a bit
    if (u8LD700ExtCtrl != 0)
    {
    	goto done;
    }

    // whether it's 0 or 1, we always shift
	g_ld700_u8Message >>= 1;

	// if it's a 1 bit
	if (u16Timer >= CYCLES_TIL_ITS_A_1)
	{
		// 1 bit!
		g_ld700_u8Message |= (1 << 7);	// byte is sent least-significant bit first
	}

	// if program flow comes here, it means that we processed a 0 or 1 bit
	TCNT1 = 0;	// we are at beginning of new pulse, so reset timer to look for the next pulse
	g_ld700_u8ReceivedBitCount++;

	// if we've processed all 8 bits, we're done
	if ((g_ld700_u8ReceivedBitCount & 7) == 0)
	{
		g_ld700_u8FinishedByte = g_ld700_u8Message;	// copy so we can start receiving the next one before this one is processed (probably not necessary, just a precaution)
		g_ld700_u8FinishedByteReady = 1;	// tell non-ISR code that finished message is ready

		// if we've received our 4 bytes
		if (g_ld700_u8ReceivedBitCount == 32)
		{
			goto reset;
		}
	}
	goto done;

reset:
	g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;
	g_ld700_u8ReceivedBitCount = 0;

done:
	return;
}
