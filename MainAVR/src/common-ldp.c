#include "strings.h"
#include "protocol.h"
#include "settings.h"
#include "led_driver.h"
#include "common-ldp.h"
#include <ldp-abst/ldp_search_delay.h>

uint8_t g_u8LDPCLastStatus = 0;
uint8_t g_u8VsyncCounter = 0;

// whether text overlay is currently enabled.
uint8_t g_bTextOverlayEnabled = 0;

// the text overlay payload is set separately.  The ID is sent with every field to indicate that the text overlay payload should be displayed.
uint8_t g_u8CurTextOverlayId = 0;

// minimum number of fields to delay for a seek (some games may need this to be non-zero)
uint8_t g_u8MinimumSearchDelayFields = 0;

void on_video_field()
{
	LDPCStatus_t u8 = ldpc_get_status();

	ldpsd_on_vsync();	// advance search delay	

	// if disc is not searching 
	if (u8 != LDPC_SEARCHING)
	{
		uint32_t u = ldpc_get_current_abs_field();

		// if current field is valid (ie disc is not stopped or spinning up) and video is not muted (time traveler mutes video sometimes)
		if ((u != VBIMiniNoFrame) && (ldpc_get_video_muted() == LDPC_FALSE))
		{
			// if text overlay is enabled, send a special packet to indicate such
			if (g_bTextOverlayEnabled)
			{
				MediaServerSendFieldWithText(u, g_u8CurTextOverlayId, ldpc_get_audio_status());
			}
			else
			{
				// send current field to frame server
				MediaServerSendField(u, ldpc_get_audio_status());
			}
		}
		// else disc is stopped, spinning up, or muted so just blank the screen
		else
		{
			MediaServerSendBlankScreen();
		}
	}
	// If we have a status of searching...
	else
	{
		// Blank the screen if seek delay is enabled, otherwise do not.
		// (We can still come here if seek delay is disabled due to landing in the middle of a frame)
		if (IsSearchDelayEnabledMemory())
		{
			MediaServerSendBlankScreen();
		}

		// if the search can finished, do so now
		if (ldpsd_can_search_finish())
		{
			// no point logging that the search is ending because the status will change to 'paused' which will indicate the same thing
			ldpc_end_search();
		}
	}

	// log ldpc status changes only
	if (u8 != g_u8LDPCLastStatus)
	{
		common_log_ldpc_status(u8);
		g_u8LDPCLastStatus = u8;
	}

	g_u8VsyncCounter++;
	if (g_u8VsyncCounter & 0x8)
	{
		// blink the LED so we know the interrupt is working
		LED_VSYNC_PIN_TOGGLE();
		g_u8VsyncCounter = 0;
	}
}

void common_ldp_begin_search(uint32_t u32FrameNum)
{
	char s[35];	// make this as small as possible
	char s1[30];
	uint32_t uStartField = ldpc_get_current_abs_field();
	LDPC_BOOL bSearchSuccess = ldpc_begin_search(u32FrameNum);

	string_to_buf(s1, STRING_CMD_SEEK_TO_FRAME);
	sprintf(s, s1, u32FrameNum);
	LOG(s);

	// if search fails immediately (due to being out of range most likely)	
	if (bSearchSuccess == LDPC_FALSE)
	{
		// don't need to log anything here because the on_video_field method will log it for us later
		ldpc_end_search();
		return;
	}
	
	// if we have some search delay
	//  AND the disc is not stopped or spinning up
	if ((IsSearchDelayEnabledMemory()) && (uStartField != VBIMiniNoFrame))
	{
		uint32_t uTargetField = ldpc_get_last_searched_field();
		uint32_t uTrackDelta;
		uint8_t u8FieldsToDelay;

		// get absolute value of the delta.
		//  We have to do a comparison no matter what, so may as well do it this way
		if (uTargetField >= uStartField)
		{
			uTrackDelta = uTargetField - uStartField;
		}
		else
		{
			uTrackDelta = uStartField - uTargetField;
		}
		uTrackDelta >>= 1;	// divide by 2 to convert from fields to tracks

		// if we are doing a very short seek, different rules apply
		if (uTrackDelta < 200)
		{
			// (((uFieldsToDelay * 1.3) + 100) / 16.683) + 0.5 is the algorithm.  Based on actual measurements from an LDP-1450.
			// Rewritten to not use floating point math.
			// The +0.5 is to round to the nearest whole number.
			// 167 is 16.683 * 10
//			u8FieldsToDelay = (uint16_t) (((uTrackDelta * 13) + 1300) / 167);

			// reduced 50% at customer's request
			u8FieldsToDelay = (uint8_t) (((uTrackDelta * 6) + 650) / 167);

		}
		// else if we are doing a longer seek
		else
		{
			// (((uFieldsToDelay * 0.0435) + 375) / 16.683) + 0.5 is the algorithm.  Based on actual measurements from an LDP-1450.
			// Rewritten to not use floating point math.
			// 1/23 is .0435
			// 9315 is 375*23 + 0.5*16.683*23
			// 384 is 16.683*23
			// The +0.5 is to round to the nearest whole number.
//			u8FieldsToDelay = (uint16_t) ((uTrackDelta + 8817) / 384);

			// reduced 33% at customer request
			u8FieldsToDelay = (uint8_t) ((uTrackDelta + 8817) / 576);
		}

		// don't go below our minimum
		if (u8FieldsToDelay < g_u8MinimumSearchDelayFields)
		{
			u8FieldsToDelay = g_u8MinimumSearchDelayFields;
		}

		ldpsd_start_counter(u8FieldsToDelay);
	}
	// else search delay is disabled,
	//  or we got a search before a play command, which probably means dexter got reset while the game was already running;
	// We will immediately complete the search in this latter case since we have no way of knowing what frame we were supposed to be coming from.
	else
	{
		ldpc_end_search();
	}
}

void common_ldp_set_minimum_search_delay_ms(uint16_t u16DelayMs)
{
	// Dividing by 16 is approximately correct (and fast!) since each field is 16.68ms
	// We assume that a byte is enough to hold this value since no player would have minimum search delay longer than 255 fields.
	g_u8MinimumSearchDelayFields = u16DelayMs >> 4;
}

void common_enable_spinup_delay(uint8_t bEnabled)
{	
	ldpc_set_vblanks_per_spinup(bEnabled ?

	// This is about right for Dragon's Lair, not too fast, not too slow (it will be about 10 seconds)
		(10 * 60)

		: 0);
}

void common_log_ldpc_status(LDPCStatus_t u8)
{
	switch (u8)
	{
	default:
		log_string(STRING_ERROR);
		break;
	case LDPC_STOPPED:
		log_string(STRING_STOPPED);
		break;
	case LDPC_SEARCHING:
		log_string(STRING_SEARCHING);
		break;
	case LDPC_PAUSED:
		log_string(STRING_PAUSED);
		break;
	case LDPC_SPINNING_UP:
		log_string(STRING_SPINNING_UP);
		break;
	case LDPC_PLAYING:
		log_string(STRING_PLAYING);
		break;
	case LDPC_STEPPING:
		// do not log stepping status since it will change almost immediately to paused
		break;
	}
}