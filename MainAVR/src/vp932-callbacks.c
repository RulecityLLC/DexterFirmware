#include "dexter_strings.h"
#include <ldp-abst/ldpc.h>
#include "common-ldp.h"
#include "protocol.h"
#include "util.h"
#include "vp932-callbacks.h"

void vp932_setup_callbacks()
{
	g_vp932i_play = vp932_play;
	g_vp932i_step = vp932_step;
	g_vp932i_pause = vp932_pause;
	g_vp932i_begin_search = common_ldp_begin_search;
	g_vp932i_change_audio = vp932_change_audio;
	g_vp932i_error = vp932_error;
}

void vp932_play(uint8_t u8Numerator, uint8_t u8Denominator, VP932_BOOL bBackward, VP932_BOOL bAudioSquelched)
{
	// don't log play command due it being too spammy

	ldpc_change_speed(u8Numerator, u8Denominator);
	ldpc_set_audio_squelched(bAudioSquelched ? LDPC_AUDIOSQUELCH_FORCE_ON : LDPC_AUDIOSQUELCH_NO_CHANGE);
	ldpc_play(bBackward == VP932_FALSE ? LDPC_FORWARD : LDPC_BACKWARD);
}

void vp932_step(VP932_BOOL bBackward)
{
	ldpc_step(bBackward == VP932_FALSE ? LDPC_FORWARD : LDPC_BACKWARD);
}

void vp932_pause()
{
	// don't log play command due it being too spammy

	ldpc_pause();
}

void vp932_change_audio(uint8_t u8Channel, uint8_t uEnable)
{
	ldpc_change_audio(u8Channel, uEnable);
}

void vp932_error(VP932ErrCode_t err, uint8_t u8Val)
{
	char s[30];	// should be as short as possible

	switch (err)
	{	
	case VP932_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case VP932_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case VP932_ERR_RX_BUF_OVERFLOW:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);
		break;
	default:
	case VP932_ERR_UNHANDLED_SITUATION:
		string_to_buf(s, STRING_UNHANDLED_SITUATION);
		break;
	}

	LOG_ERR(s);
}
