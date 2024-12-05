#ifndef PR7820_CALLBACKS_H
#define PR7820_CALLBACKS_H

#include <ldp-in/pr7820-interpreter.h>

void pr7820_setup_callbacks();

PR7820Status_t pr7820_get_status();

void pr7820_play();

void pr7820_pause();

void pr7820_begin_search(unsigned int uFrameNumber);

void pr7820_change_audio(unsigned char uChannel, unsigned char uEnable);

void pr7820_enable_super_mode();

void pr7820_on_error(PR7820ErrCode_t code, unsigned char u8Value);

#endif // PR7820_CALLBACKS_H
