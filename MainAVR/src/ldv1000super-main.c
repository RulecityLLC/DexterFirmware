#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/ldv1000-interpreter.h>
#include "ldv1000_callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "dexter_strings.h"
#include "idle.h"
#include "vsync.h"
#include "timer1.h"
#include "led_driver.h"
#include "common-ldp.h"
#include "ldv1000super-main.h"

#define LDV1000SUPER_DO_IDLE() idle_think(); if (g_bRestartPlayer) { goto done; }

void ldv1000super_main_loop()
{
	// for verbose logging
	uint8_t u8LastStatus = 0;

	uint8_t u8CurStatus = 0;

	// which field will be next (0 = top, 1 = bottom)
	uint8_t u8NextField = 0;

	// to help command chaining be as fast as possible
	uint8_t bProcessIdleTasks = 0;

	// Setup 16-bit timer
	// (other drivers may change this so we must set it here)
	TCCR1B = (1 << CS11);	// divide cycle count by 8 (to match 8ms boundary) (search datasheet for TCCR1B for details), no CTC

	OCR1A = (F_CPU / 1000);	// we want to trigger the flag every 8 milliseconds

	// setup 8-bit timer, no CTC mode, /8 prescaling (would've chosen /2 if it was available)
	TCCR2A = 0;
	TCCR2B = (0 << CS22) | (1 << CS21) | (0 << CS20);
	OCR2A = TICKS_FOR_STATUS;

	// make sure RS232 is disabled
	DISABLE_DCE_AND_DTE();

	// for safety reasons, opto relay should be disabled when DB25 port is not in use
	DISABLE_OPTO_RELAY();

	// setup I/O
	LDV1000SUPER_SETUP_IO();

	// pin 12 needs to be ground-friendly, this applies to ld-v1000 mode as well
	SETUP_LDV1000_GND();

	// the regular ldv1000 callbacks work fine for super mode also
	ldv1000_setup_callbacks();

	{
		char s1[20];
		char s[25];
		string_to_buf(s, STRING_LDV1000SUPER);	// 18 bytes incl terminator
		string_to_buf(s1, STRING_START);	// 6 bytes incl terminator
		strcat(s, s1);
		LOG(s);
		reset_ldv1000i(LDV1000_EMU_SUPER);
	}

	// ignore stop codes (will be re-enabled when disc type changes)
	SetHonorStopCodesMemory(0);

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// resume processing idle tasks until we get a command
		bProcessIdleTasks = 1;

		// while we haven't got the next vsync, listen for commands and process idle tasks
		while (GOT_VSYNC() == 0)
		{
			// if we have an incoming command
			if (LDV1000SUPER_IS_PIN17_RAISED())
			{
				ldv1000super_process_cmd();

				// don't proess idle tasks until the next vsync to encourage super fast command chaining
				bProcessIdleTasks = 0;
			}

			// only process idle tasks if a chain of commands are not being sent through
			// (we want to process command chains as quickly as possible)
			if (bProcessIdleTasks != 0)
			{
				LDV1000SUPER_DO_IDLE();
			}
		}

		// so we can detect when the next one comes
		CLEAR_VSYNC_FLAG();

		// Use the real field value (instead of simulated) for accuracy.
		// The LM1881 uses 0 for even/bottom and 1 for top/odd (backward from our convention) but since we want the next field (not the current one),
		//   this works out.
		u8NextField = FIELD_PIN;

		// pretend that vblank is instant because it doesn't matter to us
		ldpc_OnVBlankChanged(
			LDPC_TRUE,
			u8NextField);

		ldpc_OnVBlankChanged(
			LDPC_FALSE,
			u8NextField);

		// generic video work
		on_video_field();

		// if we received a command too recently, don't send any status byte now
		if ((TIFR1 & (1 << OCF1A)) == 0)
		{
			continue;
		}

		// it's ok to send status because we haven't received a command recently
		u8CurStatus = read_ldv1000i();

		// if diagnostics mode is enabled, add some verbose logging
		if (IsDiagnosticsEnabledEeprom())
		{
			// to avoid spamming, only print status when it changes
			if (u8CurStatus != u8LastStatus)
			{
				MediaServerSendTxLog(u8CurStatus);
				u8LastStatus = u8CurStatus;
			}
		}

		// put status byte on bus
		LDV1000SUPER_DATA_DRIVE_OUTPUT(u8CurStatus);

		// lower pin 7
		LDV1000SUPER_ENABLE_PIN7();

		// wait for status period to end so game has a chance to read the byte
		TCNT2 = 0;
		TIFR2 |= (1 << OCF2A);
		while ((TIFR2 & (1 << OCF2A)) == 0)
		{
			// do nothing here so that it is quick
		}

		// stop putting data on bus
		LDV1000SUPER_DATA_GO_INPUT();

		// raise pin 7
		LDV1000SUPER_DISABLE_PIN7();

	} // end main loop
done:
	return;
}

void ldv1000super_process_cmd()
{
	uint8_t u8CmdByte = 0;

	// pre-requisite: pin 17 is raised

	// read command from bus
	u8CmdByte = LDV1000SUPER_DATA_READ();

	// if diagnostics mode is enabled, add some verbose logging
	if (IsDiagnosticsEnabledEeprom())
	{
		// TODO : delay if tx buffer is full to avoid overflowing?

		// since super mode doesn't spam 0xFF, we want to log every command for better troubleshooting
		MediaServerSendRxLog(u8CmdByte);
	}

	write_ldv1000i(u8CmdByte);

	// lower pin 7 to indicate receipt of command
	LDV1000SUPER_ENABLE_PIN7();

	// wait for pin 17 to go low
	while (LDV1000SUPER_IS_PIN17_RAISED())
	{
		// we want to make this as fast as possible, so we live dangerously and allow for potential endless loop
	}

	// raise pin 7 to indicate that we saw pin 17 go low
	LDV1000SUPER_DISABLE_PIN7();

	// reset the 8ms timer so we know when we can safely send another status
	TCNT1 = 0;
	TIFR1 = (1 << OCF1A);
}
