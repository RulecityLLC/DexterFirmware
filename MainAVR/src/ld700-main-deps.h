#ifndef LD700_MAIN_DEPS_H
#define LD700_MAIN_DEPS_H

#include <stdint.h>

void ld700_deps_reset();	// resets all state, for testing
void ld700_idle_think();
//void ld700_button_think();
uint8_t GetLD700CandidateSide();
//void ld700_on_vblank();
void OnFlipDiscPressed();
void OnFlipDiscHeld();
uint8_t GetDiscSideByDiscId(uint8_t u8DiscId);	// 0 = can't determine, 1 = side 1, 2 = side 2
uint8_t GetTargetDiscIdByCurDiscIdAndTargetSide(uint8_t u8DiscIdCur, uint8_t u8SideTarget);

#endif // LD700_MAIN_DEPS_H
