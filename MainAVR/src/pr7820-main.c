#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/pr7820-interpreter.h>
#include <ldp-abst/ldp_search_delay.h>
#include "pr7820-callbacks.h"
#include "protocol.h"
#include "settings.h"
#include "dexter_strings.h"
#include "idle.h"
#include "vsync.h"
#include "led_driver.h"
#include "common.h"
#include "common-ldp.h"

/////////////////////////////////////////////////////////////////

void pr7820_main_loop()
{
	uint8_t u8CurField = 0;
	uint8_t u8 = 0;

	// make sure RS232 is disabled
	DISABLE_DCE_AND_DTE();

	// for safety reasons, opto relay should be disabled when DB25 port is not in use
	DISABLE_OPTO_RELAY();

	// we only read from the data bus.  Enable pull-ups so that the logic analyzer dumps are easier to read.
	DATA_PORT_WRITE=0xFF;
	DATA_ENABLE_READ();

	// setup PR7820 pins
	SETUP_PR7820();

	// pin 12 needs to be ground-friendly, this applies to ld-v1000/pr-7820 modes as well
	SETUP_LDV1000_GND();

	// make us unready while we are initializing
	PR7820_RAISE_READY_PRIME();

	pr7820_setup_callbacks();
	pr7820i_reset();

	{
		char s1[20];
		char s[30];	// count this out to make sure it won't overflow (I already did)
		string_to_buf(s, STRING_PR7820);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
	}

	// ignore stop codes
	SetHonorStopCodesMemory(0);

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// while we haven't got the next vsync, listen for commands and process idle tasks
		while (GOT_VSYNC() == 0)
		{
			// if the ENTER' line has changed
			if ((PCIFR & (1 << PCIF2)) != 0)
			{
				// reset the flag so that we can detect the next ENTER' transition
				PCIFR |= (1 << PCIF2);

				// has ENTER' gone low?
				if (PR7820_ENTER_PRIME() == 0)
				{
					// is INT/EXT' line in external mode? (this check probably isn't necessary, I am just putting it here for completeness)
					if (PR7820_INT_EXT_PRIME() == 0)
					{
						// read the bus and process the data
						u8 = DATA_PORT_READ;

						// if diagnostics mode is enabled, log the incoming command
						if (IsDiagnosticsEnabledEeprom())
						{
							MediaServerSendTxLog(u8);
						}

						pr7820i_write(u8);
					}
				}
				// else ENTER' has gone high, we don't care about this
			}

			idle_think();

 			// if user presses MODE button then abort (this is needed in case we have no video signal plugged in)
			if (g_bRestartPlayer)
			{
				goto done;
			}
		}

		// so we can detect when the next one comes
		CLEAR_VSYNC_FLAG();

		// pr-7820 is so basic that there is no point in tracking the actual field,
		//  and having a fake field makes debugging easier
		u8CurField ^= 1;

		ldpc_OnVBlankChanged(
			LDPC_TRUE,
			u8CurField);

		ldpc_OnVBlankChanged(
			LDPC_FALSE,
			u8CurField);

		// generic video work
		on_video_field();

		// update READY' line
		u8 = pr7820i_is_busy();
		if (u8)
		{
			PR7820_RAISE_READY_PRIME();
		}
		else
		{
			PR7820_LOWER_READY_PRIME();
		}

	}

done:

	// stop codes automatically will be honored again when the player type changes

	return;
}
