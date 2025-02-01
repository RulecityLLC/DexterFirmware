#include <ldp-abst/ldpc.h>	// for ldpc_get_status
#include "ld700-callbacks.h"	// to insert/eject the disc
#include "settings.h"	// to get active disc id
#include "dexter_strings.h"	// for log_string
#include "timer-global.h"	// for button debouncing
#include "ld700-main-deps.h"
#include "common-ldp.h"	// for vsync counter
#include "disc_switch.h"

/////////////////////////////////////////////////////////////////

// intentionally made not static so that unit tests can modify this value
uint8_t g_ld700_bDiscSwitchIsActive = 0;

// the disc side we will switch to if user holds down button
// 0 = user has not made a choice
// 1 = side 1
// 2 = side 2
static uint8_t g_u8LD700CandidateSide = 0;

void ld700_deps_reset()
{
	g_ld700_bDiscSwitchIsActive = 0;
	g_u8LD700CandidateSide = 0;
}

void ld700_idle_think()
{
	if (g_ld700_bDiscSwitchIsActive)
	{
		DiscSwitchStatus_t status = disc_switch_get_status();

		// if disc switching is done
		if (status != DISC_SWITCH_ACTIVE)
		{
            disc_switch_end();	// we need to call this once we've observed that disc switching is ended
			g_ld700_bDiscSwitchIsActive = 0;

			// media server's log message is a little obscure, add our own log to remove all doubt
			if (status == DISC_SWITCH_ERROR)
			{
				log_string(STRING_DISCSWITCH_FAILED);
			}
			else
			{
				// now that the disc switch is complete, we can safely close the tray
				ld700_close_tray();
			}
		}
	}
}

uint8_t GetLD700CandidateSide()
{
	uint8_t u8Side = g_u8LD700CandidateSide;

	// if user has not yet made a choice, then just show the disc we're currently on
	if (u8Side == 0)
	{
		u8Side = GetDiscSideByDiscId(GetActiveDiscIdMemory());
	}
	return u8Side;
}

void OnFlipDiscPressed(LD700Status_t status)
{
	uint8_t u8Side = GetLD700CandidateSide();

	// we only respond to button press if the disc is already ejected.  We want the user to be more intentional about whether they are wanting to manually eject the disc.
	if (status != LD700_TRAY_EJECTED)
	{
		return;
	}

	// u8Side will be 0, 1 or 2
	if (u8Side == 1)
	{
		g_u8LD700CandidateSide = 2;
	}
	// if 0 or 2, then we'll just change to 1 since 0 is an error state so the user shouldn't expect correct operation
	else
	{
		g_u8LD700CandidateSide = 1;
	}
}

void OnFlipDiscHeld(LD700Status_t status)
{
	// if they hold the button down while the disc is inserted, then we want to eject the tray
	if (status != LD700_TRAY_EJECTED)
	{
		ld700_eject();
	}
	// else if they hold the button down while the tray is ejected, then we want to insert the disc (and switch to it because they may have changed disc sides)
	else
	{
		uint8_t u8NewDiscId = GetTargetDiscIdByCurDiscIdAndTargetSide(GetActiveDiscIdMemory(), GetLD700CandidateSide());
		
		disc_switch_initiate(u8NewDiscId);	// if we're already on the desired side, this won't cause any harm.  So we don't need to spend extra logic checking to see if it needs to be called.
		g_ld700_bDiscSwitchIsActive = 1;	// we need to call disc_switch_end() once the switch is complete
		
		// we'll wait to close the tray until the disc switch has finished to avoid HAL trying to control the LDP before the LDP is ready
	}
}

uint8_t GetDiscSideByDiscId(uint8_t u8DiscId)
{
    uint8_t u8Result = 0;

    switch (u8DiscId)
    {
    case 61:	// thayer's quest halcyon side 1
    case 43:	// NFL football halcyon side 1
    	u8Result = 1;
        break;
    case 62:	// thayer's quest halcyon side 2
    case 44:	// NFL football halcyon side 2
    	u8Result = 2;
        break;
    default:	// unknown disc id
    	break;
    }

    return u8Result;
}

uint8_t GetTargetDiscIdByCurDiscIdAndTargetSide(uint8_t u8DiscIdCur, uint8_t u8SideTarget)
{
    uint8_t u8Result;	// no need to initialize (value will always be set)

    switch (u8DiscIdCur)
    {
    case 61:	// thayer's quest halcyon side 1
    case 62:	// thayer's quest halcyon side 2
    	if (u8SideTarget == 2)
		{
			u8Result = 62;
		}
		else
		{
			u8Result = 61;
		}
        break;
    case 43:	// NFL football halcyon side 1
    case 44:	// NFL football halcyon side 2
    	if (u8SideTarget == 2)
		{
			u8Result = 44;
		}
		else
		{
			u8Result = 43;
		}
        break;
    default:	// unknown disc id
		u8Result = u8DiscIdCur;	// just return the same disc id since we don't really have a way to flag an error right now	
    	break;
    }

    return u8Result;
}
