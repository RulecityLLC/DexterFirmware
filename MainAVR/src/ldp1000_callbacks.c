#include "protocol.h"
#include "ldp1000_callbacks.h"
#include "dexter_strings.h"
#include "util.h"
#include "protocol.h"
#include <ldp-abst/ldpc.h>
#include "common-ldp.h"	// for text overlay generic variables
#include <stdio.h>

/////////////////////////////////////////////////

uint8_t g_bLdp1000_text_needs_update = 0;	// whether text update packet needs to be sent

// complete text overlay packet, stored so that we only need to do minimal processing on it in order to send it
uint8_t g_pTextOverlayPacket[38] = { 'T', 0, 0, 0, 0, 0 };	// stored so it can easily be re-sent if it gets dropped

/////////////////////////////////////////////////

void ldp1000_setup_callbacks()
{
	g_ldp1000i_play = ldp1000_play;
	g_ldp1000i_pause = ldp1000_pause;
	g_ldp1000i_begin_search = common_ldp_begin_search;
	g_ldp1000i_step_forward = ldp1000_step_forward;
	g_ldp1000i_step_reverse = ldp1000_step_reverse;
	g_ldp1000i_skip = ldp1000_skip;
	g_ldp1000i_change_audio = ldp1000_change_audio;
	g_ldp1000i_change_video = ldp1000_change_video;
	g_ldp1000i_get_status = ldp1000_get_status;
	g_ldp1000i_get_cur_frame_num = ldp1000_get_cur_frame_num;
	g_ldp1000i_text_enable_changed = ldp1000_text_enable_changed;
	g_ldp1000i_text_buffer_contents_changed = ldp1000_text_buffer_contents_changed;
	g_ldp1000i_text_buffer_start_index_changed = ldp1000_text_buffer_start_index_changed;
	g_ldp1000i_text_modes_changed = ldp1000_text_modes_changed;
	g_ldp1000i_error = ldp1000_error;
}

// should be called if *text_needs_update is non-zero
void ldp1000_send_text_overlay_packet()
{
	g_pTextOverlayPacket[1]++;	// the ID needs to change so that media server knows that the payload is changing
	g_u8CurTextOverlayId = g_pTextOverlayPacket[1];	// common LDP routine needs to know the new ID
	MediaServerSendSmallBuf(g_pTextOverlayPacket, sizeof(g_pTextOverlayPacket));
	g_bLdp1000_text_needs_update = 0;
}

////////////////////////////////////////

void ldp1000_play(uint8_t u8Numerator, uint8_t u8Denominator, LDP1000_BOOL bBackward, LDP1000_BOOL bAudioSquelched)
{
// 	play can be spammy on the ldp-1450 due to time traveler sending 0x3d commands which send two plays back to back
//	log_string(STRING_CMD_PLAY);
	ldpc_change_speed(u8Numerator, u8Denominator);

	ldpc_set_audio_squelched(bAudioSquelched ? LDPC_AUDIOSQUELCH_FORCE_ON : LDPC_AUDIOSQUELCH_NO_CHANGE);

	ldpc_play(bBackward == LDP1000_FALSE ? LDPC_FORWARD : LDPC_BACKWARD);
}

void ldp1000_pause()
{
	// pause is very spammy on the ldp-1450 due to search and repeat commands pausing before the command has been fully received
//	log_string(STRING_CMD_PAUSE);
	ldpc_pause();
}

void ldp1000_step_forward()
{
	ldpc_step(LDPC_FORWARD);
}

void ldp1000_step_reverse()
{
	ldpc_step(LDPC_BACKWARD);
}

void ldp1000_skip(int16_t i16TracksToSkip)
{
	ldpc_skip_tracks(i16TracksToSkip);
}

void ldp1000_change_audio(uint8_t u8Channel, uint8_t uEnable)
{
	ldpc_change_audio(u8Channel, uEnable);
}

void ldp1000_change_video(LDP1000_BOOL bEnabled)
{
	ldpc_set_video_muted((bEnabled == LDP1000_TRUE) ? LDPC_FALSE : LDPC_TRUE);
}

LDP1000Status_t ldp1000_get_status()
{
	LDP1000Status_t res = LDP1000_ERROR;

	switch (ldpc_get_status())
	{
	case LDPC_SEARCHING:
		res = LDP1000_SEARCHING;
		break;
	case LDPC_STOPPED:
		res = LDP1000_STOPPED;
		break;
	case LDPC_PLAYING:
		res = LDP1000_PLAYING;
		break;
	case LDPC_PAUSED:
		res = LDP1000_PAUSED;
		break;
	case LDPC_SPINNING_UP:
		res = LDP1000_SPINNING_UP;
		break;
	default:
		res = LDP1000_ERROR;
		break;
	}
	return res;
}

uint32_t ldp1000_get_cur_frame_num()
{
	return ldpc_get_cur_frame_num();
}

void ldp1000_text_enable_changed(LDP1000_BOOL bEnabled)
{
	g_bLdp1000_text_needs_update = 1;
	g_bTextOverlayEnabled = bEnabled;
}

void ldp1000_text_buffer_contents_changed(const uint8_t *p8Buf32Bytes)
{
	uint8_t u;
	uint8_t *pDst = g_pTextOverlayPacket + 6;
	const uint8_t *pSrc = p8Buf32Bytes;

	g_bLdp1000_text_needs_update = g_bTextOverlayEnabled;
	
	for (u = 0; u < 32; u++)
	{
		*(pDst++) = *(pSrc++);
	}
}

void ldp1000_text_buffer_start_index_changed(uint8_t u8StartIdx)
{
	g_bLdp1000_text_needs_update = g_bTextOverlayEnabled;
	g_pTextOverlayPacket[2] = u8StartIdx;
}

void ldp1000_text_modes_changed(uint8_t u8Mode, uint8_t u8X, uint8_t u8Y)
{
	g_bLdp1000_text_needs_update = g_bTextOverlayEnabled;
	g_pTextOverlayPacket[3] = u8X;
	g_pTextOverlayPacket[4] = u8Y;
	g_pTextOverlayPacket[5] = u8Mode;
}

void ldp1000_error(LDP1000ErrCode_t code, uint8_t u8Val)
{
	char s[17];

	switch (code)
	{
	default:
	case LDP1000_ERR_UNKNOWN_CMD_BYTE:
		string_to_buf(s, STRING_UNKNOWN_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case LDP1000_ERR_UNSUPPORTED_CMD_BYTE:
		string_to_buf(s, STRING_UNSUPPORTED_BYTE);
		ByteToHexString(&s[14], u8Val);
		break;
	case LDP1000_ERR_TOO_MANY_DIGITS:
		string_to_buf(s, STRING_TOO_MANY_DIGITS);
		break;
	case LDP1000_ERR_UNHANDLED_SITUATION:
		string_to_buf(s, STRING_UNHANDLED_SITUATION);
		break;
	}

	LOG(s);
}
