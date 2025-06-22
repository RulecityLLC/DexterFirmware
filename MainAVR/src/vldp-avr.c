#include <stdio.h>
#include <string.h>	// for strcat
#include <avr/io.h> 
#include <avr/interrupt.h>
#include "autodetect.h"
#include "serial.h"
#include "idle.h"
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/ldv1000-interpreter.h>
#include "ldv1000_callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "dexter_strings.h"
#include "led_driver.h"
#include "buttons.h"
#include "ldv1000-main.h"
#include "ldv1000super-main.h"
#include "ldp_stub.h"
#include "vp931-main.h"
#include "vp932-main.h"
#include "ldp1000-main.h"
#include "pr8210-main.h"
#include "pr7820-main.h"
#include "vip9500sg-main.h"
#include "ld700-main.h"
#include "timer-global.h"

// whether we need to restart the active laserdisc player
volatile unsigned char g_bRestartPlayer = 0;

// whether media server has told us to go into the bootloader and reprogram ourselves
volatile uint8_t g_bReprogramForced = 0;

///////////////////////////////////////////////////////////

int main (void) 
{
	// no telling what might rely on this, so initialize it first
	timer_global_init();

	// put dce/dte pins in output mode
	SETUP_DCE_AND_DTE();

	// disable dce/dte for safety reasons
	DISABLE_DCE_AND_DTE();

	// put opto relay pin in output mode
	SETUP_OPTO_RELAY();

	// disable opto relay for safety reasons
	DISABLE_OPTO_RELAY();

	// for safety reasons, PR8210A mode should be disabled by default as it will conflict with other modes such as VP931 mode
	DISABLE_PR8210A_MODE();

	// setup interrupt to occur every vsync
	SETUP_VSYNC_INT();

	DATA_INIT();

#ifdef REV1
	// setup LED to blink
	LED_SETUP();
	LED_POWER_PIN_ACTIVE();
#endif // REV1

	///////////////////

	buttons_init();	// setup push buttons

	leds_init();	// setup led driver

	serial_init();

	SETUP_SVIDEO_MUTE();	// if s-video is supported (rev2 only)

	// make sure VP931 interrupts are disabled by default
	DISABLE_INT_WREN();
	DISABLE_INT_RDEN();

	// enable interrupts so we can start receiving serial data
	sei();

	///////////////////

	// give a nice creative "hello" message :)
	log_string(STRING_AM_HERE);

	// load in our settings from eeprom and use those to start off with
	load_settings_from_eeprom();

	// tell media server that we've just started up so it can check our settings
	MediaServerSendHello();

	// wait for this data to be transmitted so it doesn't get lost (our tx buf is pretty small)
	while (tx_buf_is_not_empty())
	{
		idle_think();	// this sometimes is needed to empty TX buffer
	}

	// this is the outer loop that can occasionally be revisited by the media server sending us new settings
	for (;;)
	{
		LDPType ldpType = GetManualLDPTypeEeprom();
		LDPType ldpTypeAutodetected;	// will be set later
		LDPType ldpTypeOld;	//  used later

		if (IsAutoDetectionEnabledEeprom())
		{
			// at this point, we know that auto-detection is enabled
			AutoDetectStrategy strat = GetAutoDetectStrategyEeprom();

			// if there is no auto-detection strategy, then the user may have requested auto-detection when we are unable to fulfill that request,
			//  so just set the auto-detected player to none and log a message in case they are watching the log window.
			if (strat == AUTODETECT_NONE)
			{
				log_string(STRING_AUTODETECTION_UNAVAILABLE);
				SetAutodetectedLDPType(LDP_NONE);
			}
			// else if the type is less than 0x80, it means only one laserdisc player can be selected, so select it
			else if ((strat & 0x80) == 0)
			{
				log_string(STRING_AUTODETECTED);	// so we know that auto detection took place
				SetAutodetectedLDPType((LDPType) strat);
			}
			// else employ auto-detection strategy
			else
			{
				switch (strat)
				{
				default:	// unexpected scenario, we don't know how to handle it
					log_string(STRING_ERROR);	// give some indication that something went wrong
					SetAutodetectedLDPType(LDP_LDV1000);	// a default
					break;
				case AUTODETECT_LDV1000_OR_PR7820:
					{
						char s[30];
						char s1[10];

						ldpTypeAutodetected = detect_ldv1000_or_pr7820();
						SetAutodetectedLDPType(ldpTypeAutodetected);
						
						string_to_buf(s, STRING_AUTODETECTED);
						string_to_buf(s1, (ldpTypeAutodetected == LDP_LDV1000) ? STRING_LDV1000 : STRING_PR7820);
						strcat(s, s1);
						LOG(s);
					}
					break;
				}
			}

			ldpTypeAutodetected = GetAutodetectedLDPType();

			// if we have successfully auto-detected a type, use that as our main type
			if (ldpTypeAutodetected != LDP_NONE)
			{
				ldpType = ldpTypeAutodetected;
			}
			// else leave it unchanged since our autodetection failed and we would have already logged an error message
		}
		// else if the player type is not explicitly selected, this is a bug
		else if (ldpType == LDP_NONE)
		{
			ldpType = LDP_LDV1000;	// select some default
			log_string(STRING_ERROR);	// this is a bug so we'll log it, but we will keep going
		}
		// else do nothing special

		// if we were previously using one of these types, Dexter may be using an adapter plugged into the DB25 port that has hardware to help us verify that the claimed player type is actually the one being used.
		// This is a safety feature so that we don't drive the wrong pins and cause hardware conflicts.
		// So we double-check that the type that we were using before is still current.
		if ((ldpType == LDP_LDP1000A) || (ldpType == LDP_LD700))
		{
			ldpType = LDP_OTHER;	// this will force an auto-detection of the 'other' mode
		}

		// At this point, if the ldp type is 'other' then we need to do further auto-detection to figure out the actual player type so that we can start that player type's code.
		if (ldpType == LDP_OTHER)
		{
			ldpType = detect_other_mode();
		}

		// to detect whether we are about to change the ldp type
		ldpTypeOld = GetActiveLDPType();

		// this is the type we will launch
		SetActiveLDPType(ldpType);

		// if active LDP type has now changed, we need to notify media server according to our contract
		if (ldpTypeOld != ldpType)
		{
			MediaServerSendSettings();
		}

		// Set this to 0 so that inner-LDP loops don't exit immediately
		g_bRestartPlayer = 0;

		// prepare to enable the current LEDs
		// Coupling warning : the led_blinker relies on this behavior (all LEDs being cleared when the mode changes).  If you change it... be sure to update the blinker.
		leds_clear_all();

		// set LDP LED
		leds_change(LDPTypeToLED(ldpType), 1);

		// set diagnose LED
		update_diagnose_led();

		// stop codes honored by default
		SetHonorStopCodesMemory(1);

		// emulate the LDP type specified by our settings
		switch (ldpType)
		{
		default:
			ldp_stub_main_loop();
			break;
		case LDP_LDV1000:
		case LDP_LDV1000_BL:
			// if super mode is enabled
			if (IsSuperModeEnabledMemory())
			{
				ldv1000super_main_loop();
			}
			// else start off in classic mode
			else
			{
				ldv1000_main_loop(ldpType);
			}
			break;
		case LDP_VP931:
			vp931_main_loop();
			break;
		case LDP_VP932:
			vp932_main_loop();
			break;
		case LDP_LDP1000A:
		case LDP_LDP1450:
			ldp1000_main_loop(ldpType);
			break;
		case LDP_PR7820:
			// if super mode is enabled
			if (IsSuperModeEnabledMemory())
			{
				ldv1000super_main_loop();
			}
			// else start off in classic mode
			else
			{
				pr7820_main_loop();
			}
			break;
		case LDP_PR8210:
		case LDP_PR8210A:
			pr8210_main_loop(ldpType);
			break;
		case LDP_VIP9500SG:
			vip9500sg_main_loop();
			break;
		case LDP_LD700:
			ld700_main_loop();
			break;
		}

		// if we need to reprogram ourselves, do so now
		if (g_bReprogramForced)
		{
			ForceReprogram();
		}
	}
}

//////////////////////////////

