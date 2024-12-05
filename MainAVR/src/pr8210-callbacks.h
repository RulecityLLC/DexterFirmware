#include <ldp-in/pr8210-interpreter.h>

// so that rev3 knows the state of the stand by line
extern uint8_t g_bPR8210AStandByRaised;

void pr8210_setup_callbacks();
void pr8210_play();
void pr8210_pause();
void pr8210_step(int8_t iTracks);
void pr8210_begin_search(uint32_t u32FrameNum);
void pr8210_change_audio(uint8_t u8Channel, uint8_t uEnable);
void pr8210_skip(int8_t i8TracksToSkip);
void pr8210_change_auto_track_jump(PR8210_BOOL bAutoTrackJumpEnabled);
PR8210_BOOL pr8210_is_player_busy();
void pr8210_change_standby(PR8210_BOOL bRaised);
void pr8210_error(PR8210ErrCode_t code, uint16_t u16Val);
