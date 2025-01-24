#include "protocol.h"
#include "vip9500sg-callbacks.h"
#include "dexter_strings.h"
#include "util.h"
#include "protocol.h"
#include "common-ldp.h"
#include <ldp-abst/ldpc.h>
#include <ldp-in/vip9500sg-interpreter.h>
#include <stdio.h>

/////////////////////////////////////////////////

void vip9500sgi_play()
{
	log_string(STRING_CMD_PLAY);
	ldpc_play(LDPC_FORWARD);
}

void vip9500sgi_pause()
{
	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void vip9500sgi_stop()
{
	log_string(STRING_CMD_STOP);
	ldpc_stop();
}

void vip9500sgi_step_reverse()
{
	log_string(STRING_CMD_STEP);
	ldpc_step(LDPC_BACKWARD);
}

void vip9500sgi_skip(int32_t iTracksToSkip)
{
	ldpc_skip_tracks(iTracksToSkip);
}

void vip9500sgi_change_audio(uint8_t u8Channel, uint8_t uEnable)
{
	ldpc_change_audio(u8Channel, uEnable);
}

VIP9500SGStatus_t vip9500sgi_get_status()
{
	VIP9500SGStatus_t res = VIP9500SG_ERROR;

	switch (ldpc_get_status())
	{
	case LDPC_SEARCHING:
		res = VIP9500SG_SEARCHING;
		break;
	case LDPC_STOPPED:
		res = VIP9500SG_STOPPED;
		break;
	case LDPC_PLAYING:
		res = VIP9500SG_PLAYING;
		break;
	case LDPC_PAUSED:
		res = VIP9500SG_PAUSED;
		break;
	case LDPC_STEPPING:
		res = VIP9500SG_STEPPING;
		break;
	case LDPC_SPINNING_UP:
		res = VIP9500SG_SPINNING_UP;
		break;
	default:
		res = VIP9500SG_ERROR;
		break;
	}
	return res;
}

uint32_t vip9500sgi_get_cur_frame_num()
{
	return ldpc_get_cur_frame_num();
}

uint32_t vip9500sgi_get_cur_vbi_line18()
{
	return ldpc_get_current_field_vbi_line18();
}

void vip9500sgi_error(VIP9500SGErrCode_t code, uint8_t u8Val)
{
	char s[17];

	switch (code)
	{
	default:
	case VIP9500SG_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case VIP9500SG_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case VIP9500SG_ERR_TOO_MANY_DIGITS:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);
		break;
	case VIP9500SG_ERR_UNHANDLED_SITUATION:
		string_to_buf(s, STRING_UNHANDLED_SITUATION);
		break;
	}

	LOG(s);
}

///////////////////////////////////////////

void vip9500sg_setup_callbacks()
{
	g_vip9500sgi_play = vip9500sgi_play;
	g_vip9500sgi_pause = vip9500sgi_pause;
	g_vip9500sgi_stop = vip9500sgi_stop;
	g_vip9500sgi_step_reverse = vip9500sgi_step_reverse;
	g_vip9500sgi_skip = vip9500sgi_skip;
	g_vip9500sgi_begin_search = common_ldp_begin_search;
	g_vip9500sgi_change_audio = vip9500sgi_change_audio;
	g_vip9500sgi_get_status = vip9500sgi_get_status;
	g_vip9500sgi_get_cur_frame_num = vip9500sgi_get_cur_frame_num;
	g_vip9500sgi_get_cur_vbi_line18 = vip9500sgi_get_cur_vbi_line18;
	g_vip9500sgi_error = vip9500sgi_error;
}

////////////////////////////////////////
