#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/vp932-interpreter.h>
#include "vp932-callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "strings.h"
#include "serial2.h"
#include "idle.h"
#include "vsync.h"
#include "util.h"
#include "led_driver.h"
#include "timer-global.h"
#include "vp932-main.h"
#include "common-ldp.h"	// for video callback definition

uint8_t g_bVP932VsyncActive = 0;	// whether vsync is active or not

/////////////////////////////////////////////////////////////////

VP932Status_t vp932_convert_status(LDPCStatus_t status)
{
	VP932Status_t res = VP932_ERROR;

	switch (status)
	{
	case LDPC_SEARCHING:
		res = VP932_SEARCHING;
		break;
	case LDPC_PLAYING:
		res = VP932_PLAYING;
		break;
	case LDPC_PAUSED:
		res = VP932_PAUSED;
		break;
	case LDPC_SPINNING_UP:
		res = VP932_SPINNING_UP;
		break;
	case LDPC_STOPPED:
	default:
		res = VP932_ERROR;
		break;
	}
	return res;
}

void vp932_main_loop()
{
	uint8_t u8 = 0;
	uint8_t u8VP932CurField = 0;

	SETUP_VP932();

	// Enable RS232 in DCE mode (no null modem adapter)
	// This also enables hardware flow control.
	// The Euro DL PCB adapter I made is designed for Dexter to run in DCE mode.
	ENABLE_DCE();

	// opto relay must be disabled when in RS232 mode
	DISABLE_OPTO_RELAY();

	// indicate that we are ready to receive serial data
	VP932_LOWER_RTS();

	vp932_setup_callbacks();

	// setup UART1, the only game we know of (DLEuro) runs at 9600 baud, so don't need to configure it
	serial2_init(9600);

	{
		char s1[20];
		char s[30];	// count this out to make sure it won't overflow (I already did)
		string_to_buf(s, STRING_VP932);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
	}

	vp932i_reset();

	set_vsync_isr_callback(vp932_vsync_callback);
	ENABLE_VSYNC_INT();

	// we don't know whether this player autoplays, but the vp931 does, so assume it does also
	ldpc_play(LDPC_FORWARD);

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// wait for vsync to start
		while (!g_bVP932VsyncActive)
		{
			// if we have incoming serial byte
			if (rx2_is_char_waiting())
			{
				u8 = rx2_from_buf();

				vp932i_write(u8);

				// if diagnostics mode is enabled, log the incoming command
				if (IsDiagnosticsEnabledEeprom())
				{
					MediaServerSendRxLog(u8);
				}
			}

			// if we have outgoing serial data
			if (vp932i_can_read())
			{
				// if game is currently accepting serial data
				// (it might lose our data if we transmit without permission)
				if (VP932_DSR() == 0)
				{
					// send it!
					u8 = vp932i_read();

					// tx data is buffered, so we don't need to block
					tx2_to_buf(u8);

					// if diagnostics mode is enabled, log the outgoing command
					if (IsDiagnosticsEnabledEeprom())
					{
						MediaServerSendTxLog(u8);
					}

				}
			}

			// use idle time to process low-priority tasks
			idle_think();

 			// if user presses MODE button then abort (this is needed in case we have no video signal plugged in)
			if (g_bRestartPlayer)
			{
				goto done;
			}
		}

		// The LM1881 uses 0 for even/bottom and 1 for top/odd (backward from our convention) so we need to flip it.
		u8VP932CurField = FIELD_PIN ^ 1;

		ldpc_OnVBlankChanged(
			LDPC_TRUE,	// vblank _is_ active
			u8VP932CurField);

		// We don't really care when vsync ends, so we just pretend that it has ended immediately
		g_bVP932VsyncActive = 0;

		vp932i_think_during_vblank(vp932_convert_status(ldpc_get_status()));

		ldpc_OnVBlankChanged(
			LDPC_FALSE,	// vblank is no longer active (or at least, this is a good place to end it)
			u8VP932CurField);

		// generic video work
		on_video_field();

		// having RTS raised indicates player is busy
		{
			LDPCStatus_t stat = ldpc_get_status();

			if ((stat == LDPC_SEARCHING) || (stat == LDPC_SPINNING_UP))
			{
				VP932_RAISE_RTS();
			}
			else
			{
				VP932_LOWER_RTS();
			}
		}

		// re-enable vsync interrupt and wait for the next one
		ENABLE_VSYNC_INT();
	}

done:
	// clean-up

	DISABLE_VSYNC_INT();

	serial2_shutdown();
}
