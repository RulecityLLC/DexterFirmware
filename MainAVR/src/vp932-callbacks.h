#include <ldp-in/vp932-interpreter.h>

void vp932_setup_callbacks();
void vp932_play(uint8_t u8Numerator, uint8_t u8Denominator, VP932_BOOL bBackward, VP932_BOOL bAudioSquelched);
void vp932_step(VP932_BOOL bBackward);
void vp932_pause();
void vp932_change_audio(uint8_t u8Channel, uint8_t uEnable);
void vp932_error(VP932ErrCode_t code, uint8_t u8Val);
