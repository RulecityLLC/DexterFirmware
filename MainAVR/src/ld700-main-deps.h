#ifndef LD700_MAIN_DEPS_H
#define LD700_MAIN_DEPS_H

#include <stdint.h>
#include "ld700-callbacks.h"	// for LD700Status_t definition

void ld700_deps_reset();	// resets all state, for testing
void ld700_idle_think();
uint8_t GetLD700CandidateSide();
void OnFlipDiscPressed(LD700Status_t status);
void OnFlipDiscHeld(LD700Status_t status);
uint8_t GetDiscSideByDiscId(uint8_t u8DiscId);	// 0 = can't determine, 1 = side 1, 2 = side 2
uint8_t GetTargetDiscIdByCurDiscIdAndTargetSide(uint8_t u8DiscIdCur, uint8_t u8SideTarget);

#endif // LD700_MAIN_DEPS_H
