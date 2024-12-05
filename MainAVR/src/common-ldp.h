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

#endif // COMMON_LDP_H
