#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/vp931-interpreter.h>
#include "protocol.h"
#include "settings.h"
#include "strings.h"
#include "idle.h"
#include "vsync.h"
#include "led_driver.h"
#include "vp931-main.h"
#include "vp931-callbacks.h"
#include "common-ldp.h"

// uncomment this to ignore the actual field and just toggle a field internally to make debugging easier
#ifdef DEBUG
#define VP931_DEBUG
#endif

volatile uint8_t g_bVP931VsyncActive = 0;
volatile uint8_t g_arrVP931StatusBytes[VP931_STATUS_BYTE_ARRAY_SIZE];	// the 6 status bytes that we send per field
volatile uint8_t g_u8VP931StatusBytesSent = 0;
volatile uint8_t g_arrVP931CmdBytes[VP931_CMD_BYTE_ARRAY_SIZE];	// the command bytes that we receive
volatile uint8_t g_u8VP931CmdBytesReceived = 0;
volatile uint8_t g_bVP931CmdOverflow = 0;

#define VP931_DO_IDLE() idle_think(); if (g_bRestartPlayer) { goto done; }

VP931Status_t vp931_convert_status(LDPCStatus_t status)
{
	VP931Status_t res = VP931_ERROR;

	switch (status)
	{
	case LDPC_SEARCHING:
		res = VP931_SEARCHING;
		break;
	case LDPC_PLAYING:
		res = VP931_PLAYING;
		break;
	case LDPC_PAUSED:
		res = VP931_PAUSED;
		break;
	case LDPC_SPINNING_UP:
		res = VP931_SPINNING_UP;
		break;
	case LDPC_STOPPED:
	default:
		res = VP931_ERROR;
		break;
	}
	return res;
}

void vp931_main_loop()
{
	uint8_t u8VsyncCounter = 0;
	uint8_t u8Field = 0;
	uint32_t u32Vbi = 0;
	LDPCStatus_t uLdpStatus = 0;

#ifdef VP931_DEBUG
	uint8_t u8LastField = 0;
#endif

	vp931_setup_callbacks();

	// Setup 16-bit timer
	// (other drivers may change this so we must set it here)
	TCCR1A = 0;
	TCCR1B = (1 << CS10);	// no prescaling (search datasheet for TCCR1B for details)

	// setup 8-bit timer, no CTC mode, /1024 prescaling
	TCCR2A = 0;
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);

	// 8 bit data port to Firefox
	DATA_CONTROL = 0x00;				
	DATA_PORT_WRITE = 0x00;

	DISABLE_DCE_AND_DTE();		// disable RS232 since we will be using the same lines on the DB25 port

	DDRC |= ((1 << PC1)|(1 << PC0));	// DAV' and DAV
	PORTC |= (1 << PC1);	// set DAV' high (disabled)
	PORTC &= ~(1 << PC0);	// set DAV low (disabled)

	ENABLE_OPTO_RELAY();		// enable opto relay because we are parallel mode (ie not RS232 mode)

	DDRD |= (1 << PD5);	// output mode - DAK 
	DDRD &= ~((1 << PD2)|(1 << PD3)|(1 << PD4));	// input mode - PD2=RDEN', PD3=WREN', PD4=RESET'
	PORTD |= ((1 << PD2)|(1 << PD3)|(1 << PD4)|(1 << PD5));	// enable pull-ups for all input lines, set DAK to high (disabled)

	set_vsync_isr_callback(vp931_vsync_callback);

	// for now, no spin-up delay
	common_enable_spinup_delay(0);

	// the VP-931 automatically plays on power-up
	ldpc_play(LDPC_FORWARD);

	// setup WREN' interrupt
	DISABLE_INT_WREN();	// disable WREN' interrupt while we do setup
	EICRA &= ~((1 << ISC10) | (1 << ISC11));	// zero out bits in preparation to set them
	EICRA |= (1 << ISC11);				// Falling edge of INT1 generates interrupt
	EIFR |= (1 << INTF1);	// clear bit if is happens to be set

	// setup RDEN' interrupt
	DISABLE_INT_RDEN();	// disable RDEN' interrupt while we do setup
	EICRA &= ~((1 << ISC00) | (1 << ISC01));	// zero out bits in preparation to set them
	EICRA |= (1 << ISC01);				// Falling edge of INT1 generates interrupt
	EIFR |= (1 << INTF0);	// clear bit if is happens to be set

	// reset interpreter
	vp931i_reset();

	// prepare for first iteration
	g_u8VP931CmdBytesReceived = 0;
	uLdpStatus = ldpc_get_status();

	// Go until our settings change;  the ISR's will work a lot during this
	while (!g_bRestartPlayer)
	{
		// don't accept WREN' except during a specific time
		DISABLE_INT_WREN();

		// probably wise not to accept RDEN' here either unless we discover otherwise through testing
		DISABLE_INT_RDEN();

		// get into a sane state
		PORTA = 0;
		DDRA = 0;
		RAISE_DAV_PRIME();
		RAISE_DAK();

		// I intentionally call this right _before_ vsync occurs so that any commands such as search/skip/play
		//	get executed during the next vblank.
		vp931i_on_vsync((const uint8_t *) g_arrVP931CmdBytes, g_u8VP931CmdBytesReceived, vp931_convert_status(uLdpStatus));

		// now that any cmds have been processed, reset the index
		g_u8VP931CmdBytesReceived = 0;
		g_bVP931CmdOverflow = 0;

		g_bVP931VsyncActive = 0;

		// (re)enable vsync interrupt
		ENABLE_VSYNC_INT();

		// wait for vsync
		while (!g_bVP931VsyncActive)
		{
			VP931_DO_IDLE();
		}

		// set up timer to wait for line 23-ish, as soon after vsync as possible to help accuracy
		OCR1A = CYCLES_TIL_LINE23;
		TCNT1 = 0;	// this must be set after OCR1A is set because otherwise it is in an unknown state
		TIFR1 |= (1 << OCF1A); // clear the output compare match flag in case it was set (writing 1 clears)

#ifdef VP931_DEBUG
		// more convenient to use a fake field value when debugging
		u8Field = u8LastField;
		u8LastField ^= 1;
#else
		// Use the real field value (instead of simulated) just to be more accurate
		// The LM1881 uses 0 for even/bottom and 1 for top/odd (backward from our convention)
		u8Field = FIELD_PIN ^ 1;
#endif

		ldpc_OnVBlankChanged(
			LDPC_TRUE,	// vblank _is_ active
			u8Field);

		u8VsyncCounter++;
		if (u8VsyncCounter & 0x8)
		{
			// blink the LED so we know the interrupt is working
			LED_VSYNC_PIN_TOGGLE();
			u8VsyncCounter = 0;
		}

		ldpc_OnVBlankChanged(
			LDPC_FALSE,	// vblank is no longer active (or at least, this is a good place to end it)
			u8Field);

		uLdpStatus = ldpc_get_status();

		// send field update if not seeking
		if (uLdpStatus != LDPC_SEARCHING)
		{
			// send current field to frame server
			unsigned long u = ldpc_get_current_abs_field();
			if (u != VBIMiniNoFrame)
			{
				MediaServerSendField(u, ldpc_get_audio_status());
			}
		}

		// grab the VBI data
		u32Vbi = ldpc_get_current_field_vbi_line18();

		// get status bytes to be sent
		vp931i_get_status_bytes(u32Vbi, vp931_convert_status(uLdpStatus), (uint8_t *) g_arrVP931StatusBytes);
		g_u8VP931StatusBytesSent = 0;

		// wait for line 23
		while (!TIMER1_TIMED_OUT())
		{
			VP931_DO_IDLE();
		}

		// reset long timeout timer
		OCR2A = TICKS_FOR_13MS;
		TCNT2 = 0;
		TIFR2 |= (1 << OCF2A); // clear the output compare match flag in case it was set (writing 1 clears)

		// if WREN' is low (FFX holds it low during RAM/ROM test)
		//  then don't do anything because it will cause bus contention and strain on the hardware
		//  to have both Dexter and FFX in output mode.
		if ((PIND & (1 << PD3)) == 0)
		{
			continue;
		}

		// jumpstart process to send status.
		// The ISRs will do the rest.
		PORTA = g_arrVP931StatusBytes[0];
		DDRA = 0xFF;
		EIFR |= (1 << INTF0);	// clear out any interrupts that occurred in invalid places
		ENABLE_INT_RDEN();
		LOWER_DAV_PRIME();

		// while we aren't getting close to the next field and will still accept I/O
		// (the ISRs are doing most of the work during this loop)
		while (!TIMER2_TIMED_OUT())
		{
			VP931_DO_IDLE();
		} // end while I/O window is active

		// if we overflowed, log it
		if (g_bVP931CmdOverflow)
		{
			char s[35];	// make this as small as possible
			string_to_buf(s, STRING_TOO_MANY_DIGITS);
			LOG(s);
			g_bVP931CmdOverflow = 0;
		}
	}

done:
	// clean-up

	// restore spin-up delay since we forcefully disabled it
	common_enable_spinup_delay(IsSpinupDelayEnabledEeprom());

	// other modes will expect it to be disabled, so disable it now as part of clean-up
	DISABLE_OPTO_RELAY();

	// disable interrupts
	DISABLE_VSYNC_INT();
	DISABLE_INT_WREN();
	DISABLE_INT_RDEN();
}
