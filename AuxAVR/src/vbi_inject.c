#include <stdio.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include "vbi_inject.h"
#include "protocol.h"
#include "serial.h"

////////////////////////////////////////////////////////////

volatile uint8_t g_u8Stage = STAGE_WAITING_FOR_VSYNC;
volatile uint32_t g_u32Line = 0;

void do_picture_number(uint32_t u32PicNum);	// assembly

// these are arrays to indicate even and odd fields (0 and 1 respectively)
uint8_t g_u8WhiteFlags[2] = { 1, 0};	// on/off

uint32_t g_u32PicNums[3][2] =
{
{ 0xF86767, 0xF86760 },	// line 16
{ 0xF81313, 0x800001 },	// line 17
{ 0xF83210, 0x800002 },	// line 18
};

// whether we should unmute the video on the next vsync (so star rider doesn't parse bad data)
volatile uint8_t g_bUnmuteQueued = 0;

int main (void) 
{
	SETUP_WHITE();
	SETUP_CONTROL();
	SETUP_PR8210A_STAND_BY();
	SETUP_PR8210A_VSYNC();

	// setup 16-bit timer, no CTC mode
	TCCR1A = 0;
	TCCR1B = (1 << CS10);	// no prescaling (search datasheet for TCCR1B for details)

	// setup interrupts
	SETUP_VSYNC_INT();
	SETUP_CSYNC_INT();

	// enable interrupts
	ENABLE_VSYNC_INT();

	// vsync ISR enables csync int
	DISABLE_CSYNC_INT();

	serial_init();

	ProtocolSetup();

	// enable all interrupts now that our setup is complete
	sei();

	// announce that we've booted successfully
	SendLog("Am here allos");

	// main loop
	for (;;)
	{	
		// make sure extended vsync pulse is disabled if we're after line ~15-16 (approximately like the pr-8210a does, star rider relies on this long vsync pulse)
		if (TCNT1 > CYCLES_TIL_LINE16)
		{
			DISABLE_PR8210A_VSYNC();
		}

		// wait for incoming serial data
		io_think();
	}

	// no point shutting down serial since we never end
}

//////////////////////////////

// vsync interrupt
ISR(INT0_vect)
{
	ENABLE_CSYNC_INT();
	TCNT1 = 0;	// reset timer so we can figure out which line number we're on

	// enable extended vsync pulse that lasts a long time so that star rider works
	ENABLE_PR8210A_VSYNC();

	g_u8Stage = STAGE_WAITING_FOR_LINE11;

	// we are entering critical section where we don't want any other interrupts firing
	// (we should try very hard not to send any serial data during this right after vsync)
	SERIAL_INT_DISABLE();

	if (g_bUnmuteQueued)
	{
		DISABLE_VIDEO_MUTE();
		g_bUnmuteQueued = 0;
	}

	DISABLE_VSYNC_INT();	// important for debugging
}
