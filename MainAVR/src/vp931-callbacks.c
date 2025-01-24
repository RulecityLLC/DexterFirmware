#include "vp931-callbacks.h"
#include "vp931-main.h"
#include "dexter_strings.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/vp931-interpreter.h>
#include "protocol.h"
#include "settings.h"
#include "common-ldp.h"
#include "util.h"
#include <avr/io.h>

void vp931_setup_callbacks()
{
	g_vp931i_play = vp931_play;
	g_vp931i_pause = vp931_pause;
	g_vp931i_begin_search = vp931_begin_search;
	g_vp931i_skip_tracks = vp931_skip_tracks;
	g_vp931i_skip_to_framenum = vp931_skip_to_framenum;
	g_vp931i_error = vp931_error;
}

void vp931_log_think()
{
	if (IsDiagnosticsEnabledEeprom())
	{
		char s[35];	// make this as small as possible
		char s1[30];

		uint8_t u8Idx = 0;
		string_to_buf(s1, STRING_OUTGOING_VP931_STATUS);
		sprintf(s, s1, g_arrVP931StatusBytes[0], g_arrVP931StatusBytes[1], g_arrVP931StatusBytes[2],
			g_arrVP931StatusBytes[3], g_arrVP931StatusBytes[4], g_arrVP931StatusBytes[5]);
		LOG(s);

		// log all incoming commands (usually there will only be one set of 3)
		for (u8Idx = 0; (u8Idx+3) <= g_u8VP931CmdBytesReceived; u8Idx += 3)
		{
			string_to_buf(s1, STRING_INCOMING_VP931_CMD);
			sprintf(s, s1, g_arrVP931CmdBytes[u8Idx], g_arrVP931CmdBytes[u8Idx+1], g_arrVP931CmdBytes[u8Idx+2]);
			LOG(s);
		}
	}
}

void vp931_play()
{
//	log_string(STRING_CMD_PLAY);
	ldpc_play(LDPC_FORWARD);
}

void vp931_pause()
{
	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void vp931_begin_search(uint32_t uFrameNumber, VP931_BOOL bAudioSquelchedOnComplete)
{
	char s[35];	// make this as small as possible
//	char s1[30];
//	string_to_buf(s1, STRING_CMD_SEEK_TO_FRAME);
//	sprintf(s, s1, uFrameNumber);
//	LOG(s);
	if (ldpc_begin_search(uFrameNumber))
	{
		// Seeking needs to be instant because FFX uses seeks to skip.
		// If instant seeking proves problematic in the future, we can add a delay where appropriate.
		ldpc_end_search();
		
		// if audio is not supposed to be squelched on search complete, then manually unsquelch it now
		// (this is to handle skips that masquerade as searches)
		if (!bAudioSquelchedOnComplete)
		{
			ldpc_set_audio_squelched(LDPC_FALSE);
		}
	}
	else
	{
		string_to_buf(s, STRING_SEARCH_FAILED);
		LOG(s);
	}

	vp931_log_think();

}

void vp931_skip_tracks(int16_t i16TracksToSkip)
{
	ldpc_skip_tracks(i16TracksToSkip);

	// don't log firefox's spamming of -1 (pause)
	if (i16TracksToSkip != -1)
	{
		vp931_log_think();
	}
}

void vp931_skip_to_framenum(uint32_t u32FrameNum)
{
	LDPC_BOOL res = ldpc_skip_to_frame(u32FrameNum);
	vp931_log_think();

	// log any error to help troubleshoot
	if (res != LDPC_TRUE)
	{
		char s[20];
		string_to_buf(s, STRING_SEARCH_FAILED);
		LOG_ERR(s);

		// log current ldpc status for greater insight
		common_log_ldpc_status(ldpc_get_status());
	}
}

void vp931_error(VP931ErrCode_t code, uint8_t u8Val)
{
	char s[20];	// should be as short as possible

	switch (code)
	{
	default:
	case VP931_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u8Val);	// val will always be 8-bits
		break;
	case VP931_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u8Val);	// val will always be 8-bits
		break;
	case VP931_ERR_TOO_MANY_WRITES:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);	// fits for now...
		break;
	}

	LOG_ERR(s);

	vp931_log_think();
}
