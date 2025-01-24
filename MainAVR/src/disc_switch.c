#include "protocol.h"
#include "disc_switch.h"
#include "timer-global.h"
#include "common-ldp.h"
#include "dexter_strings.h"	// to log error
#include <ldp-abst/ldpc.h>

static DiscSwitchStatus_t g_disc_switch_status = DISC_SWITCH_IDLE;
static uint8_t g_disc_switch_id = 0;	// which disc id was requested for the disc switch
static uint16_t g_u16DiscSwitchSlowTimerStart = 0;

void on_disc_switch_acknowledge(uint8_t u8MediaServerDiscId, uint8_t bSuccess)
{
	// if we are in the middle of a disc switch
	if (g_disc_switch_status == DISC_SWITCH_ACTIVE)
	{
		if (u8MediaServerDiscId == g_disc_switch_id)
		{
			// if disc switch succeeded
			if (bSuccess != 0)
			{
				g_disc_switch_status = DISC_SWITCH_SUCCESS;

                                // TODO : set active disc to the newly switched disc?
			}
			// else disc switch failed
			else
			{
				g_disc_switch_status = DISC_SWITCH_ERROR;
			}
		}
		// else the disc id does not match what we are expecting, so stay in the active state (which will cause us to resend the request)
		else
		{
			// this should never happen, so log it if it does
			log_string(STRING_ERROR);
		}
	}
	// else we can safely ignore this	
}

void disc_switch_initiate(uint8_t idDisc)
{
	g_u16DiscSwitchSlowTimerStart = GET_GLOBAL_SLOW_TIMER_VALUE();
	
	// want screen to go blank as soon as disc switch starts
	MediaServerSendBlankScreen();

	// stop the disc from playing so that we don't see 'garbage' fields during the transition
	ldpc_stop();	
	g_disc_switch_status = DISC_SWITCH_ACTIVE;
	g_disc_switch_id = idDisc;
}

DiscSwitchStatus_t disc_switch_get_status()
{
	return g_disc_switch_status;
}

void disc_switch_think()
{
	if (g_disc_switch_status == DISC_SWITCH_ACTIVE)
	{
		uint16_t u16SlowTimerDiffSinceStart = GLOBAL_SLOW_TIMER_DIFF_SINCE_START(g_u16DiscSwitchSlowTimerStart);
		
		// slow timer ticks at about 70 Hz, so we'll send updates every 1/4 second (70/4) to not overwhelm our TX buffer
		if (u16SlowTimerDiffSinceStart > 18)
		{
			// While we are active, send (another) disc switch request.  these requests are idempotent, so no problem sending duplicates.
			MediaServerSendBlankScreen();	// in unlikely event that previous blank screen request was lost, keep resending it
			MediaServerSendDiscSwitch(g_disc_switch_id);		
			
			// reset timer
			g_u16DiscSwitchSlowTimerStart = GET_GLOBAL_SLOW_TIMER_VALUE();
		}
	}
	// else disc switching is not active
}

void disc_switch_end()
{
	g_disc_switch_status = DISC_SWITCH_IDLE;
}
