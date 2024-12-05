#include <ldp-in/ldp1000-interpreter.h>

extern uint8_t g_bLdp1000_text_needs_update;
extern uint8_t g_bLdp1000_text_overlay_enabled;
extern uint8_t g_pTextOverlayPacket[];	// so that ldp1000 main can send the text ID

void ldp1000_setup_callbacks();

void ldp1000_send_text_overlay_packet();

void ldp1000_play(uint8_t u8Numerator, uint8_t u8Denominator, LDP1000_BOOL bBackward, LDP1000_BOOL bAudioSquelched);

void ldp1000_pause();

void ldp1000_begin_search(uint32_t u32FrameNum);

void ldp1000_step_forward();

void ldp1000_step_reverse();

void ldp1000_skip(int16_t i16TracksToSkip);

void ldp1000_change_audio(uint8_t u8Channel, uint8_t uEnable);

void ldp1000_change_video(LDP1000_BOOL bEnabled);

LDP1000Status_t ldp1000_get_status();

uint32_t ldp1000_get_cur_frame_num();

// start new
void ldp1000_text_enable_changed(LDP1000_BOOL bEnabled);

void ldp1000_text_buffer_contents_changed(const uint8_t *p8Buf32Bytes);

void ldp1000_text_buffer_start_index_changed(uint8_t u8StartIdx);

void ldp1000_text_modes_changed(uint8_t u8Mode, uint8_t u8X, uint8_t u8Y);
// end new

void ldp1000_error(LDP1000ErrCode_t code, uint8_t u8Val);
