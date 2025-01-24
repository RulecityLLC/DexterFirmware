#include "common-ldp.h"
#include "protocol.h"
#include "pr8210-callbacks.h"
#include "dexter_strings.h"
#include "util.h"
#include "protocol.h"
#include "settings.h"
#include "rev.h"
#include <ldp-abst/ldpc.h>
#include <ldp-abst/ldp_search_delay.h>
#include <stdio.h>
#include <avr/io.h>

/////////////////////////////////////////////////

extern uint8_t g_u8PR8210AGotJumpTriggerThisField;

// so that rev3 knows the state of the stand by line
uint8_t g_bPR8210AStandByRaised = 0;

void pr8210_setup_callbacks()
{
	g_pr8210i_play = pr8210_play;
	g_pr8210i_pause = pr8210_pause;
	g_pr8210i_step = pr8210_step;
	g_pr8210i_begin_search = pr8210_begin_search;
	g_pr8210i_change_audio = pr8210_change_audio;
	g_pr8210i_skip = pr8210_skip;
	g_pr8210i_change_auto_track_jump = pr8210_change_auto_track_jump;
	g_pr8210i_is_player_busy = pr8210_is_player_busy;
	g_pr8210i_change_standby = pr8210_change_standby;
	g_pr8210i_error = pr8210_error;
}

////////////////////////////////////////

void pr8210_play()
{
	log_string(STRING_CMD_PLAY);
	ldpc_play(LDPC_FORWARD);
}

void pr8210_pause()
{
	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void pr8210_step(int8_t iTracks)
{
	log_string(STRING_CMD_STEP);

	if (iTracks > 0)
	{
		ldpc_step(LDPC_FORWARD);
	}
	else
	{
		ldpc_step(LDPC_BACKWARD);
	}
}

void pr8210_begin_search(uint32_t u32FrameNum)
{
	common_ldp_begin_search(u32FrameNum);

	// video is muted elsewhere
}

void pr8210_change_audio(uint8_t u8Channel, uint8_t uEnable)
{
	ldpc_change_audio(u8Channel, uEnable);
}

void pr8210_skip(int8_t i8TracksToSkip)
{
	g_u8PR8210AGotJumpTriggerThisField = 1;	// used to compensate for star rider defect
	ldpc_skip_tracks(i8TracksToSkip);
}

void pr8210_change_auto_track_jump(PR8210_BOOL bAutoTrackJumpEnabled)
{
// we handle this ourselves
//	ldpc_set_disable_auto_track_jump(bAutoTrackJumpEnabled ? LDPC_FALSE : LDPC_TRUE);
}

PR8210_BOOL pr8210_is_player_busy()
{
	PR8210_BOOL result = PR8210_FALSE;
	
	LDPCStatus_t status = ldpc_get_status();

	if ((status == LDPC_SEARCHING) || (status == LDPC_SPINNING_UP))
	{
		result = PR8210_TRUE;
	}

	return result;
}

void pr8210_change_standby(PR8210_BOOL bRaised)
{
	if (bRaised) 
	{
		ENABLE_PR8210A_STANDBY(); // set 8210A STAND BY high (active)
	}
	else
	{
		DISABLE_PR8210A_STANDBY(); // set 8210A STAND BY low (inactive)
	}
}

void pr8210_error(PR8210ErrCode_t code, uint16_t u16Val)
{
	char s[20];	// longest length is corrupt input, which is 20 bytes long including null termination

	switch (code)
	{
	default:
	case PR8210_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u16Val);	// val will always be 8-bits
		break;
	case PR8210_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u16Val);	// val will always be 8-bits
		break;
	case PR8210_ERR_TOO_MANY_DIGITS:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);
		break;
	case PR8210_ERR_UNHANDLED_SITUATION:
		string_to_buf(s, STRING_UNHANDLED_SITUATION);
		break;
	case PR8210_ERR_CORRUPT_INPUT:
		string_to_buf(s, STRING_CORRUPT_16BIT_INPUT);
		Uint16ToHexString(&s[15], u16Val);
		break;
	}

	LOG(s);
}
