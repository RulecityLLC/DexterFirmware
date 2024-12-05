#include "protocol.h"
#include "pr7820-callbacks.h"
#include "strings.h"
#include <ldp-abst/ldpc.h>
#include <stdio.h>
#include "common.h"	// for setting state to busy
#include "common-ldp.h"
#include "settings.h"	// for super mode
#include "util.h"	// for ByteToHexString

void pr7820_setup_callbacks()
{
	g_pr7820i_get_status = pr7820_get_status;
	g_pr7820i_play = pr7820_play;
	g_pr7820i_pause = pr7820_pause;
	g_pr7820i_begin_search = pr7820_begin_search;
	g_pr7820i_change_audio = pr7820_change_audio;
	g_pr7820i_enable_super_mode = pr7820_enable_super_mode;
	g_pr7820i_on_error = pr7820_on_error;
}

PR7820Status_t pr7820_get_status()
{
	PR7820Status_t res = PR7820_ERROR;

	switch (ldpc_get_status())
	{
	case LDPC_SEARCHING:
		res = PR7820_SEARCHING;
		break;
	case LDPC_STOPPED:
		res = PR7820_STOPPED;
		break;
	case LDPC_PLAYING:
		res = PR7820_PLAYING;
		break;
	case LDPC_PAUSED:
		res = PR7820_PAUSED;
		break;
	case LDPC_SPINNING_UP:
		res = PR7820_SPINNING_UP;
		break;
	default:
		res = PR7820_ERROR;
		break;
	}
	return res;
}

void pr7820_play()
{
	log_string(STRING_CMD_PLAY);
	ldpc_play(LDPC_FORWARD);
}

void pr7820_pause()
{
	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void pr7820_begin_search(unsigned int uFrameNumber)
{
	common_ldp_begin_search(uFrameNumber);

	// go busy immediately so that the game will not try to send a new command before we've finished seeking.
	// (we were observing this in testing)
	PR7820_RAISE_READY_PRIME();
}

void pr7820_change_audio(unsigned char uChannel, unsigned char uEnable)
{
	ldpc_change_audio(uChannel, uEnable);
}

void pr7820_enable_super_mode()
{
	OnSuperModeChanged(1);
}

void pr7820_on_error(PR7820ErrCode_t err, unsigned char u8Val)
{
	char s[20];	// should be as short as possible

	switch (err)
	{
	default:
	case PR7820_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case PR7820_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case PR7820_ERR_TOO_MANY_DIGITS:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);
		break;
	}

	LOG_ERR(s);
}
