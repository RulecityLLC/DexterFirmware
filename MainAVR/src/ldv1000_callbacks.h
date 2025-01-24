#include <ldp-in/ldv1000-interpreter.h>

void ldv1000_setup_callbacks();

LDV1000Status_t ldv1000_get_status();

uint32_t ldv1000_get_cur_frame_num();

void ldv1000_play();

void ldv1000_pause();

void ldv1000_step_reverse();

void ldv1000_change_speed(uint8_t uNum, uint8_t uDenom);

void ldv1000_skip_forward(uint8_t uTracks);

void ldv1000_skip_backward(uint8_t uTracks);

void ldv1000_change_audio(uint8_t uChannel, uint8_t Enable);

void ldv1000_on_error(const char *pszErrMsg);

void ldv1000_begin_changing_to_disc(uint8_t idDisc);
