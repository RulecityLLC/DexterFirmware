#include <avr/io.h>
#include "strings.h"
#include <ldp-abst/ldpc.h>
#include "common-ldp.h"
#include "protocol.h"
#include "util.h"	// for ByteToHexString
#include "ld700-callbacks.h"

void ld700_setup_callbacks()
{
	g_ld700i_play = ld700_play;
	g_ld700i_pause = ld700_pause;
	g_ld700i_stop = ld700_stop;
	g_ld700i_eject = ld700_eject;
	g_ld700i_begin_search = common_ldp_begin_search;
	g_ld700i_change_audio = ld700_change_audio;
	g_ld700i_on_ext_ack_changed = ld700_on_ext_ack_changed;
	g_ld700i_error = ld700_error;
}

// ldpc doesn't currently keep track of whether disc is ejected, so we need to here
uint8_t g_bLD700DiscEjected = 0;

LD700Status_t ld700_convert_status(LDPCStatus_t status)
{
	LD700Status_t res = LD700_ERROR;

	switch (status)
	{
	case LDPC_SEARCHING:
		res = LD700_SEARCHING;
		break;
	case LDPC_PLAYING:
		res = LD700_PLAYING;
		break;
	case LDPC_PAUSED:
		res = LD700_PAUSED;
		break;
	case LDPC_SPINNING_UP:
		res = LD700_SPINNING_UP;
		break;
	case LDPC_STOPPED:
		if (g_bLD700DiscEjected)
		{
			res = LD700_TRAY_EJECTED;
		}
		else
		{
			res = LD700_STOPPED;
		}
		break;
	default:
		res = LD700_ERROR;
		break;
	}
	return res;
}

void ld700_play()
{
	log_string(STRING_CMD_PLAY);
	ldpc_play(LDPC_FORWARD);
	g_bLD700DiscEjected = 0;	// it seems if the disc plays, that that's good enough to 'insert' an ejected disc
}

void ld700_pause()
{
	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void ld700_stop()
{
	log_string(STRING_CMD_STOP);
	ldpc_stop();
}

void ld700_close_tray()
{
	log_string(STRING_CMD_STOP);	// so user has some feedback
	ldpc_stop();
	g_bLD700DiscEjected = 0;	// semi-hack to insert a disc (call 'stop')
}

void ld700_eject()
{
	log_string(STRING_CMD_EJECT);
	ldpc_stop();	// this is the closest thing that ldpc currently supports to ejecting the disc
	g_bLD700DiscEjected = 1;
}

void ld700_on_ext_ack_changed(LD700_BOOL bActive)
{
	// PA7: EXT_ACK'
	if (bActive)
	{
		PORTA &= ~(1 << PA7);
	}
	else
	{
		PORTA |= (1 << PA7);
	}
}

void ld700_change_audio(LD700_BOOL bEnableLeft, LD700_BOOL bEnableRight)
{
	ldpc_change_audio(0, bEnableLeft);
	ldpc_change_audio(1, bEnableRight);
}

void ld700_error(LD700ErrCode_t err, uint8_t u8Val)
{
	char s[30];	// should be as short as possible

	switch (err)
	{	
	case LD700_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case LD700_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case LD700_ERR_TOO_MANY_DIGITS:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);
		break;
	default:
	case LD700_ERR_UNHANDLED_SITUATION:
		string_to_buf(s, STRING_UNHANDLED_SITUATION);
		break;
	}

	LOG_ERR(s);
}
