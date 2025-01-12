#ifndef LD700_CALLBACKS_H
#define LD700_CALLBACKS_H

#include <ldp-in/ld700-interpreter.h>

void ld700_setup_callbacks();
LD700Status_t ld700_convert_status(LDPCStatus_t status);
void ld700_play();
void ld700_pause();
void ld700_step(LD700_BOOL bStepReverse);
void ld700_stop();
void ld700_close_tray();
void ld700_eject();
void ld700_on_ext_ack_changed(LD700_BOOL bActive);
void ld700_change_audio(LD700_BOOL bEnableLeft, LD700_BOOL bEnableRight);
void ld700_error(LD700ErrCode_t err, uint8_t u8Val);

#endif // LD700_CALLBACKS_H
