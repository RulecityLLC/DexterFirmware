#include <ldp-in/vp931-interpreter.h>

void vp931_setup_callbacks();

void vp931_play();

void vp931_pause();

void vp931_begin_search(uint32_t uFrameNumber, VP931_BOOL bSquelch);

void vp931_skip_tracks(int16_t i16TracksToSkip);

void vp931_skip_to_framenum(uint32_t u32FrameNum);

void vp931_error(VP931ErrCode_t code, uint8_t u8Val);
