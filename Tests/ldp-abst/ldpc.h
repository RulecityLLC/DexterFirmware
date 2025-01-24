#ifndef LDPC_H
#define LDPC_H

#include <cstdint>

// this enum is to make it easy to remember which field is which
// (in NTSC convention, the odd field is actually the first field which is opposite from the computer science convention of the first item being index 0, an even number)
typedef enum
{
	VID_FIELD_TOP_ODD = 0,
	VID_FIELD_BOTTOM_EVEN = 1,
	VID_FIELD_COUNT = 2	/* this is just here to make arrFieldOffsetsPerVBlank clearer */
} VidField_t;

// different states that the laserdisc player could be in
typedef enum
{
	LDPC_ERROR, LDPC_SEARCHING, LDPC_STOPPED, LDPC_PLAYING, LDPC_PAUSED, LDPC_SPINNING_UP, LDPC_STEPPING
} LDPCStatus_t;

typedef enum
{
	LDPC_FALSE = 0,
	LDPC_TRUE = 1
} LDPC_BOOL;

typedef enum
{
	LDPC_DISC_NTSC = 0,
	LDPC_DISC_PAL
} LDPCDiscType_t;

typedef enum
{
	LDPC_AUDIO_MUTED = 0,
	LDPC_AUDIO_LEFT_ONLY = 1,
	LDPC_AUDIO_RIGHT_ONLY = 2,
	LDPC_AUDIO_STEREO = 3
} LDPCAudioStatus_t;

typedef enum
{
	LDPC_FORWARD = 0,
	LDPC_BACKWARD = 1,
	LDPC_DIRECTION_COUNT = 2	/* this is just here to make arrFieldOffsetsPerVBlank clearer */
} LDPCDirection_t;

typedef enum
{
	LDPC_AUDIOSQUELCH_NO_CHANGE = 0,	// let ldpc decide whether audio should be squelched
	LDPC_AUDIOSQUELCH_FORCE_ON = 1,	// override ldpc's audio squelching rules and force audio to be squelched
	LDPC_AUDIOSQUELCH_FORCE_OFF = 2	// override ldpc's audio squelching rules and force audio to be unsquelched
} LDPCAudioSquelch_t;

LDPC_BOOL ldpc_OnVBlankChanged(LDPC_BOOL bVBlankActive, uint8_t field);

LDPCStatus_t ldpc_get_status();

#endif //LDPC_H
