#ifndef COMMON_LDP_H
#define COMMON_LDP_H

#include <stdint.h>
#include <ldp-abst/ldpc.h>

extern uint8_t g_bTextOverlayEnabled;
extern uint8_t g_u8CurTextOverlayId;

void on_video_field();

void common_ldp_begin_search(uint32_t u32FrameNum);

// should be 0 for most players type.  Some games may need it to be non-zero.
void common_ldp_set_minimum_search_delay_ms(uint16_t u16DelayMs);

void common_enable_spinup_delay(uint8_t bEnabled);

void common_log_ldpc_status(LDPCStatus_t u8);

// should be called when new stopcode info comes in from media server
void common_on_new_stopcode_etc(uint32_t u32NextField, uint8_t u8FieldAfterNextOffset);

// range is 0-7, for blinking LEDs
uint8_t common_get_3bit_vsync_counter();

#endif // COMMON_LDP_H
