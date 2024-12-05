#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>	// for strcat
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/vip9500sg-interpreter.h>
#include "vip9500sg-callbacks.h"
#include "strings.h"
#include "serial2.h"
#include "protocol.h"	// for LOG
#include "settings.h"	// to check diagnostics
#include "idle.h"
#include "common-ldp.h"
#include "vip9500sg-main.h"

// how many consecutive frame requests we will log before we suppress to avoid spamming
#define MAX_LOGGABLE_FRAME_REQUESTS 2

// so we can avoid spamming the log with too many of these and overflowing our buffer
uint16_t g_u16VIP9500SGConsecutiveFrameRequests = 0;

/////////////////////////////////////////////////////////////////

#ifndef DEBUG
inline
#endif
void VIP9500SGCheckRxDiagnostics(uint8_t u8)
{
	// if diagnostics mode is enabled, add some verbose logging
	if (IsDiagnosticsEnabledEeprom())
	{
		// if this is a frame request
		if (u8 == 0x6B)
		{
			g_u16VIP9500SGConsecutiveFrameRequests++;
		}
		// else it's not a frame request, so it's okay to log it
		else
		{
			g_u16VIP9500SGConsecutiveFrameRequests = 0;
		}

		// if we haven't had too many consecutive frame requests, it's okay to log
		if (g_u16VIP9500SGConsecutiveFrameRequests <= MAX_LOGGABLE_FRAME_REQUESTS)
		{
			MediaServerSendRxLog(u8);
		}
	}
}

#ifndef DEBUG
inline
#endif
void VIP9500SGCheckTxDiagnostics(uint8_t u8)
{
	// if diagnostics mode is enabled, add some verbose logging
	if (IsDiagnosticsEnabledEeprom())
	{
		// if we haven't had too many consecutive frame requests
		if (g_u16VIP9500SGConsecutiveFrameRequests <= MAX_LOGGABLE_FRAME_REQUESTS)
		{
			MediaServerSendTxLog(u8);
		}
		// else don't log it as it to avoid spam
	}
}

void vip9500sg_main_loop()
{
	uint8_t u8 = 0;
	uint8_t u8NextField = 0;

	// Setup 16-bit timer
	// (other drivers may change this so we must set it here)
	TCCR1B = (1 << CS10) | (1 << WGM12);	// no prescaling (search datasheet for TCCR1B for details), CTC mode enabled

	// Enable RS232 in DCE mode (no null modem adapter)
	// This also enables hardware flow control
	ENABLE_DCE();

	// opto relay must be disabled when in RS232 mode
	DISABLE_OPTO_RELAY();

	vip9500sg_setup_callbacks();

	// setup UART1
	serial2_init(9600);

	{
		char s1[20];
		char s[30];
		string_to_buf(s, STRING_VIP9500SG);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
		vip9500sgi_reset();
	}

	// TODO: does the VIP9500SG auto-play?
//	ldpc_play();

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// wait for vsync to start
		while (GOT_VSYNC() == 0)
		{
			// if we have incoming serial byte
			if (rx2_is_char_waiting())
			{
				u8 = rx2_from_buf();
				vip9500sgi_write(u8);

				VIP9500SGCheckRxDiagnostics(u8);
			}

			// if we can send
			if (vip9500sgi_can_read())
			{
				u8 = vip9500sgi_read();
				tx2_to_buf(u8);
				VIP9500SGCheckTxDiagnostics(u8);
			}

			// use idle time to process low-priority tasks
			idle_think();

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
		vip9500sgi_think_after_vblank();
	}

done:
	// clean-up
			
	serial2_shutdown();

	// serial mode should be disabled when not in use because that what other drivers will expect
	DISABLE_DCE_AND_DTE();
}
