#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/ldv1000-interpreter.h>
#include "ldv1000_callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "strings.h"
#include "idle.h"
#include "vsync.h"
#include "timer1.h"
#include "led_driver.h"
#include "common-ldp.h"
#include "ldv1000-main.h"

volatile uint8_t g_u8LDV1000Stage = STAGE_WAITING_FOR_VSYNC;
volatile uint8_t g_u8LDV1000Status = 0;
volatile uint8_t g_u8LDV1000Cmd = 0;

volatile unsigned char g_u8CurField = 0;	// the current field that we're on

void ldv1000_main_loop(LDPType ldptype)
{
	// for verbose logging
	uint8_t u8LastStatus = 0, u8LastCmd = 0;

	// Setup 16-bit timer
	// (other drivers may change this so we must set it here)
	TCCR1B = (1 << CS10) | (1 << WGM12);	// no prescaling (search datasheet for TCCR1B for details), CTC mode enabled

	// make sure RS232 is disabled and NAND gate is disabled (if NAND gate is enabled it interferes with pin A3)
	DISABLE_DCE_AND_DTE();

	// for safety reasons, opto relay should be disabled when DB25 port is not in use
	DISABLE_OPTO_RELAY();

	// enable output mode for strobes
	SETUP_STROBES();

	// set-up ENTER pin for LD-V1000 operation
	SETUP_LDV1000_ENTER();

	// turn off strobes by default
	DISABLE_STATUS_STROBE();
	DISABLE_CMD_STROBE();

	// pin 12 needs to be ground-friendly
	SETUP_LDV1000_GND();

	ldv1000_setup_callbacks();

	// if this is the Badlands-modified version of the LD-V1000
	if (ldptype == LDP_LDV1000_BL)
	{
		char s1[20];
		char s[30];	// count this out to make sure it won't overflow (I already did)
		string_to_buf(s, STRING_LDV1000);
		string_to_buf(s1, STRING_BADLANDS);
		strcat(s, s1);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
		reset_ldv1000i(LDV1000_EMU_BADLANDS);
	}
	// else it's the standard LD-V1000
	else
	{
		char s1[20];
		char s[25];
		string_to_buf(s, STRING_LDV1000);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
		reset_ldv1000i(LDV1000_EMU_STANDARD);
	}

	g_u8LDV1000Status = read_ldv1000i();	// prep for first loop

	set_timer1_isr_callback(ldv1000_timer1_callback);

	set_vsync_isr_callback(ldv1000_vsync_callback);
	ENABLE_VSYNC_INT();

	// ld-v1000 ignore stop codes
	SetHonorStopCodesMemory(0);

	// DEBUG: force play
//	write_ldv1000i(0xFF);
//	write_ldv1000i(0xFD);

	// Go until our settings change;  the ISR's will work a lot during this
	while (!g_bRestartPlayer)
	{
		// wait for vsync
		while (g_u8LDV1000Stage != STAGE_VSYNC_STARTED)
		{
			// use idle time to process low-priority tasks
			idle_think();

 			// if user presses MODE button then abort (this is needed in case we have no video signal plugged in)
			if (g_bRestartPlayer)
			{
				goto done;
			}
		}

		// entering timing critical section of code
		ENTER_CRITICAL_SECTION();

		// Enable timer interrupts to fire for LD-V1000 strobes.
		// interrupt will fire if following conditions are met:
		//	- global interrupts enabled (which will always be true in our code)
		//	- the OCF1A flag in TIFR1 is set (which happens once timer setup in vsync handler reaches a certain count)
		ENABLE_CTC_INT();

		// g_u8CurField is set by vsync ISR
		ldpc_OnVBlankChanged(
			LDPC_TRUE,	// vblank _is_ active
			g_u8CurField);

		// wait until ISR has done status/command strobes
		while (g_u8LDV1000Stage < STAGE_CMD_STOPPED)
		{
			// use idle time to process low-priority tasks
			idle_think_critical_section();
		}

		// timing critical section of code is done
		LEAVE_CRITICAL_SECTION();

		// This is a good spot to do this because we can use extra cycles here without worrying about missing our next interrupt.
		// It just needs to come before write_ldv1000 to be consistent with real ld-v1000 behavior.
		ldpc_OnVBlankChanged(
			LDPC_FALSE,	// vblank is no longer active (or at least, this is a good place to end it)
			g_u8CurField);

		// generic video work
		on_video_field();

		// if diagnostics mode is enabled, add some verbose logging
		if (IsDiagnosticsEnabledEeprom())
		{
			// print command every time it changes
			if (g_u8LDV1000Cmd != u8LastCmd)
			{
				char sdbg[6];	// keep this as small as possible
				sprintf(sdbg, "<- %x", g_u8LDV1000Cmd);
				LOG(sdbg);
				u8LastCmd = g_u8LDV1000Cmd;
			}
		}

		write_ldv1000i(g_u8LDV1000Cmd);

		// get the next status
		// (This is a good place to do it because we have plenty of cycles to use)
		// NOTE : we could disable interrupts here while we modify g_u8LDV1000Status since it is a shared variable,
		//  but since it is just 1 byte in memory, I think it any change operation on it would be atomic.
		g_u8LDV1000Status = read_ldv1000i();

		// if diagnostics mode is enabled, add some verbose logging
		if (IsDiagnosticsEnabledEeprom())
		{
			// print a status every time it changes
			if (g_u8LDV1000Status != u8LastStatus)
			{
				char sdbg[6];	// keep this as small as possible
				sprintf(sdbg, "-> %x", g_u8LDV1000Status);
				LOG(sdbg);
				u8LastStatus = g_u8LDV1000Status;
			}
		}

		g_u8LDV1000Stage = STAGE_WAITING_FOR_VSYNC;

		// re-enable vsync interrupt and wait for the next one
		ENABLE_VSYNC_INT();
	}

done:
	DISABLE_VSYNC_INT();
	DISABLE_CTC_INT();	// just to be safe

	// stop codes automatically will be honored again when the player type changes
}
