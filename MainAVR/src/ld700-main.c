#include <avr/io.h> 
//#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/ld700-interpreter.h>
#include "ld700-common.h"
#include "ld700-callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "strings.h"
#include "idle.h"
#include "vsync.h"
//#include "util.h"
//#include "led_driver.h"
//#include "timer-global.h"
#include "timer1.h"	// for setting the timer1 callback
#include "ld700-main.h"
#include "common-ldp.h"	// for video callback definition

/////////////////////////////////////////////////////////////////

typedef struct
{
	uint16_t u16CyclesTilTimeout;	// if we go this many cycles without a change in the line, then something is wrong and we'll retry
	uint16_t u16CyclesMin;			// if the EXT_CTRL' signal changes before this many cycles has elapsed, then something is wrong and we'll retry
	uint8_t u8ExpectedSignalChange;	// the expected EXT_CTRL' signal change that 'signals' the end of the current stage
} ld700_cycles;

// used by ISR to validate that the incoming EXT_CTRL' pulses conform to the correct cycle timing
ld700_cycles g_ld700_cycles[4] =
{
	{0, 0, 0},	// before the leader begins (the pulse stays high during idle times). signal goes low (0) at the end of this stage.
	{CYCLES_TIL_8MS_TIMEOUT, CYCLES_8MS_MIN, 1},	// for when leader goes low for 8ms. signal goes high (1) at the end of this stage.
	{CYCLES_TIL_4MS_TIMEOUT, CYCLES_4MS_MIN, 0},	// for when leader goes high for 4ms.  signal goes low (0) at the end of this stage.
	{CYCLES_TIL_TIMEOUT, 0, 0}	// for when we are receiving the actual bits.  the second and third value are ignored
};

// used only by ISR
uint8_t g_ld700_u8ReceivingStage = STAGE_WAITING_FOR_8MS;
uint8_t g_ld700_u8Message = 0;
uint8_t g_ld700_u8ReceivedBitCount = 0;

// shared by ISR.  The finished byte that the ISR has for us to process.
volatile uint8_t g_ld700_u8FinishedByte = 0;

// shared by ISR.  Gets set to 1 when ISR has a full byte ready for us to process.  We need to set this back to 0 when we've processed the byte so we can detect the next one.
volatile uint8_t g_ld700_u8FinishedByteReady = 0;

// PA0 is PCINT0, which is part of PCIE0 (PCINT7..0)
#define SETUP_LD700_INT() PCMSK0 = (1 << PCINT0)

// enable/disable PCIE0 interrupt group
#define ENABLE_LD700_INT() PCICR |= (1 << PCIE0)
#define DISABLE_LD700_INT() PCICR &= ~(1 << PCIE0)

// EXT_CTRL is PA0
#define LD700_EXT_CTRL (PINA & 1)

/////////////////////////////////////////////////////////

void ld700_main_loop()
{
	uint8_t u8NextField = 0;
	
	// so we can clean-up ourselves when we exit
	uint8_t u8DDRAStored = DDRA;
	uint8_t u8PCMSK0Stored = PCMSK0;
	uint8_t u8PCICRStored = PCICR;
	uint8_t u8PORTAStored = PORTA;
	uint8_t u8TCCRB1BStored = TCCR1B;

	// Setup 16-bit timer
	TCCR1B = (1 << CS11) | (1 << WGM12);	// /8 prescaling to fit large timeout values (search datasheet for TCCR1B for details), CTC mode enabled

	// Inputs
	// PA0: EXT_CTRL'
	// PA1: INT/EXT'
	// PA2: Flip Disc Button (active low)
	DDRA &= ~((1 << PA0) | (1 << PA1) | (1 << PA2));
	
	// Output
	// PA5: Side 2 LED
	// PA6: Side 1 LED (if SSR is enabled)
	// PA7: EXT_ACK'

	DDRA |= ((1 << PA5)|(1 << PA6)|(1 << PA7));

	ld700_setup_callbacks();

	SETUP_LD700_INT();

	{
		char s1[20];
		char s[30];	// count this out to make sure it won't overflow (I already did)
		string_to_buf(s, STRING_LD700);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
	}

	ld700i_reset();

	// to handle EXT_CTRL' timeouts
	set_timer1_isr_callback(ld700_on_ext_ctrl_timeout);
	
	// to handle EXT_CTRL' events
	set_pcint0_isr_callback(ld700_on_ext_ctrl_changed);

	// start receiving interrupts from EXT_CTRL' pin
	ENABLE_LD700_INT();

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// wait for vsync to start
		while (GOT_VSYNC() == 0)
		{
			// if we have a message ready to process
			if (g_ld700_u8FinishedByteReady)
			{
				// send received byte to interpreter
				ld700i_write(g_ld700_u8FinishedByte, ld700_convert_status(ldpc_get_status()));
				
				// set flag back to 0 so we can detect when the next byte has come in
				g_ld700_u8FinishedByteReady = 0;
				
				// if diagnostics mode is enabled log the incoming byte
				if (IsDiagnosticsEnabledEeprom())
				{
					MediaServerSendRxLog(g_ld700_u8FinishedByte);
				}
			}
			
			// use idle time to process low-priority tasks
			idle_think();
			
			// if 'flip disc' button is being pressed
			if ((PINA & (1 << PA2)) == 0)
			{
				LDPCStatus_t status = ldpc_get_status();
				
				// (for now) we only want to take action if the disc is stopped. This saves us from having to implement a debouncer, and also gives
				//   a quick way to override Halcyon's persistent efforts to eject the disc.
				if (status == LDPC_STOPPED)
				{
					ld700_close_tray();
				}
			}

 			// if user presses MODE button then abort (this is needed in case we have no video signal plugged in)
			if (g_bRestartPlayer)
			{
				goto done;
			}
		}

		// so we are ready for the next vsync
		CLEAR_VSYNC_FLAG();

		// Track the next field (instead of the current one) since whatever we tell the media server to render won't be rendered until the next field.
		// This, in effect, keeps us in sync with what is actually being displayed.

		// if we are debugging, fake the next field to make debugging easier, else use the actual next field from the hardware
#ifdef DEBUG
		u8NextField ^= 1;
#else
		// The LM1881 uses 0 for even/bottom and 1 for top/odd (backward from our convention) but since we want the next field (not the current one),
		//   this works out.
		u8NextField = FIELD_PIN;
#endif

		ldpc_OnVBlankChanged(
			LDPC_TRUE,	// vblank _is_ active
			u8NextField);

		ldpc_OnVBlankChanged(
			LDPC_FALSE,	// vblank is no longer active (or at least, this is a good place to end it)
			u8NextField);

		// do common operations
		on_video_field();

		// do interpreter stuff that happens after vblank.
		// I put this after on_video_field so that performance of the video fields to the media server stays fairly constant.
		ld700i_on_vblank(ld700_convert_status(ldpc_get_status()));
	}

done:

	// clean-up	
	DISABLE_LD700_INT();
	DISABLE_CTC_INT();
	TCCR1B = u8TCCRB1BStored;
	PCICR = u8PCICRStored;	// this will disable interrupts so we call it first
	PCMSK0 = u8PCMSK0Stored;	
	DDRA = u8DDRAStored;
	PORTA = u8PORTAStored;
}
