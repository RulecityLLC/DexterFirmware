#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/ldp1000-interpreter.h>
#include "ldp1000_callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "dexter_strings.h"
#include "serial2.h"
#include "idle.h"
#include "vsync.h"
#include "util.h"
#include "led_driver.h"
#include "timer-global.h"
#include "ldp1000-common.h"
#include "ldp1000-main.h"
#include "common-ldp.h"	// for video callback definition
#include "disc_switch.h"	// for disc switching

uint8_t g_u8LDP1000VsyncActive = 0;	// whether vsync is active or not
uint8_t g_u8LDP1000CurField = 0;

// how many consecutive frame requests we will log before we suppress to avoid spamming
#define MAX_LOGGABLE_FRAME_REQUESTS 2

// so we can avoid spamming the log with too many of these and overflowing our buffer
uint16_t g_u16LDP1000ConsecutiveFrameRequests = 0;

// to be sent after our latency requirements are satisfied
uint8_t g_u8LDP1000CachedRxByte = 0;
uint8_t g_u8LDP1000RxByteIsCached = 0;
uint8_t g_u8LDP1000LatencyEnabled = 0;

/////////////////////////////////////////////////////////////////

#ifndef DEBUG
inline
#endif
void CheckRxDiagnostics(uint8_t u8)
{
	// if diagnostics mode is enabled, add some verbose logging
	if (IsDiagnosticsEnabledEeprom())
	{
		// if this is a frame request
		if (u8 == 0x60)
		{
			g_u16LDP1000ConsecutiveFrameRequests++;
		}
		// else it's not a frame request, so it's okay to log it
		else
		{
			g_u16LDP1000ConsecutiveFrameRequests = 0;
		}

		// if we haven't had too many consecutive frame requests, it's okay to log
		if (g_u16LDP1000ConsecutiveFrameRequests <= MAX_LOGGABLE_FRAME_REQUESTS)
		{
			MediaServerSendRxLog(u8);
		}
	}
}

#ifndef DEBUG
inline
#endif
void CheckTxDiagnostics(uint8_t u8)
{
	// if diagnostics mode is enabled, add some verbose logging
	if (IsDiagnosticsEnabledEeprom())
	{
		// if we haven't had too many consecutive frame requests
		if (g_u16LDP1000ConsecutiveFrameRequests <= MAX_LOGGABLE_FRAME_REQUESTS)
		{
			MediaServerSendTxLog(u8);
		}
		// else if we are not sending back a frame number, then it's still ok to log it
		else if ((u8 & 0x30) != 0x30)
		{
			MediaServerSendTxLog(u8);
		}
		// else don't log it as it is a frame number
	}
}

void ldp1000_main_loop(LDPType ldptype)
{
	uint8_t u8TextOverlayCounter = 0;
	uint8_t u8 = 0;
	uint16_t u16 = 0;
	uint16_t u16Baud = 9600;

	// before we start taking vsync interrupts, check to see if this is an ALG multirom unit trying to switch us to the correct disc
	// (this also lets us know when that loop is finished because the baud rate message will display)
	alg_multirom_check();

	// Setup 16-bit timer
	// (other drivers may change this so we must set it here)
	TCCR1B = (1 << CS10) | (1 << WGM12);	// no prescaling (search datasheet for TCCR1B for details), CTC mode enabled

	// Enable RS232 in DTE mode (that's the mode Sony players operates in)
	// This also enables hardware flow control
	ENABLE_DTE();

	// opto relay must be disabled when in RS232 mode
	DISABLE_OPTO_RELAY();

	ldp1000_setup_callbacks();

	if (Is4800BaudEnabledEeprom())
	{
		u16Baud = 4800;
	}

	// AFAIK, the only LDP1000A games are the Data East games and they run at 1200 baud.  So it should be safe to force 1200 baud.
	else if (ldptype == LDP_LDP1000A)
	{
		u16Baud = 1200;
	}

	// setup UART1
	serial2_init(u16Baud);

	// log baud rate to help troubleshoot
	{
		char s[35];	// make this as small as possible
		char s1[30];
		string_to_buf(s1, STRING_BAUD_RATE);
		sprintf(s, s1, u16Baud);
		LOG(s);
	}

	// if this is the LDP-1000A
	if (ldptype == LDP_LDP1000A)
	{
		char s1[20];
		char s[30];	// count this out to make sure it won't overflow (I already did)
		string_to_buf(s, STRING_LDP1000A);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
		ldp1000i_reset(LDP1000_EMU_LDP1000A);
	}
	// else it's the LDP-1450
	else
	{
		char s1[20];
		char s[30];
		string_to_buf(s, STRING_LDP1450);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
		ldp1000i_reset(LDP1000_EMU_LDP1450);
	}

	set_vsync_isr_callback(ldp1000_vsync_callback);
	ENABLE_VSYNC_INT();

	// if disc is already active, we don't want to hijack it
	if (ldpc_get_status() == LDPC_STOPPED)
	{
		// The LDP-1450 automatically plays on power-up (I assume the 1000A does also)
		// However, if serial cable is connected, it will freeze on the first available frame.
		// DL2 and Time Traveler's boot-up sequence will expect this freeze-frame behavior.
		ldpc_begin_search(0);
		ldpc_end_search();
	}

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// wait for vsync to start
		while (!g_u8LDP1000VsyncActive)
		{
			// if we have incoming serial byte
			if (rx2_is_char_waiting())
			{
				u8 = rx2_from_buf();
				ldp1000i_write(u8);

				CheckRxDiagnostics(u8);
			}

			// if it's okay to send a byte (if latency is enabled, it means we are waiting for latency timer to expire)
			if (!g_u8LDP1000LatencyEnabled)
			{
				// if we have a byte we've been waiting to send due to latency
				if (g_u8LDP1000RxByteIsCached)
				{
					// tx data is buffered, so we don't need to block
					tx2_to_buf(g_u8LDP1000CachedRxByte);
					CheckTxDiagnostics(g_u8LDP1000CachedRxByte);
					g_u8LDP1000RxByteIsCached = 0;
				}
				// else if we have a new byte we can send
				else if (ldp1000i_can_read())
				{
					uint16_t u16CyclesToWait = 0;
					uint8_t u8LatencyIdx = 0;
					u16 = ldp1000i_read();
					u8LatencyIdx = u16 >> 8;
					u8 = u16 & 0xFF;

					switch (u8LatencyIdx)
					{
					// virtually no latency at all (just the baud rate)
					case LDP1000_LATENCY_INQUIRY:
						// no cycles to wait
						break;

					// the rest of these are all pretty much the same on the LDP-1450, so until we support LDP-1000A fully we will ignore this
					default:
						u16CyclesToWait = CYCLES_FOR_GENERIC;
						break;
					}

					g_u8LDP1000RxByteIsCached = 1;
					g_u8LDP1000CachedRxByte = u8;					

					if (u16CyclesToWait > 0)
					{
						OCR1A = u16CyclesToWait;
						TCNT1 = 0;	// this must be set after OCR1A is set because otherwise it is in an unknown state
						TIFR1 = (1 << OCF1A); // clear the CTC flag (writing a logic one to the set flag clears it)
						g_u8LDP1000LatencyEnabled = 1;
					}
				}

				// else no work to do
			}
			// else check to see if latency timer has expired
			else
			{
				if (TIFR1 & (1 << OCF1A))
				{
					g_u8LDP1000LatencyEnabled = 0;
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

		// g_u8CurField is set by vsync ISR
		ldpc_OnVBlankChanged(
			LDPC_TRUE,	// vblank _is_ active
			g_u8LDP1000CurField);

		// We don't really care when vsync ends, so we just pretend that it has ended immediately
		g_u8LDP1000VsyncActive = 0;

		ldp1000i_think_during_vblank();

		ldpc_OnVBlankChanged(
			LDPC_FALSE,	// vblank is no longer active (or at least, this is a good place to end it)
			g_u8LDP1000CurField);

		// generic video work
		on_video_field();

		u8TextOverlayCounter++;
		if (u8TextOverlayCounter & 0x4)
		{
			u8TextOverlayCounter = 0;

			// if text overlay has changed, update it before we display a new field to ensure that it is displayed on the media server ASAP
			// (NOTE: if we do this every field, we will overflow the TX buffer, so we throttle it back)
			if (g_bLdp1000_text_needs_update)
			{
				ldp1000_send_text_overlay_packet();
			}
		}

		// re-enable vsync interrupt and wait for the next one
		ENABLE_VSYNC_INT();
	}

done:
	// clean-up

	DISABLE_VSYNC_INT();

	serial2_shutdown();

	// serial mode should be disabled when not in use because that what other drivers will expect
	DISABLE_DCE_AND_DTE();
}

void alg_multirom_check()
{
	uint8_t u8, u8DiscId;

	// enable input mode for PB6 (centronics pin 17)
	DDRB &= ~(1 << PB6);
	PORTB |= (1 << PB6);	// enable pull-up

	// set pull-ups on relevant ports
	DATA_CONTROL = 0;	// go into input mode on all of PORTA
	DATA_PORT_WRITE = 0xff;	// enable all pull-ups so we can detect grounded lines

	// read PB6.. if it's high, that means nothing is connected and forcing it low
	if ((PINB & (1 << PB6)) != 0)
	{
		return;
	}

	// to make troubleshooting easier
	log_string(STRING_ALGMULTIROM_DETECTED);

	// if we get this far, we interpret that to mean an ALG multirom device is plugged in, telling us which laserdisc image to load
	u8 = PINA;

	switch (u8)
	{
	case 0x7f:	// mad dog
		u8DiscId = 39;
		break;
	case 0xf7:	// mad dog 2
		u8DiscId = 38;
		break;
	case 0xbf:	// johnny rock
		u8DiscId = 71;
		break;
	case 0xfb:	// gallaghers
		u8DiscId = 30;
		break;
	case 0xdf:	// space pirates
		u8DiscId = 55;
		break;
	case 0xfd:	// crime patrol
		u8DiscId = 12;
		break;
	case 0xef:	// crime patrol 2
		u8DiscId = 13;
		break;
	case 0xfe:	// last bounty hunter
		u8DiscId = 7;
		break;
	default:
		// input invalid, or there is nothing plugged in, so do nothing
		{
			char s[25], s1[25];
			string_to_buf(s, STRING_DISCSWITCH_UNKNOWN);
			sprintf(s1, s, u8);
			LOG(s1);
		}
		return;
	}

	// If we are already using this disc image, the media server may change our settings,
	//   so always do an explicit disc switch just in case.

	// we don't need to log what we are about to do because media server will log it for us

	// initiate disc switch
	disc_switch_initiate(u8DiscId);

	// we don't want to loop if user manually intervenes
	while (!g_bRestartPlayer)
	{
		DiscSwitchStatus_t status;

		// wait until disc switch is complete
		idle_think();

		// check to see if disc switch is finished
		status = disc_switch_get_status();

		// if disc switching is done
		if (status != DISC_SWITCH_ACTIVE)
		{
            disc_switch_end();	// we need to call this once we've observed that disc switching is ended

			// media server's log message is a little obscure, add our own log to remove all doubt
			if (status == DISC_SWITCH_ERROR)
			{
				log_string(STRING_DISCSWITCH_FAILED);
			}
			break;
		}
	}
}
