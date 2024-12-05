#include "protocol.h"
#include "ldv1000_callbacks.h"
#include "strings.h"
#include "settings.h"
#include "common-ldp.h"
#include <ldp-abst/ldpc.h>
#include <stdio.h>

void ldv1000_setup_callbacks()
{
	g_ldv1000i_get_status = ldv1000_get_status;
	g_ldv1000i_get_cur_frame_num = ldv1000_get_cur_frame_num;
	g_ldv1000i_play = ldv1000_play;
	g_ldv1000i_pause = ldv1000_pause;
	g_ldv1000i_begin_search = common_ldp_begin_search;
	g_ldv1000i_step_reverse = ldv1000_step_reverse;
	g_ldv1000i_change_speed = ldv1000_change_speed;
	g_ldv1000i_skip_forward = ldv1000_skip_forward;
	g_ldv1000i_skip_backward = ldv1000_skip_backward;
	g_ldv1000i_change_audio = ldv1000_change_audio;
	g_ldv1000i_on_error = ldv1000_on_error;
	g_ldv1000i_query_available_discs = GetAvailableDiscIdsEeprom;
	g_ldv1000i_query_active_disc = GetActiveDiscIdMemory;
	g_ldv1000i_begin_changing_to_disc = protocol_initiate_disc_switch;
	g_ldv1000i_change_seek_delay = SetSearchDelayEnabledMemory;
	g_ldv1000i_change_spinup_delay = common_enable_spinup_delay;
	g_ldv1000i_change_super_mode = OnSuperModeChanged;
}

LDV1000Status_t ldv1000_get_status()
{
	LDV1000Status_t res = LDV1000_ERROR;

	ProtocolDiscSwitchStatus_t status = protocol_get_disc_switch_status_and_think();

	// if no disc switch is underway, just return the regular LD-V1000 status
	if (status == PROTOCOL_DISC_SWITCH_IDLE)
	{
		switch (ldpc_get_status())
		{
		case LDPC_SEARCHING:
			res = LDV1000_SEARCHING;
			break;
		case LDPC_STOPPED:
			res = LDV1000_STOPPED;
			break;
		case LDPC_PLAYING:
			res = LDV1000_PLAYING;
			break;
		case LDPC_PAUSED:
			res = LDV1000_PAUSED;
			break;
		case LDPC_SPINNING_UP:
			res = LDV1000_SPINNING_UP;
			break;
		default:
			res = LDV1000_ERROR;
			break;
		}
	}
	// else if a disc switch is active
	else
	{
		switch (status)
		{
		case PROTOCOL_DISC_SWITCH_ACTIVE:
			res = LDV1000_DISC_SWITCHING;
			break;
		case PROTOCOL_DISC_SWITCH_SUCCESS:
			res = LDV1000_STOPPED;
			break;
		default:	// make default the error case so we can catch defects in our code
		case PROTOCOL_DISC_SWITCH_ERROR:
			res = LDV1000_ERROR;
			break;
		}
	}

	return res;
}

uint32_t ldv1000_get_cur_frame_num()
{
	return ldpc_get_cur_frame_num();
}

void ldv1000_play()
{
	log_string(STRING_CMD_PLAY);
	ldpc_play(LDPC_FORWARD);
}

void ldv1000_pause()
{
	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void ldv1000_step_reverse()
{
	log_string(STRING_UNIMPLEMENTED);
	ldpc_pause();
}

void ldv1000_change_speed(uint8_t uNum, uint8_t uDenom)
{
	ldpc_change_speed(uNum, uDenom);
}

void ldv1000_skip_forward(uint8_t uTracks)
{
	ldpc_skip_tracks(uTracks);
}

void ldv1000_skip_backward(uint8_t uTracks)
{
	ldpc_skip_tracks(-uTracks);
}

void ldv1000_change_audio(uint8_t uChannel, uint8_t Enable)
{
	ldpc_change_audio(uChannel, Enable);
}

void ldv1000_on_error(const char *pszErrMsg)
{
	LOG_ERR(pszErrMsg);
}
